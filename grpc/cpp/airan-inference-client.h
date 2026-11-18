/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NS3_AI_NTN_AIRAN_INFERENCE_CLIENT_H
#define NS3_AI_NTN_AIRAN_INFERENCE_CLIENT_H

// Client-side wrapper for the AI-RAN inference contract.
//
// ns-3 code holds an AiranInferenceClient bound to one
// InferenceChannel. Typical lifecycle:
//
//   1. Construct, then Attach(channel)
//   2. Call SubmitPrecoderRequest() / SubmitBeamRequest() from a
//      simulator event. Each call returns a request_id immediately
//      and registers a CompletionCallback to fire on response.
//   3. Schedule Poll(short_timeout_ms) on a recurring tick — the
//      client drains its inbound queue and dispatches completions.
//   4. Close() on shutdown.
//
// The client is non-thread-safe by design: ns-3 simulator events
// always run on a single thread.

#include "airan-messages.h"
#include "inference-channel.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ns3
{
namespace oranntn
{
namespace airan
{

class AiranInferenceClient
{
  public:
    using CompletionCallback =
        std::function<void(const InferenceResponse& resp)>;

    AiranInferenceClient();
    ~AiranInferenceClient();

    /// Adopt an open channel.
    void Attach(std::unique_ptr<InferenceChannel> ch);
    bool IsAttached() const { return m_ch && m_ch->IsOpen(); }

    /// Submit a CSI-driven precoder inference request. Returns the
    /// generated request_id (>0 on success, 0 on failure).
    uint64_t SubmitPrecoderRequest(const std::string& model_name,
                                    uint64_t ue_id,
                                    uint64_t nr_cgi,
                                    double sim_time_s,
                                    const CsiTensor& csi,
                                    CompletionCallback cb);

    /// Submit an RSRP-driven beam-classification request.
    uint64_t SubmitBeamRequest(const std::string& model_name,
                                uint64_t ue_id,
                                uint64_t nr_cgi,
                                double sim_time_s,
                                const RsrpVector& rsrp,
                                CompletionCallback cb);

    /// Drain the inbound side, dispatching matched CompletionCallbacks.
    /// Returns the number of responses dispatched.
    size_t Poll(uint32_t timeout_ms, uint32_t max_msgs = 16);

    /// Outstanding (sent, no response yet) request count.
    size_t PendingCount() const { return m_pending.size(); }

    /// Total requests submitted so far.
    uint64_t TotalSent() const { return m_totalSent; }
    /// Total responses dispatched so far.
    uint64_t TotalRecv() const { return m_totalRecv; }
    /// Number of responses dropped because request_id had no
    /// outstanding callback.
    uint64_t TotalOrphan() const { return m_totalOrphan; }

    /// Frames dropped because they did not decode as a response.
    uint64_t TotalUndecodable() const { return m_totalUndecodable; }

    /// Generated request_id counter (next id that will be assigned).
    uint64_t NextRequestId() const { return m_nextRequestId; }

    void Close();

  private:
    uint64_t SubmitInternal(const InferenceRequest& req,
                             CompletionCallback cb);

    std::unique_ptr<InferenceChannel> m_ch;
    std::map<uint64_t, CompletionCallback> m_pending;
    uint64_t m_nextRequestId{1};
    uint64_t m_totalSent{0};
    uint64_t m_totalRecv{0};
    uint64_t m_totalOrphan{0};
    uint64_t m_totalUndecodable{0};
};

} // namespace airan
} // namespace oranntn
} // namespace ns3

#endif // NS3_AI_NTN_AIRAN_INFERENCE_CLIENT_H
