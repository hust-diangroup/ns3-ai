import numpy as np
import gymnasium as gym
from gymnasium import spaces
# from gymnasium.utils import seeding
import messages_pb2 as pb
import ns3ai_gym_msg_py as ns3msg
import py_cycle

class SharedMemoryBridge(object):
    def __init__(self):
        self.msg_interface = ns3msg.Ns3AiMsgInterface(
            True,
            False,
            False,
            ns3msg.msg_buffer_size * 4,  # should > approx twice the size of buffer
            "My Seg",
            "My Cpp to Python Msg",
            "My Python to Cpp Msg",
            "My Lockable")

        self.envStopped = False
        self.forceEnvStop = False

        self._action_space = None
        self._observation_space = None

        self.newStateRx = False
        self.obsData = None
        self.reward = 0
        self.gameOver = False
        self.gameOverReason = None
        self.extraInfo = None
        self.recv_env_cycle = 0
        self.send_act_cycle = 0
        print('Created message interface, waiting for C++ side to send initial environment...')

    def close(self):
        if not self.envStopped:
            self.envStopped = True
            self.force_env_stop()
            self.rx_env_state()
            self.send_close_command()
            del self.msg_interface

    def _create_space(self, spaceDesc):
        space = None
        if spaceDesc.type == pb.Discrete:
            discreteSpacePb = pb.DiscreteSpace()
            spaceDesc.space.Unpack(discreteSpacePb)
            space = spaces.Discrete(discreteSpacePb.n)

        elif spaceDesc.type == pb.Box:
            boxSpacePb = pb.BoxSpace()
            spaceDesc.space.Unpack(boxSpacePb)
            low = boxSpacePb.low
            high = boxSpacePb.high
            shape = tuple(boxSpacePb.shape)
            mtype = boxSpacePb.dtype

            if mtype == pb.INT:
                mtype = np.int
            elif mtype == pb.UINT:
                mtype = np.uint
            elif mtype == pb.DOUBLE:
                mtype = np.float
            else:
                mtype = np.float

            space = spaces.Box(low=low, high=high, shape=shape, dtype=mtype)

        elif spaceDesc.type == pb.Tuple:
            mySpaceList = []
            tupleSpacePb = pb.TupleSpace()
            spaceDesc.space.Unpack(tupleSpacePb)

            for pbSubSpaceDesc in tupleSpacePb.element:
                subSpace = self._create_space(pbSubSpaceDesc)
                mySpaceList.append(subSpace)

            mySpaceTuple = tuple(mySpaceList)
            space = spaces.Tuple(mySpaceTuple)

        elif spaceDesc.type == pb.Dict:
            mySpaceDict = {}
            dictSpacePb = pb.DictSpace()
            spaceDesc.space.Unpack(dictSpacePb)

            for pbSubSpaceDesc in dictSpacePb.element:
                subSpace = self._create_space(pbSubSpaceDesc)
                mySpaceDict[pbSubSpaceDesc.name] = subSpace

            space = spaces.Dict(mySpaceDict)

        return space

    def initialize_env(self):
        simInitMsg = pb.SimInitMsg()
        self.msg_interface.py_recv_begin()
        request = self.msg_interface.m_single_cpp2py_msg.get_buffer()
        simInitMsg.ParseFromString(request)
        self.msg_interface.py_recv_end()

        self._action_space = self._create_space(simInitMsg.actSpace)
        self._observation_space = self._create_space(simInitMsg.obsSpace)

        reply = pb.SimInitAck()
        reply.done = True
        reply.stopSimReq = False
        reply_str = reply.SerializeToString()
        assert len(reply_str) <= ns3msg.msg_buffer_size
        self.msg_interface.py_send_begin()
        self.msg_interface.m_single_py2cpp_msg.size = len(reply_str)
        self.msg_interface.m_single_py2cpp_msg.get_buffer_full()[:len(reply_str)] = reply_str
        self.msg_interface.py_send_end()
        return True

    def get_action_space(self):
        return self._action_space

    def get_observation_space(self):
        return self._observation_space

    def force_env_stop(self):
        self.forceEnvStop = True

    def rx_env_state(self):
        if self.newStateRx:
            return

        # request = self.socket.recv()
        envStateMsg = pb.EnvStateMsg()
        self.msg_interface.py_recv_begin()
        request = self.msg_interface.m_single_cpp2py_msg.get_buffer()
        envStateMsg.ParseFromString(request)
        self.msg_interface.py_recv_end()

        # For benchmarking: here get CPU cycle
        self.recv_env_cycle = py_cycle.getCycle()

        self.obsData = self._create_data(envStateMsg.obsData)
        self.reward = envStateMsg.reward
        self.gameOver = envStateMsg.isGameOver
        self.gameOverReason = envStateMsg.reason

        if self.gameOver:
            if self.gameOverReason == pb.EnvStateMsg.SimulationEnd:
                self.envStopped = True
                self.send_close_command()
            else:
                self.forceEnvStop = True
                self.send_close_command()

        self.extraInfo = envStateMsg.info
        if not self.extraInfo:
            self.extraInfo = {}

        self.newStateRx = True

    def send_close_command(self):
        reply = pb.EnvActMsg()
        reply.stopSimReq = True

        replyMsg = reply.SerializeToString()
        assert len(replyMsg) <= ns3msg.msg_buffer_size
        self.msg_interface.py_send_begin()
        self.msg_interface.m_single_py2cpp_msg.size = len(replyMsg)
        self.msg_interface.m_single_py2cpp_msg.get_buffer_full()[:len(replyMsg)] = replyMsg
        self.msg_interface.py_send_end()

        # self.socket.send(replyMsg)
        self.newStateRx = False
        return True

    def send_actions(self, actions):
        reply = pb.EnvActMsg()

        actionMsg = self._pack_data(actions, self._action_space)
        reply.actData.CopyFrom(actionMsg)

        reply.stopSimReq = False
        if self.forceEnvStop:
            reply.stopSimReq = True

        # For benchmarking: here get CPU cycle
        self.send_act_cycle = py_cycle.getCycle()
        reply.pyRecvEnvCpuCycle = self.recv_env_cycle
        reply.pySendActCpuCycle = self.send_act_cycle

        replyMsg = reply.SerializeToString()
        assert len(replyMsg) <= ns3msg.msg_buffer_size
        self.msg_interface.py_send_begin()
        self.msg_interface.m_single_py2cpp_msg.size = len(replyMsg)
        self.msg_interface.m_single_py2cpp_msg.get_buffer_full()[:len(replyMsg)] = replyMsg
        self.msg_interface.py_send_end()
        self.newStateRx = False
        return True

    def step(self, actions):
        self.send_actions(actions)
        self.rx_env_state()

    def is_game_over(self):
        return self.gameOver

    def _create_data(self, dataContainerPb):
        if dataContainerPb.type == pb.Discrete:
            discreteContainerPb = pb.DiscreteDataContainer()
            dataContainerPb.data.Unpack(discreteContainerPb)
            data = discreteContainerPb.data
            return data

        if dataContainerPb.type == pb.Box:
            boxContainerPb = pb.BoxDataContainer()
            dataContainerPb.data.Unpack(boxContainerPb)
            # print(boxContainerPb.shape, boxContainerPb.dtype, boxContainerPb.uintData)

            if boxContainerPb.dtype == pb.INT:
                data = boxContainerPb.intData
            elif boxContainerPb.dtype == pb.UINT:
                data = boxContainerPb.uintData
            elif boxContainerPb.dtype == pb.DOUBLE:
                data = boxContainerPb.doubleData
            else:
                data = boxContainerPb.floatData

            # TODO: reshape using shape info
            data = np.array(data)
            return data

        elif dataContainerPb.type == pb.Tuple:
            tupleDataPb = pb.TupleDataContainer()
            dataContainerPb.data.Unpack(tupleDataPb)

            myDataList = []
            for pbSubData in tupleDataPb.element:
                subData = self._create_data(pbSubData)
                myDataList.append(subData)

            data = tuple(myDataList)
            return data

        elif dataContainerPb.type == pb.Dict:
            dictDataPb = pb.DictDataContainer()
            dataContainerPb.data.Unpack(dictDataPb)

            myDataDict = {}
            for pbSubData in dictDataPb.element:
                subData = self._create_data(pbSubData)
                myDataDict[pbSubData.name] = subData

            data = myDataDict
            return data

    def get_obs(self):
        return self.obsData

    def get_reward(self):
        return self.reward

    def get_extra_info(self):
        return self.extraInfo

    def _pack_data(self, actions, spaceDesc):
        dataContainer = pb.DataContainer()

        spaceType = spaceDesc.__class__

        if spaceType == spaces.Discrete:
            dataContainer.type = pb.Discrete
            discreteContainerPb = pb.DiscreteDataContainer()
            discreteContainerPb.data = actions
            dataContainer.data.Pack(discreteContainerPb)

        elif spaceType == spaces.Box:
            dataContainer.type = pb.Box
            boxContainerPb = pb.BoxDataContainer()
            shape = [len(actions)]
            boxContainerPb.shape.extend(shape)

            if spaceDesc.dtype in ['int', 'int8', 'int16', 'int32', 'int64']:
                boxContainerPb.dtype = pb.INT
                boxContainerPb.intData.extend(actions)

            elif spaceDesc.dtype in ['uint', 'uint8', 'uint16', 'uint32', 'uint64']:
                boxContainerPb.dtype = pb.UINT
                boxContainerPb.uintData.extend(actions)

            elif spaceDesc.dtype in ['float', 'float32', 'float64']:
                boxContainerPb.dtype = pb.FLOAT
                boxContainerPb.floatData.extend(actions)

            elif spaceDesc.dtype in ['double']:
                boxContainerPb.dtype = pb.DOUBLE
                boxContainerPb.doubleData.extend(actions)

            else:
                boxContainerPb.dtype = pb.FLOAT
                boxContainerPb.floatData.extend(actions)

            dataContainer.data.Pack(boxContainerPb)

        elif spaceType == spaces.Tuple:
            dataContainer.type = pb.Tuple
            tupleDataPb = pb.TupleDataContainer()

            spaceList = list(self._action_space.spaces)
            subDataList = []
            for subAction, subActSpaceType in zip(actions, spaceList):
                subData = self._pack_data(subAction, subActSpaceType)
                subDataList.append(subData)

            tupleDataPb.element.extend(subDataList)
            dataContainer.data.Pack(tupleDataPb)

        elif spaceType == spaces.Dict:
            dataContainer.type = pb.Dict
            dictDataPb = pb.DictDataContainer()

            subDataList = []
            for sName, subAction in actions.items():
                subActSpaceType = self._action_space.spaces[sName]
                subData = self._pack_data(subAction, subActSpaceType)
                subData.name = sName
                subDataList.append(subData)

            dictDataPb.element.extend(subDataList)
            dataContainer.data.Pack(dictDataPb)

        return dataContainer


class Ns3Env(gym.Env):
    def __init__(self):
        self.SharedMemoryBridge = SharedMemoryBridge()
        self.SharedMemoryBridge.initialize_env()
        self.action_space = self.SharedMemoryBridge.get_action_space()
        self.observation_space = self.SharedMemoryBridge.get_observation_space()
        # get first observations
        self.SharedMemoryBridge.rx_env_state()
        self.envDirty = False
        # self.seed()

    # def seed(self, seed=None):
    #     self.np_random, seed = seeding.np_random(seed)
    #     return [seed]

    def get_state(self):
        obs = self.SharedMemoryBridge.get_obs()
        reward = self.SharedMemoryBridge.get_reward()
        done = self.SharedMemoryBridge.is_game_over()
        extraInfo = {"info": self.SharedMemoryBridge.get_extra_info()}
        return obs, reward, done, False, extraInfo

    def step(self, action):
        self.SharedMemoryBridge.step(action)
        self.envDirty = True
        return self.get_state()

    def reset(self, seed=None, options=None):
        if not self.envDirty:
            obs = self.SharedMemoryBridge.get_obs()
            return obs, {}

        if self.SharedMemoryBridge:
            self.SharedMemoryBridge.close()
            self.SharedMemoryBridge = None

        self.envDirty = False
        self.SharedMemoryBridge = SharedMemoryBridge()
        self.SharedMemoryBridge.initialize_env()
        self.action_space = self.SharedMemoryBridge.get_action_space()
        self.observation_space = self.SharedMemoryBridge.get_observation_space()
        # get first observation
        self.SharedMemoryBridge.rx_env_state()
        obs = self.SharedMemoryBridge.get_obs()
        return obs, {}

    def render(self, mode='human'):
        return

    def get_random_action(self):
        act = self.action_space.sample()
        return act

    def close(self):
        if self.SharedMemoryBridge:
            self.SharedMemoryBridge.close()
            self.SharedMemoryBridge = None

        # if self.viewer:
        #     self.viewer.close()
