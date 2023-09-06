/*
 * Copyright (c) 2023 Huazhong University of Science and Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Muyuan Shen <muyuan_shen@hust.edu.cn>
 */

#ifndef NS3_RLTCP_AGENT_H
#define NS3_RLTCP_AGENT_H

#include <cmath>
#include <random>
#include <torch/torch.h>
#include <tuple>
#include <vector>

#define REPLAY_LENGTH 2000
#define BATCH_SIZE 32
#define OBS_SHAPE 5
#define ACTION_NUM 4
#define LEARNING_RATE 0.0001

class NetImpl : public torch::nn::Module
{
  public:
    NetImpl(int in, int out)
        : in_features(in),
          out_features(out),
          layers(torch::nn::Linear(in_features, 20),
                 torch::nn::Linear(20, 20),
                 torch::nn::Linear(20, out_features))
    {
        register_module("layers", layers);
    }

    torch::Tensor forward(torch::Tensor x)
    {
        return layers->forward(x);
    }

  private:
    int in_features, out_features;
    torch::nn::Sequential layers;
};

TORCH_MODULE(Net);

struct Transition
{
    std::array<float, OBS_SHAPE> state;
    int64_t action;
    std::array<float, OBS_SHAPE> next_state;
    int64_t reward;
};

class ReplayMemory
{
  public:
    ReplayMemory()
        : capacity(REPLAY_LENGTH),
          rng(std::random_device()())
    {
    }

    void Add(Transition& experience)
    {
        if (memory.size() >= capacity)
        {
            memory.erase(memory.begin());
        }
        memory.push_back(experience);
    }

    void Sample(std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor>& sample)
    {
        // these should be static because torch::from_blob does not take ownership
        static std::array<float, OBS_SHAPE * BATCH_SIZE> states;
        static std::array<int64_t, BATCH_SIZE> actions;
        static std::array<float, OBS_SHAPE * BATCH_SIZE> next_states;
        static std::array<int64_t, BATCH_SIZE> rewards;

        // get sampled batch
        std::uniform_int_distribution<> randomIndex(0, memory.size() - 1);
        for (uint32_t i = 0; i < BATCH_SIZE; ++i)
        {
            uint32_t index = randomIndex(rng);
            actions[i] = memory[index].action;
            rewards[i] = memory[index].reward;
            uint32_t state_base = OBS_SHAPE * i;
            for (uint32_t j = 0; j < OBS_SHAPE; ++j)
            {
                states[state_base + j] = memory[index].state[j];
                next_states[state_base + j] = memory[index].next_state[j];
            }
        }

        // save samples to tuple
        std::get<0>(sample) = torch::from_blob(states.data(), {BATCH_SIZE, OBS_SHAPE}, at::kFloat);
        std::get<1>(sample) = torch::from_blob(actions.data(), {BATCH_SIZE, 1}, at::kLong);
        std::get<2>(sample) =
            torch::from_blob(next_states.data(), {BATCH_SIZE, OBS_SHAPE}, at::kFloat);
        std::get<3>(sample) = torch::from_blob(rewards.data(), {BATCH_SIZE, 1}, at::kLong);
    }

    const uint32_t capacity;

  private:
    std::vector<Transition> memory;
    std::default_random_engine rng;
};

class DQN
{
  public:
    DQN()
        : memory_counter(0),
          policy_net(OBS_SHAPE, ACTION_NUM),
          target_net(OBS_SHAPE, ACTION_NUM),
          step(0),
          target_update_interval(100),
          memory(),
          rng(std::random_device()()),
          dist(0.0, 1.0),
          optim(policy_net->parameters(), torch::optim::AdamOptions(LEARNING_RATE)),
          loss_model(torch::nn::MSELossOptions(torch::kMean))
    {
    }

    uint32_t ChooseAction(std::array<float, OBS_SHAPE> obs)
    {
        torch::Tensor x = torch::from_blob(obs.data(), {OBS_SHAPE});
        torch::Tensor q_value;
        uint32_t action;
        if (dist(rng) > pow(0.99, memory.capacity))
        {
            q_value = policy_net->forward(x);
            action = torch::argmax(q_value, 0).item().toInt();
        }
        else
        {
            action = std::floor(dist(rng) * ACTION_NUM);
        }
        return action;
    }

    void SaveTransition(Transition& trans)
    {
        memory.Add(trans);
        memory_counter += 1;
    }

    void OptimizeModel()
    {
        static std::tuple<torch::Tensor, torch::Tensor, torch::Tensor, torch::Tensor> sample;
        step += 1;
        if (step % target_update_interval == 0)
        {
            std::stringstream stream;
            torch::save(policy_net, stream);
            torch::load(target_net, stream);
        }

        memory.Sample(sample);
        auto& s = std::get<0>(sample);
        auto& a = std::get<1>(sample);
        auto& s_ = std::get<2>(sample);
        auto& r = std::get<3>(sample);
        auto q_eval = policy_net->forward(s).gather(1, a);
        auto q_next = target_net->forward(s_).detach();
        auto q_target = r + 0.8 * std::get<0>(q_next.max(1, true));

        auto loss = loss_model(q_eval, q_target);
        optim.zero_grad();
        loss.backward();
        optim.step();
    }

    uint32_t memory_counter;

  private:
    Net policy_net;
    Net target_net;
    uint32_t step;
    uint32_t target_update_interval;
    ReplayMemory memory;
    std::default_random_engine rng;
    std::uniform_real_distribution<double> dist;
    torch::optim::Adam optim;
    torch::nn::MSELoss loss_model;
};

class TcpDeepQAgent
{
  public:
    TcpDeepQAgent()
        : dqn()
    {
    }

    std::tuple<uint32_t, uint32_t> GetAction(float ssThresh,
                                             float cWnd,
                                             float segmentsAcked,
                                             float segmentSize,
                                             float bytesInFlight)
    {
        trans.state = trans.next_state;
        trans.next_state = {ssThresh, cWnd, segmentsAcked, segmentSize, bytesInFlight};

        // update model
        if (trans.state[3] != 0) // not the first time calling GetAction
        {
            trans.reward = segmentsAcked - bytesInFlight - cWnd;
            dqn.SaveTransition(trans);
            if (dqn.memory_counter > REPLAY_LENGTH)
            {
                dqn.OptimizeModel();
            }
        }

        // choose action
        trans.action = dqn.ChooseAction(trans.next_state);
        auto& new_cWnd = std::get<0>(action_tup);
        auto& new_ssThresh = std::get<1>(action_tup);

        if (trans.action & 1)
        {
            new_cWnd = cWnd + segmentSize;
        }
        else if (cWnd > 0)
        {
            new_cWnd =
                cWnd + std::floor(std::max((double)1, (double)segmentSize * segmentSize / cWnd));
        }
        if (trans.action < 3)
        {
            new_ssThresh = 2 * segmentSize;
        }
        else
        {
            new_ssThresh = std::floor((double)bytesInFlight / 2);
        }

        return action_tup;
    }

  private:
    DQN dqn;
    Transition trans = {{0, 0, 0, 0, 0}, 0, {0, 0, 0, 0, 0}, 0};
    std::tuple<uint32_t, uint32_t> action_tup = {0, 0};
};

#endif // NS3_RLTCP_AGENT_H
