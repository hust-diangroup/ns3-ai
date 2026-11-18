/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#ifndef NS3_AI_NTN_AIRAN_INFERENCE_SERVER_H
#define NS3_AI_NTN_AIRAN_INFERENCE_SERVER_H

// Server-side dispatcher for the AI-RAN inference contract.
//
// Holds:
//   * a map of model_name -> handler that returns an InferenceResponse
//     for a given InferenceRequest (the registered "inference model")
//   * a TritonModelConfig per registered model — used to validate the
//     request shape and to enforce the inference latency budget
//   * one or more InferenceChannel endpoints (in-process or TCP)
//
// Pumping the server (Poll()) drains each channel, deserialises the
// inbound request, dispatches it to the registered handler, then
// serialises the response back. Pump cadence is set by the caller via
// Simulator::Schedule.

#include "airan-messages.h"
#include "inference-channel.h"
#include "triton-model-config.h"

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

class TcpInferenceListener;

class AiranInferenceServer
{
  public:
    /// Pure-function handler signature. Handlers must populate `out`
    /// with the response that should travel back to the client. Any
    /// `output_kind` mismatch with the model's TritonModelConfig is
    /// fatal — the request will be rejected with status=2.
    using ModelHandler =
        std::function<void(const InferenceRequest& req,
                            InferenceResponse& out)>;

    AiranInferenceServer();
    ~AiranInferenceServer();

    /// Register a model and its handler. The TritonModelConfig is
    /// stored verbatim and made available via GetConfig().
    bool RegisterModel(const TritonModelConfig& cfg,
                       ModelHandler handler);

    /// Adopt an inbound channel. The server takes ownership and pumps
    /// it on every Poll().
    void AddChannel(std::unique_ptr<InferenceChannel> ch);

    /// Open a TCP listener — every Poll() will also AcceptOne() with a
    /// short timeout so new clients are picked up between drain ticks.
    bool StartTcpListener(const std::string& host, uint16_t port);

    /// Resolved listening port (0 if no listener active).
    uint16_t LocalTcpPort() const;

    /// One pump cycle: accept queued clients, drain frames from each
    /// channel, dispatch + reply. Returns the number of requests
    /// handled in this call.
    size_t Poll(uint32_t timeout_ms,
                uint32_t max_msgs_per_channel = 16);

    /// Channel count at this instant (excludes the listener itself).
    size_t NumChannels() const { return m_channels.size(); }

    uint64_t RequestsHandled() const { return m_requests; }
    uint64_t RequestsRejected() const { return m_rejected; }
    /// Sum of the reported inference latencies (ms). A double: the mock
    /// models report 0.25 to 0.45 ms, which the former uint64_t accumulator
    /// truncated to 0 on every request.
    double InferenceLatencySumMs() const { return m_latencyMsSum; }
    /// Requests a model handler declined (response status != 0, e.g. a CSI
    /// tensor that does not match the declared shape). They are part of
    /// RequestsHandled(), since the handler ran, and are counted here as well.
    uint64_t RequestsRefusedByModel() const { return m_modelRefused; }

    /// Look up a registered model's TritonModelConfig (nullptr if
    /// unknown). Tests use this to assert what was loaded.
    const TritonModelConfig* GetConfig(const std::string& name) const;

    void Stop();

  private:
    struct ModelEntry
    {
        TritonModelConfig cfg;
        ModelHandler handler;
    };

    bool DispatchOne(InferenceChannel& ch,
                     const std::vector<uint8_t>& bytes);

    std::map<std::string, ModelEntry> m_models;
    std::vector<std::unique_ptr<InferenceChannel>> m_channels;
    std::unique_ptr<TcpInferenceListener> m_listener;

    uint64_t m_requests{0};
    uint64_t m_rejected{0};
    double m_latencyMsSum{0.0};
    uint64_t m_modelRefused{0};
};

} // namespace airan
} // namespace oranntn
} // namespace ns3

#endif // NS3_AI_NTN_AIRAN_INFERENCE_SERVER_H
