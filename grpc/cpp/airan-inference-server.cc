/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#include "airan-inference-server.h"

#include "inference-channel-tcp.h"

namespace ns3
{
namespace oranntn
{
namespace airan
{

AiranInferenceServer::AiranInferenceServer() = default;
AiranInferenceServer::~AiranInferenceServer()
{
    Stop();
}

bool
AiranInferenceServer::RegisterModel(const TritonModelConfig& cfg,
                                     ModelHandler handler)
{
    if (cfg.name.empty() || !handler)
    {
        return false;
    }
    if (cfg.output_field == OutputField::unknown)
    {
        return false;
    }
    ModelEntry e{cfg, std::move(handler)};
    m_models[cfg.name] = std::move(e);
    return true;
}

void
AiranInferenceServer::AddChannel(std::unique_ptr<InferenceChannel> ch)
{
    if (ch)
    {
        m_channels.push_back(std::move(ch));
    }
}

bool
AiranInferenceServer::StartTcpListener(const std::string& host,
                                        uint16_t port)
{
    m_listener = std::make_unique<TcpInferenceListener>();
    if (!m_listener->Listen(host, port))
    {
        m_listener.reset();
        return false;
    }
    return true;
}

uint16_t
AiranInferenceServer::LocalTcpPort() const
{
    return m_listener ? m_listener->LocalPort() : 0;
}

size_t
AiranInferenceServer::Poll(uint32_t timeout_ms,
                             uint32_t max_msgs_per_channel)
{
    size_t handled = 0;
    if (m_listener && m_listener->IsOpen())
    {
        auto next = m_listener->AcceptOne(0);
        while (next)
        {
            m_channels.push_back(std::move(next));
            next = m_listener->AcceptOne(0);
        }
    }
    for (auto it = m_channels.begin(); it != m_channels.end();)
    {
        InferenceChannel& ch = **it;
        bool drained_ok = true;
        for (uint32_t k = 0; k < max_msgs_per_channel; ++k)
        {
            std::vector<uint8_t> bytes;
            if (!ch.TryRecv(bytes, timeout_ms))
            {
                break;
            }
            if (!DispatchOne(ch, bytes))
            {
                drained_ok = false;
                break;
            }
            ++handled;
        }
        if (!drained_ok || !ch.IsOpen())
        {
            it = m_channels.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return handled;
}

bool
AiranInferenceServer::DispatchOne(InferenceChannel& ch,
                                    const std::vector<uint8_t>& bytes)
{
    InferenceRequest req;
    if (!AiranMessageCodec::DecodeRequest(bytes, req))
    {
        ++m_rejected;
        return false;
    }
    const auto it = m_models.find(req.model_name);
    InferenceResponse resp;
    resp.request_id = req.request_id;
    if (it == m_models.end())
    {
        resp.status = 1;
        resp.status_message = "unknown model";
        ++m_rejected;
    }
    else
    {
        // Cross-check the input flavour against the model's
        // declared output kind.
        const OutputField wanted = it->second.cfg.output_field;
        if (wanted == OutputField::precoder &&
            req.input_kind != InferenceRequest::InputKind::csi)
        {
            resp.status = 2;
            resp.status_message = "model precoder requires CSI input";
            ++m_rejected;
        }
        else if (wanted == OutputField::beam &&
                 req.input_kind != InferenceRequest::InputKind::rsrp)
        {
            resp.status = 2;
            resp.status_message = "model beam requires RSRP input";
            ++m_rejected;
        }
        else
        {
            it->second.handler(req, resp);
            ++m_requests;
            m_latencyMsSum += resp.inference_latency_ms;
            if (resp.status != 0)
            {
                ++m_modelRefused;
            }
        }
    }
    const auto out = AiranMessageCodec::EncodeResponse(resp);
    return ch.Send(out);
}

const TritonModelConfig*
AiranInferenceServer::GetConfig(const std::string& name) const
{
    const auto it = m_models.find(name);
    return (it == m_models.end()) ? nullptr : &it->second.cfg;
}

void
AiranInferenceServer::Stop()
{
    for (auto& ch : m_channels)
    {
        if (ch)
        {
            ch->Close();
        }
    }
    m_channels.clear();
    if (m_listener)
    {
        m_listener->Close();
        m_listener.reset();
    }
}

} // namespace airan
} // namespace oranntn
} // namespace ns3
