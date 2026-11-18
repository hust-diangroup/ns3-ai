/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#include "airan-inference-client.h"

namespace ns3
{
namespace oranntn
{
namespace airan
{

AiranInferenceClient::AiranInferenceClient() = default;

AiranInferenceClient::~AiranInferenceClient()
{
    Close();
}

void
AiranInferenceClient::Attach(std::unique_ptr<InferenceChannel> ch)
{
    Close();
    m_ch = std::move(ch);
}

uint64_t
AiranInferenceClient::SubmitInternal(const InferenceRequest& req,
                                      CompletionCallback cb)
{
    if (!m_ch || !m_ch->IsOpen())
    {
        return 0;
    }
    if (m_pending.count(req.request_id))
    {
        return 0;
    }
    const auto bytes = AiranMessageCodec::EncodeRequest(req);
    if (!m_ch->Send(bytes))
    {
        return 0;
    }
    m_pending[req.request_id] = std::move(cb);
    ++m_totalSent;
    return req.request_id;
}

uint64_t
AiranInferenceClient::SubmitPrecoderRequest(
    const std::string& model_name,
    uint64_t ue_id,
    uint64_t nr_cgi,
    double sim_time_s,
    const CsiTensor& csi,
    CompletionCallback cb)
{
    InferenceRequest req;
    req.request_id = m_nextRequestId++;
    req.model_name = model_name;
    req.ue_id = ue_id;
    req.nr_cgi = nr_cgi;
    req.sim_time_s = sim_time_s;
    req.input_kind = InferenceRequest::InputKind::csi;
    req.csi = csi;
    return SubmitInternal(req, std::move(cb));
}

uint64_t
AiranInferenceClient::SubmitBeamRequest(const std::string& model_name,
                                         uint64_t ue_id,
                                         uint64_t nr_cgi,
                                         double sim_time_s,
                                         const RsrpVector& rsrp,
                                         CompletionCallback cb)
{
    InferenceRequest req;
    req.request_id = m_nextRequestId++;
    req.model_name = model_name;
    req.ue_id = ue_id;
    req.nr_cgi = nr_cgi;
    req.sim_time_s = sim_time_s;
    req.input_kind = InferenceRequest::InputKind::rsrp;
    req.rsrp = rsrp;
    return SubmitInternal(req, std::move(cb));
}

size_t
AiranInferenceClient::Poll(uint32_t timeout_ms, uint32_t max_msgs)
{
    if (!m_ch)
    {
        return 0;
    }
    size_t dispatched = 0;
    for (uint32_t k = 0; k < max_msgs; ++k)
    {
        std::vector<uint8_t> bytes;
        if (!m_ch->TryRecv(bytes, timeout_ms))
        {
            break;
        }
        InferenceResponse resp;
        if (!AiranMessageCodec::DecodeResponse(bytes, resp))
        {
            ++m_totalUndecodable; // was dropped without a trace
            continue;
        }
        const auto it = m_pending.find(resp.request_id);
        if (it == m_pending.end())
        {
            ++m_totalOrphan;
            continue;
        }
        // TotalRecv() is documented as responses DISPATCHED; it used to be
        // incremented before the orphan check, so orphans were counted twice
        // (as received and as orphans) and Recv + Orphan exceeded the frames.
        ++m_totalRecv;
        auto cb = std::move(it->second);
        m_pending.erase(it);
        if (cb)
        {
            cb(resp);
        }
        ++dispatched;
    }
    return dispatched;
}

void
AiranInferenceClient::Close()
{
    if (m_ch)
    {
        m_ch->Close();
        m_ch.reset();
    }
    m_pending.clear();
}

} // namespace airan
} // namespace oranntn
} // namespace ns3
