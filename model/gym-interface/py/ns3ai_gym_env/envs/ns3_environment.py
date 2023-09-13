import numpy as np
import gymnasium as gym
from gymnasium import spaces
import messages_pb2 as pb
import ns3ai_gym_msg_py as py_binding
from ns3ai_utils import Experiment


class Ns3Env(gym.Env):
    _created = False

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

    def initialize_env(self):
        simInitMsg = pb.SimInitMsg()
        self.msgInterface.PyRecvBegin()
        request = self.msgInterface.GetCpp2PyStruct().get_buffer()
        simInitMsg.ParseFromString(request)
        self.msgInterface.PyRecvEnd()

        self.action_space = self._create_space(simInitMsg.actSpace)
        self.observation_space = self._create_space(simInitMsg.obsSpace)

        reply = pb.SimInitAck()
        reply.done = True
        reply.stopSimReq = False
        reply_str = reply.SerializeToString()
        assert len(reply_str) <= py_binding.msg_buffer_size

        self.msgInterface.PySendBegin()
        self.msgInterface.GetPy2CppStruct().size = len(reply_str)
        self.msgInterface.GetPy2CppStruct().get_buffer_full()[:len(reply_str)] = reply_str
        self.msgInterface.PySendEnd()
        return True

    def send_close_command(self):
        reply = pb.EnvActMsg()
        reply.stopSimReq = True

        replyMsg = reply.SerializeToString()
        assert len(replyMsg) <= py_binding.msg_buffer_size
        self.msgInterface.PySendBegin()
        self.msgInterface.GetPy2CppStruct().size = len(replyMsg)
        self.msgInterface.GetPy2CppStruct().get_buffer_full()[:len(replyMsg)] = replyMsg
        self.msgInterface.PySendEnd()

        self.newStateRx = False
        return True

    def rx_env_state(self):
        if self.newStateRx:
            return

        envStateMsg = pb.EnvStateMsg()
        self.msgInterface.PyRecvBegin()
        request = self.msgInterface.GetCpp2PyStruct().get_buffer()
        envStateMsg.ParseFromString(request)
        self.msgInterface.PyRecvEnd()

        self.obsData = self._create_data(envStateMsg.obsData)
        self.reward = envStateMsg.reward
        self.gameOver = envStateMsg.isGameOver
        self.gameOverReason = envStateMsg.reason

        if self.gameOver:
            self.send_close_command()

        self.extraInfo = envStateMsg.info
        if not self.extraInfo:
            self.extraInfo = {}

        self.newStateRx = True

    def get_obs(self):
        return self.obsData

    def get_reward(self):
        return self.reward

    def is_game_over(self):
        return self.gameOver

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

            spaceList = list(self.action_space.spaces)
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
                subActSpaceType = self.action_space.spaces[sName]
                subData = self._pack_data(subAction, subActSpaceType)
                subData.name = sName
                subDataList.append(subData)

            dictDataPb.element.extend(subDataList)
            dataContainer.data.Pack(dictDataPb)

        return dataContainer

    def send_actions(self, actions):
        reply = pb.EnvActMsg()

        actionMsg = self._pack_data(actions, self.action_space)
        reply.actData.CopyFrom(actionMsg)

        replyMsg = reply.SerializeToString()
        assert len(replyMsg) <= py_binding.msg_buffer_size
        self.msgInterface.PySendBegin()
        self.msgInterface.GetPy2CppStruct().size = len(replyMsg)
        self.msgInterface.GetPy2CppStruct().get_buffer_full()[:len(replyMsg)] = replyMsg
        self.msgInterface.PySendEnd()
        self.newStateRx = False
        return True

    def get_state(self):
        obs = self.get_obs()
        reward = self.get_reward()
        done = self.is_game_over()
        extraInfo = {"info": self.get_extra_info()}
        return obs, reward, done, False, extraInfo

    def __init__(self, targetName, ns3Path, ns3Settings=None, shmSize=4096):
        if self._created:
            raise Exception('Error: Ns3Env is singleton')
        self._created = True
        self.exp = Experiment(targetName, ns3Path, py_binding, shmSize=shmSize)
        self.ns3Settings = ns3Settings

        self.newStateRx = False
        self.obsData = None
        self.reward = 0
        self.gameOver = False
        self.gameOverReason = None
        self.extraInfo = None

        self.msgInterface = self.exp.run(setting=self.ns3Settings, show_output=True)
        self.initialize_env()
        # get first observations
        self.rx_env_state()
        self.envDirty = False

    def step(self, actions):
        self.send_actions(actions)
        self.rx_env_state()
        self.envDirty = True
        return self.get_state()

    def reset(self, seed=None, options=None):
        if not self.envDirty:
            obs = self.get_obs()
            return obs, {}

        # not using self.exp.kill() here in order for semaphores to reset to initial state
        if not self.gameOver:
            self.rx_env_state()
            self.send_close_command()

        self.msgInterface = None
        self.newStateRx = False
        self.obsData = None
        self.reward = 0
        self.gameOver = False
        self.gameOverReason = None
        self.extraInfo = None

        self.msgInterface = self.exp.run(show_output=True)
        self.initialize_env()
        # get first observations
        self.rx_env_state()
        self.envDirty = False

        obs = self.get_obs()
        return obs, {}

    def render(self, mode='human'):
        return

    def get_random_action(self):
        act = self.action_space.sample()
        return act

    def close(self):
        # environment is not needed anymore, so kill subprocess in a straightforward way
        self.exp.kill()
        # destroy the message interface and its shared memory segment
        del self.exp
