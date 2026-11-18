/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

// AI-RAN inference contract tests (Roadmap §3 T7).
//
// Coverage:
//   1. TritonModelConfigParser — pbtxt round-trip + edge cases
//   2. AiranMessageCodec      — request / response encode-decode for
//                                CSI and RSRP input flavours, plus
//                                malformed-frame rejection
//   3. InProcInferenceChannel — paired Send/Recv with deferred reads
//   4. TcpInferenceChannel    — round-trip over a real loopback
//                                socket (skipped if bind fails)
//   5. AiranInferenceServer + Client — round-trip via in-proc and
//      via TCP, under Simulator::Run() with periodic Poll() ticks
//   6. Failure modes — unknown model, mismatched input kind
//   7. Simulated workload — 60 s scenario, 100 alternating requests,
//      latency aggregation, no dropped frames

#include "ns3/airan-inference-client.h"
#include "ns3/airan-inference-server.h"
#include "ns3/airan-messages.h"
#include "ns3/airan-mock-runtime.h"
#include "ns3/inference-channel-inproc.h"
#include "ns3/inference-channel-tcp.h"
#include "ns3/triton-model-config.h"
#include <complex>

#include "ns3/constant-position-mobility-model.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/nr-epc-helper.h"
#include "ns3/ntn-real-stack-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <arpa/inet.h>
#include <atomic>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <sys/socket.h>
#include <fstream>
#include <memory>
#include <sstream>

using namespace ns3;
using namespace ns3::oranntn::airan;

namespace
{

const char* kPrecoderPbtxt = R"PBTXT(
# precoder model
name: "precoder_csi_to_weights"
platform: "onnxruntime_onnx"
max_batch_size: 32
default_model_filename: "model.onnx"
dynamic_batching {
  preferred_batch_size: [ 4, 8, 16 ]
  max_queue_delay_microseconds: 200
}
parameters {
  key: "INFERENCE_BUDGET_US"
  value: { string_value: "1000" }
}
parameters {
  key: "TOOLKIT_OUTPUT_FIELD"
  value: { string_value: "precoder" }
}
)PBTXT";

const char* kBeamPbtxt = R"PBTXT(
name: "beam_index_classifier"
platform: "tensorrt_plan"
max_batch_size: 64
parameters {
  key: "INFERENCE_BUDGET_US"
  value: { string_value: "500" }
}
parameters {
  key: "TOOLKIT_OUTPUT_FIELD"
  value: { string_value: "beam" }
}
)PBTXT";

TritonModelConfig
LoadPrecoderConfig()
{
    auto cfg = TritonModelConfigParser::Parse(kPrecoderPbtxt);
    return *cfg;
}

TritonModelConfig
LoadBeamConfig()
{
    auto cfg = TritonModelConfigParser::Parse(kBeamPbtxt);
    return *cfg;
}

CsiTensor
MakeFakeCsi(uint64_t seed)
{
    CsiTensor t;
    t.num_tx = 4;
    t.num_rx = 2;
    t.num_subcarriers = 8;
    t.doppler_hz = 100.0 + 0.5 * static_cast<double>(seed);
    t.values.reserve(2 * t.num_tx * t.num_rx * t.num_subcarriers);
    for (uint32_t i = 0; i < 2 * t.num_tx * t.num_rx * t.num_subcarriers;
         ++i)
    {
        t.values.push_back(static_cast<float>(0.01 * (i + seed)));
    }
    return t;
}

RsrpVector
MakeFakeRsrp(uint64_t seed)
{
    RsrpVector v;
    v.rsrp_dbm.reserve(8);
    v.beam_az_deg.reserve(8);
    v.beam_el_deg.reserve(8);
    for (uint32_t i = 0; i < 8; ++i)
    {
        v.rsrp_dbm.push_back(
            -90.0f + static_cast<float>((i * 7 + seed) % 30));
        v.beam_az_deg.push_back(static_cast<float>(i * 45));
        v.beam_el_deg.push_back(static_cast<float>(15 + i * 5));
    }
    return v;
}

} // namespace

// ----------------------------------------------------------------------------

class TritonModelConfigParserTest : public TestCase
{
  public:
    TritonModelConfigParserTest()
        : TestCase("Parse Triton model config (T7)")
    {
    }
    void DoRun() override
    {
        const auto p = TritonModelConfigParser::Parse(kPrecoderPbtxt);
        NS_TEST_ASSERT_MSG_EQ(p.has_value(), true, "parse precoder");
        NS_TEST_EXPECT_MSG_EQ(p->name,
                               "precoder_csi_to_weights",
                               "model name");
        NS_TEST_EXPECT_MSG_EQ(p->platform,
                               "onnxruntime_onnx",
                               "platform");
        NS_TEST_EXPECT_MSG_EQ(p->max_batch_size, 32u, "max batch");
        NS_TEST_EXPECT_MSG_EQ(p->default_model_filename,
                               "model.onnx",
                               "default model file");
        NS_TEST_ASSERT_MSG_EQ(p->max_queue_delay_us.has_value(),
                               true,
                               "queue delay parsed");
        NS_TEST_EXPECT_MSG_EQ(*p->max_queue_delay_us,
                               200u,
                               "queue delay value");
        const bool precoder_field =
            p->output_field == OutputField::precoder;
        NS_TEST_EXPECT_MSG_EQ(precoder_field,
                               true,
                               "precoder output field");
        NS_TEST_ASSERT_MSG_EQ(p->inference_budget_us.has_value(),
                               true,
                               "inference budget parsed");
        NS_TEST_EXPECT_MSG_EQ(*p->inference_budget_us,
                               1000u,
                               "inference budget value");

        const auto b = TritonModelConfigParser::Parse(kBeamPbtxt);
        NS_TEST_ASSERT_MSG_EQ(b.has_value(), true, "parse beam");
        const bool beam_field = b->output_field == OutputField::beam;
        NS_TEST_EXPECT_MSG_EQ(beam_field,
                               true,
                               "beam output field");

        // Edge cases: empty input, missing name → reject.
        NS_TEST_EXPECT_MSG_EQ(
            TritonModelConfigParser::Parse("").has_value(),
            false,
            "empty rejected");
        NS_TEST_EXPECT_MSG_EQ(TritonModelConfigParser::Parse(
                                   "platform: \"foo\"")
                                   .has_value(),
                               false,
                               "no-name rejected");
    }
};

// ----------------------------------------------------------------------------

class TritonModelConfigShippedFilesTest : public TestCase
{
  public:
    TritonModelConfigShippedFilesTest()
        : TestCase("Parse shipped Triton config.pbtxt files (T7)")
    {
    }
    void DoRun() override
    {
        // Resolve the shipped pbtxt files relative to this source.
        // We try a couple of standard relative paths so the test works
        // both during in-tree builds and from the install tree.
        const std::vector<std::string> roots = {
            "contrib/ns3-ai-ntn/grpc/triton/",
            "../contrib/ns3-ai-ntn/grpc/triton/",
            "../../contrib/ns3-ai-ntn/grpc/triton/",
        };
        bool found_precoder = false;
        bool found_beam = false;
        for (const auto& root : roots)
        {
            const auto p = TritonModelConfigParser::LoadFile(
                root + "precoder_csi_to_weights/config.pbtxt");
            if (p && p->name == "precoder_csi_to_weights")
            {
                found_precoder = true;
                const bool ok =
                    p->output_field == OutputField::precoder;
                NS_TEST_EXPECT_MSG_EQ(
                    ok,
                    true,
                    "shipped precoder pbtxt has precoder field");
            }
            const auto b = TritonModelConfigParser::LoadFile(
                root + "beam_index_classifier/config.pbtxt");
            if (b && b->name == "beam_index_classifier")
            {
                found_beam = true;
                const bool ok =
                    b->output_field == OutputField::beam;
                NS_TEST_EXPECT_MSG_EQ(
                    ok,
                    true,
                    "shipped beam pbtxt has beam field");
            }
        }
        // Soft-pass: shipped files are nice-to-have under in-tree
        // builds. The parser logic is exercised by the previous test
        // with embedded literals, so we don't fail the suite if the
        // files aren't reachable from this cwd.
        if (!found_precoder && !found_beam)
        {
            NS_LOG_UNCOND("Triton pbtxt files not in cwd, skipping");
        }
    }
};

// ----------------------------------------------------------------------------

class AiranMessageCodecRoundTripTest : public TestCase
{
  public:
    AiranMessageCodecRoundTripTest()
        : TestCase("Airan request-response codec round-trip (T7)")
    {
    }
    void DoRun() override
    {
        // CSI request round-trip
        InferenceRequest req;
        req.request_id = 42;
        req.ue_id = 0xdeadbeefcafebabeULL;
        req.nr_cgi = 7;
        req.model_name = "precoder_csi_to_weights";
        req.sim_time_s = 1.234;
        req.input_kind = InferenceRequest::InputKind::csi;
        req.csi = MakeFakeCsi(11);

        const auto bytes = AiranMessageCodec::EncodeRequest(req);
        NS_TEST_EXPECT_MSG_EQ(
            AiranMessageCodec::PeekKind(bytes), 0u, "kind=request");

        InferenceRequest decoded;
        NS_TEST_ASSERT_MSG_EQ(
            AiranMessageCodec::DecodeRequest(bytes, decoded),
            true,
            "decode csi request");
        NS_TEST_EXPECT_MSG_EQ(decoded.request_id, 42u, "rid");
        NS_TEST_EXPECT_MSG_EQ(decoded.ue_id,
                               0xdeadbeefcafebabeULL,
                               "ue id");
        const bool csi_kind =
            decoded.input_kind == InferenceRequest::InputKind::csi;
        NS_TEST_EXPECT_MSG_EQ(csi_kind,
                               true,
                               "kind is csi");
        NS_TEST_EXPECT_MSG_EQ(decoded.csi.num_tx, req.csi.num_tx, "tx");
        NS_TEST_EXPECT_MSG_EQ(decoded.csi.num_rx, req.csi.num_rx, "rx");
        NS_TEST_EXPECT_MSG_EQ(decoded.csi.num_subcarriers,
                               req.csi.num_subcarriers,
                               "sc");
        NS_TEST_ASSERT_MSG_EQ(decoded.csi.values.size(),
                               req.csi.values.size(),
                               "csi length");
        NS_TEST_EXPECT_MSG_EQ_TOL(decoded.csi.values.front(),
                                    req.csi.values.front(),
                                    1e-5,
                                    "csi front");
        NS_TEST_EXPECT_MSG_EQ_TOL(decoded.csi.values.back(),
                                    req.csi.values.back(),
                                    1e-5,
                                    "csi back");
        NS_TEST_EXPECT_MSG_EQ_TOL(decoded.csi.doppler_hz,
                                    req.csi.doppler_hz,
                                    1e-9,
                                    "doppler");

        // RSRP request round-trip
        InferenceRequest req2;
        req2.request_id = 99;
        req2.ue_id = 5;
        req2.nr_cgi = 12;
        req2.model_name = "beam_index_classifier";
        req2.input_kind = InferenceRequest::InputKind::rsrp;
        req2.rsrp = MakeFakeRsrp(3);
        const auto bytes2 = AiranMessageCodec::EncodeRequest(req2);
        InferenceRequest decoded2;
        NS_TEST_ASSERT_MSG_EQ(
            AiranMessageCodec::DecodeRequest(bytes2, decoded2),
            true,
            "decode rsrp request");
        const bool rsrp_kind =
            decoded2.input_kind == InferenceRequest::InputKind::rsrp;
        NS_TEST_EXPECT_MSG_EQ(rsrp_kind,
                               true,
                               "rsrp kind");
        NS_TEST_ASSERT_MSG_EQ(decoded2.rsrp.rsrp_dbm.size(),
                               req2.rsrp.rsrp_dbm.size(),
                               "rsrp size");
        NS_TEST_EXPECT_MSG_EQ_TOL(decoded2.rsrp.beam_el_deg.back(),
                                    req2.rsrp.beam_el_deg.back(),
                                    1e-5,
                                    "rsrp el last");

        // Response round-trip (precoder kind)
        InferenceResponse resp;
        resp.request_id = 42;
        resp.inference_latency_ms = 0.83;
        resp.status = 0;
        resp.status_message = "ok";
        resp.output_kind = InferenceResponse::OutputKind::precoder;
        resp.precoder.num_tx = 4;
        resp.precoder.num_layers = 4;
        resp.precoder.values = {0.1f,
                                  -0.2f,
                                  0.3f,
                                  -0.4f,
                                  0.5f,
                                  -0.6f,
                                  0.7f,
                                  -0.8f};
        const auto rbytes = AiranMessageCodec::EncodeResponse(resp);
        InferenceResponse rdec;
        NS_TEST_ASSERT_MSG_EQ(
            AiranMessageCodec::DecodeResponse(rbytes, rdec),
            true,
            "decode precoder response");
        const bool resp_precoder_kind =
            rdec.output_kind ==
            InferenceResponse::OutputKind::precoder;
        NS_TEST_EXPECT_MSG_EQ(resp_precoder_kind,
                               true,
                               "precoder kind round-trips");
        NS_TEST_EXPECT_MSG_EQ_TOL(rdec.inference_latency_ms,
                                    0.83,
                                    1e-9,
                                    "latency");
        NS_TEST_EXPECT_MSG_EQ_TOL(rdec.precoder.values[3],
                                    -0.4f,
                                    1e-5,
                                    "precoder value");

        // Malformed: wrong leading kind byte
        std::vector<uint8_t> bogus = bytes;
        bogus[0] = 0x77;
        InferenceRequest junk;
        NS_TEST_EXPECT_MSG_EQ(
            AiranMessageCodec::DecodeRequest(bogus, junk),
            false,
            "bogus kind rejected");
    }
};

// ----------------------------------------------------------------------------

class InProcChannelRoundTripTest : public TestCase
{
  public:
    InProcChannelRoundTripTest()
        : TestCase("InProc channel send-recv (T7)")
    {
    }
    void DoRun() override
    {
        auto pair = InProcInferenceChannel::CreatePair();
        auto& a = pair.first;
        auto& b = pair.second;
        NS_TEST_ASSERT_MSG_EQ(a->IsOpen(), true, "a open");
        NS_TEST_ASSERT_MSG_EQ(b->IsOpen(), true, "b open");

        const std::vector<uint8_t> hello = {1, 2, 3, 4, 5};
        NS_TEST_EXPECT_MSG_EQ(a->Send(hello), true, "a sent");
        std::vector<uint8_t> got;
        NS_TEST_EXPECT_MSG_EQ(b->TryRecv(got, 10),
                               true,
                               "b received");
        NS_TEST_EXPECT_MSG_EQ(got.size(), 5u, "size matches");
        NS_TEST_EXPECT_MSG_EQ(got[4], 5u, "last byte");

        // Receive on empty side should time out.
        NS_TEST_EXPECT_MSG_EQ(b->TryRecv(got, 2),
                               false,
                               "no more inbound");

        // Close on one side propagates: TryRecv returns false.
        a->Close();
        NS_TEST_EXPECT_MSG_EQ(b->TryRecv(got, 5),
                               false,
                               "closed side stops");
        const bool a_sent = a->BytesSent() >= 5;
        const bool b_recv = b->BytesRecv() >= 5;
        NS_TEST_EXPECT_MSG_EQ(a_sent, true, "counter");
        NS_TEST_EXPECT_MSG_EQ(b_recv, true, "counter b");
    }
};

// ----------------------------------------------------------------------------

class TcpChannelRoundTripTest : public TestCase
{
  public:
    TcpChannelRoundTripTest()
        : TestCase("TCP channel send-recv on loopback (T7)")
    {
    }
    void DoRun() override
    {
        TcpInferenceListener listener;
        const bool listening = listener.Listen("127.0.0.1", 0);
        if (!listening)
        {
            NS_LOG_UNCOND(
                "TCP listener could not bind, skipping test");
            return;
        }
        const uint16_t port = listener.LocalPort();
        NS_TEST_ASSERT_MSG_GT(port, 0u, "ephemeral port allocated");

        TcpInferenceChannel client;
        NS_TEST_ASSERT_MSG_EQ(client.ConnectTo("127.0.0.1", port),
                               true,
                               "connect");
        auto srv = listener.AcceptOne(200);
        NS_TEST_ASSERT_MSG_NE(srv.get(),
                               nullptr,
                               "server accepts");

        const std::vector<uint8_t> frame = {0xA0,
                                            0x01,
                                            0x02,
                                            0x03,
                                            0xFF};
        NS_TEST_EXPECT_MSG_EQ(client.Send(frame),
                               true,
                               "client sent");
        std::vector<uint8_t> got;
        NS_TEST_EXPECT_MSG_EQ(srv->TryRecv(got, 200),
                               true,
                               "server received");
        NS_TEST_ASSERT_MSG_EQ(got.size(), frame.size(), "size");
        NS_TEST_EXPECT_MSG_EQ(got.front(), 0xA0u, "first byte");
        NS_TEST_EXPECT_MSG_EQ(got.back(), 0xFFu, "last byte");

        // Round-trip in the other direction.
        const std::vector<uint8_t> rframe(33, 0x42);
        NS_TEST_EXPECT_MSG_EQ(srv->Send(rframe), true, "server sent");
        std::vector<uint8_t> rgot;
        NS_TEST_EXPECT_MSG_EQ(client.TryRecv(rgot, 200),
                               true,
                               "client received");
        NS_TEST_EXPECT_MSG_EQ(rgot.size(), 33u, "size 33");

        client.Close();
        srv->Close();
        listener.Close();
    }
};

// ----------------------------------------------------------------------------

class AiranInProcRoundTripTest : public TestCase
{
  public:
    AiranInProcRoundTripTest()
        : TestCase("Client+Server round-trip via in-proc channel (T7)")
    {
    }
    void DoRun() override
    {
        AiranInferenceServer server;
        NS_TEST_ASSERT_MSG_EQ(
            server.RegisterModel(LoadPrecoderConfig(),
                                  AiranMockRuntime::MakePrecoderHandler(
                                      /*num_layers=*/2,
                                      /*base_latency_ms=*/0.5)),
            true,
            "register precoder");
        NS_TEST_ASSERT_MSG_EQ(
            server.RegisterModel(LoadBeamConfig(),
                                  AiranMockRuntime::MakeBeamHandler(
                                      /*top_k=*/3,
                                      /*latency_ms=*/0.3)),
            true,
            "register beam");

        auto pair = InProcInferenceChannel::CreatePair();
        AiranInferenceClient client;
        client.Attach(std::move(pair.first));
        server.AddChannel(std::move(pair.second));

        bool got_precoder_resp = false;
        bool got_beam_resp = false;

        const auto rid1 = client.SubmitPrecoderRequest(
            "precoder_csi_to_weights",
            /*ue_id=*/100,
            /*nr_cgi=*/1,
            /*sim_time_s=*/0.0,
            MakeFakeCsi(5),
            [&](const InferenceResponse& resp) {
                got_precoder_resp = true;
                const bool ok = resp.output_kind ==
                                InferenceResponse::OutputKind::precoder;
                NS_TEST_EXPECT_MSG_EQ(
                    ok,
                    true,
                    "precoder response kind");
                NS_TEST_EXPECT_MSG_EQ(resp.status, 0u, "status ok");
                NS_TEST_EXPECT_MSG_EQ(resp.precoder.num_layers,
                                       2u,
                                       "two layers");
                NS_TEST_EXPECT_MSG_EQ(resp.precoder.values.size(),
                                       static_cast<size_t>(
                                           2 *
                                           resp.precoder.num_tx *
                                           resp.precoder.num_layers),
                                       "precoder value count");
            });
        NS_TEST_ASSERT_MSG_GT(rid1, 0u, "precoder rid");

        const auto rid2 = client.SubmitBeamRequest(
            "beam_index_classifier",
            /*ue_id=*/101,
            /*nr_cgi=*/2,
            /*sim_time_s=*/0.0,
            MakeFakeRsrp(13),
            [&](const InferenceResponse& resp) {
                got_beam_resp = true;
                const bool ok = resp.output_kind ==
                                InferenceResponse::OutputKind::beam;
                NS_TEST_EXPECT_MSG_EQ(
                    ok,
                    true,
                    "beam response kind");
                NS_TEST_EXPECT_MSG_EQ(resp.beam.scores_top_k.size(),
                                       3u,
                                       "top-3");
            });
        NS_TEST_ASSERT_MSG_GT(rid2, 0u, "beam rid");

        server.Poll(5);
        client.Poll(5);
        NS_TEST_EXPECT_MSG_EQ(got_precoder_resp,
                               true,
                               "precoder cb fired");
        NS_TEST_EXPECT_MSG_EQ(got_beam_resp,
                               true,
                               "beam cb fired");
        NS_TEST_EXPECT_MSG_EQ(client.PendingCount(),
                               0u,
                               "no pending");
        NS_TEST_EXPECT_MSG_EQ(server.RequestsHandled(),
                               2u,
                               "two handled");
        NS_TEST_EXPECT_MSG_EQ(server.RequestsRejected(),
                               0u,
                               "none rejected");
    }
};

// ----------------------------------------------------------------------------

class AiranTcpRoundTripTest : public TestCase
{
  public:
    AiranTcpRoundTripTest()
        : TestCase("Client+Server round-trip over TCP (T7)")
    {
    }
    void DoRun() override
    {
        AiranInferenceServer server;
        NS_TEST_ASSERT_MSG_EQ(
            server.RegisterModel(LoadBeamConfig(),
                                  AiranMockRuntime::MakeBeamHandler()),
            true,
            "register beam");
        if (!server.StartTcpListener("127.0.0.1", 0))
        {
            NS_LOG_UNCOND("TCP listener failed to bind, skip");
            return;
        }
        const uint16_t port = server.LocalTcpPort();
        NS_TEST_ASSERT_MSG_GT(port, 0u, "port assigned");

        auto cli_chan = std::make_unique<TcpInferenceChannel>();
        NS_TEST_ASSERT_MSG_EQ(
            cli_chan->ConnectTo("127.0.0.1", port),
            true,
            "client connect");

        AiranInferenceClient client;
        client.Attach(std::move(cli_chan));

        // Server pumps once to accept.
        server.Poll(10);
        NS_TEST_EXPECT_MSG_EQ(server.NumChannels(),
                               1u,
                               "one client");

        bool fired = false;
        InferenceResponse out;
        client.SubmitBeamRequest(
            "beam_index_classifier",
            7,
            3,
            0.5,
            MakeFakeRsrp(9),
            [&](const InferenceResponse& r) {
                fired = true;
                out = r;
            });

        server.Poll(50);
        client.Poll(50);
        NS_TEST_EXPECT_MSG_EQ(fired, true, "callback fired");
        NS_TEST_EXPECT_MSG_EQ(out.status, 0u, "status");
        const bool tcp_beam_kind =
            out.output_kind == InferenceResponse::OutputKind::beam;
        NS_TEST_EXPECT_MSG_EQ(
            tcp_beam_kind,
            true,
            "beam response");

        server.Stop();
        client.Close();
    }
};

// ----------------------------------------------------------------------------

class AiranFailureModesTest : public TestCase
{
  public:
    AiranFailureModesTest()
        : TestCase("Failure modes: unknown model + mismatched input (T7)")
    {
    }
    void DoRun() override
    {
        AiranInferenceServer server;
        server.RegisterModel(LoadPrecoderConfig(),
                              AiranMockRuntime::MakePrecoderHandler());

        auto pair = InProcInferenceChannel::CreatePair();
        AiranInferenceClient client;
        client.Attach(std::move(pair.first));
        server.AddChannel(std::move(pair.second));

        // 1. Unknown model
        InferenceResponse rA;
        bool cbA = false;
        client.SubmitPrecoderRequest(
            "no_such_model",
            1,
            1,
            0.0,
            MakeFakeCsi(1),
            [&](const InferenceResponse& r) {
                cbA = true;
                rA = r;
            });
        server.Poll(5);
        client.Poll(5);
        NS_TEST_EXPECT_MSG_EQ(cbA, true, "unknown-model cb fired");
        NS_TEST_EXPECT_MSG_EQ(rA.status, 1u, "unknown model status=1");

        // 2. Mismatched input kind (beam request to a precoder model
        //    — we send RSRP to the registered precoder model name)
        bool cbB = false;
        InferenceResponse rB;
        client.SubmitBeamRequest(
            "precoder_csi_to_weights",
            2,
            2,
            0.0,
            MakeFakeRsrp(1),
            [&](const InferenceResponse& r) {
                cbB = true;
                rB = r;
            });
        server.Poll(5);
        client.Poll(5);
        NS_TEST_EXPECT_MSG_EQ(cbB, true, "mismatch cb fired");
        NS_TEST_EXPECT_MSG_EQ(rB.status,
                               2u,
                               "mismatched-input status=2");

        NS_TEST_EXPECT_MSG_EQ(server.RequestsHandled(),
                               0u,
                               "none handled");
        NS_TEST_EXPECT_MSG_EQ(server.RequestsRejected(),
                               2u,
                               "two rejected");
    }
};

// ----------------------------------------------------------------------------
// End-to-end Simulator::Run() workload — 60 simulated seconds, periodic
// inference requests, drains via scheduled Poll() ticks.

class AiranSimulatorWorkloadTest : public TestCase
{
  public:
    AiranSimulatorWorkloadTest()
        : TestCase("End-to-end inference workload under Simulator::Run (T7)")
    {
    }
    void DoRun() override
    {
        auto server = std::make_shared<AiranInferenceServer>();
        server->RegisterModel(LoadPrecoderConfig(),
                               AiranMockRuntime::MakePrecoderHandler(
                                   /*num_layers=*/4,
                                   /*base_latency_ms=*/0.4));
        server->RegisterModel(LoadBeamConfig(),
                               AiranMockRuntime::MakeBeamHandler(
                                   /*top_k=*/4,
                                   /*latency_ms=*/0.25));

        auto pair = InProcInferenceChannel::CreatePair();
        auto client = std::make_shared<AiranInferenceClient>();
        client->Attach(std::move(pair.first));
        server->AddChannel(std::move(pair.second));

        std::atomic<uint64_t> precoder_responses{0};
        std::atomic<uint64_t> beam_responses{0};
        std::atomic<double> sum_latency_ms{0.0};

        const uint32_t kNumRequests = 100;
        // Schedule requests across t = [1 s, 50 s].
        for (uint32_t i = 0; i < kNumRequests; ++i)
        {
            const double when_s =
                1.0 +
                49.0 * (static_cast<double>(i) / kNumRequests);
            Simulator::Schedule(
                Seconds(when_s),
                [client, &precoder_responses, &beam_responses,
                 &sum_latency_ms, i, when_s] {
                    if (i % 2 == 0)
                    {
                        client->SubmitPrecoderRequest(
                            "precoder_csi_to_weights",
                            /*ue_id=*/i,
                            /*nr_cgi=*/1 + (i % 4),
                            when_s,
                            MakeFakeCsi(i),
                            [&](const InferenceResponse& resp) {
                                if (resp.status == 0)
                                {
                                    ++precoder_responses;
                                    sum_latency_ms.store(
                                        sum_latency_ms.load() +
                                        resp.inference_latency_ms);
                                }
                            });
                    }
                    else
                    {
                        client->SubmitBeamRequest(
                            "beam_index_classifier",
                            /*ue_id=*/i,
                            /*nr_cgi=*/1 + (i % 4),
                            when_s,
                            MakeFakeRsrp(i),
                            [&](const InferenceResponse& resp) {
                                if (resp.status == 0)
                                {
                                    ++beam_responses;
                                    sum_latency_ms.store(
                                        sum_latency_ms.load() +
                                        resp.inference_latency_ms);
                                }
                            });
                    }
                });
        }

        // Periodic pump ticks at 50 ms — server then client.
        for (uint32_t t_ms = 50; t_ms <= 60'000; t_ms += 50)
        {
            Simulator::Schedule(
                MilliSeconds(t_ms),
                [server, client] {
                    server->Poll(0, /*max=*/64);
                    client->Poll(0, /*max=*/64);
                });
        }
        Simulator::Stop(Seconds(60.0));
        Simulator::Run();
        // One final drain post-Run to flush in-flight frames the
        // periodic pump may have missed at the boundary.
        server->Poll(5, 64);
        client->Poll(5, 64);
        Simulator::Destroy();

        NS_TEST_EXPECT_MSG_EQ(precoder_responses.load(),
                               static_cast<uint64_t>(kNumRequests / 2),
                               "50 precoder responses");
        NS_TEST_EXPECT_MSG_EQ(beam_responses.load(),
                               static_cast<uint64_t>(kNumRequests / 2),
                               "50 beam responses");
        NS_TEST_EXPECT_MSG_EQ(client->PendingCount(),
                               0u,
                               "no pending at end");
        NS_TEST_EXPECT_MSG_EQ(client->TotalOrphan(),
                               0u,
                               "no orphans");
        const double avg_ms =
            sum_latency_ms.load() / kNumRequests;
        NS_TEST_EXPECT_MSG_GT(avg_ms, 0.0, "latency > 0");
        NS_TEST_EXPECT_MSG_LT(avg_ms, 5.0, "latency < 5 ms");
    }
};

// ----------------------------------------------------------------------------


/// AI-06: the precoder must be a function of the CHANNEL.
///
/// The mock computed cos/sin of `seed = req.request_id`, reading only
/// csi.num_tx. Two consequences, both bad: the same channel produced a
/// different beam on every call, and two completely different channels produced
/// the same beam if the counters lined up. No CSI influenced any output
/// anywhere in the toolkit, so a transposed or stale tensor was undetectable.
class MockPrecoderIsAFunctionOfCsiTest : public TestCase
{
  public:
    MockPrecoderIsAFunctionOfCsiTest()
        : TestCase("AI-06: the mock precoder depends on the CSI, not the request counter")
    {
    }

  private:
    static oranntn::airan::CsiTensor MakeCsi(uint32_t nTx, uint32_t nRx, uint32_t nSc,
                                            double phase)
    {
        oranntn::airan::CsiTensor csi;
        csi.num_tx = nTx;
        csi.num_rx = nRx;
        csi.num_subcarriers = nSc;
        csi.values.assign(csi.ExpectedSize(), 0.0f);
        for (uint32_t sc = 0; sc < nSc; ++sc)
        {
            for (uint32_t rx = 0; rx < nRx; ++rx)
            {
                for (uint32_t tx = 0; tx < nTx; ++tx)
                {
                    const double a = phase + 0.7 * tx + 0.31 * rx + 0.03 * sc;
                    const std::size_t o = csi.Index(sc, rx, tx);
                    csi.values[o] = static_cast<float>(std::cos(a));
                    csi.values[o + 1] = static_cast<float>(std::sin(a));
                }
            }
        }
        return csi;
    }

    static oranntn::airan::InferenceResponse Run(
        const oranntn::airan::CsiTensor& csi, uint64_t requestId, uint32_t layers,
        uint32_t expectedNumTx = 0)
    {
        auto handler = AiranMockRuntime::MakePrecoderHandler(layers, 0.4, expectedNumTx);
        oranntn::airan::InferenceRequest req;
        req.request_id = requestId;
        req.csi = csi;
        oranntn::airan::InferenceResponse out;
        handler(req, out);
        return out;
    }

    void DoRun() override
    {
        const uint32_t nTx = 4;
        const auto csiA = MakeCsi(nTx, 2, 8, 0.0);
        const auto csiB = MakeCsi(nTx, 2, 8, 1.1);

        // 1. SAME channel, DIFFERENT request id -> same precoder.
        //    This is the assertion the old kernel could never satisfy: its
        //    entire output was a function of the counter.
        const auto r1 = Run(csiA, 1, 2);
        const auto r2 = Run(csiA, 99999, 2);
        NS_TEST_ASSERT_MSG_EQ(r1.status, 0, "a valid tensor is accepted");
        NS_TEST_ASSERT_MSG_EQ(r1.precoder.values.size(), r2.precoder.values.size(),
                              "same shape");
        double maxDiff = 0.0;
        for (std::size_t i = 0; i < r1.precoder.values.size(); ++i)
        {
            maxDiff = std::max(maxDiff,
                               std::abs(static_cast<double>(r1.precoder.values[i]) -
                                        static_cast<double>(r2.precoder.values[i])));
        }
        NS_TEST_ASSERT_MSG_LT(maxDiff, 1e-9,
                              "the same channel must give the same beam whatever the request "
                              "counter says; a difference here means the counter is still in "
                              "the kernel");

        // 2. DIFFERENT channel, same request id -> different precoder.
        const auto r3 = Run(csiB, 1, 2);
        double maxChange = 0.0;
        for (std::size_t i = 0; i < r1.precoder.values.size(); ++i)
        {
            maxChange = std::max(maxChange,
                                 std::abs(static_cast<double>(r1.precoder.values[i]) -
                                          static_cast<double>(r3.precoder.values[i])));
        }
        NS_TEST_ASSERT_MSG_GT(maxChange, 1e-3,
                              "a different channel must give a different beam, or the CSI is "
                              "still being ignored");

        // 3. MRT property, checked against the closed form.
        //    Single layer, single rx: w = conj(hbar)/||hbar||, so h . w is real
        //    and equals ||hbar||. That is the definition of maximum-ratio
        //    transmission and a beam that does not satisfy it is not one.
        const auto csiS = MakeCsi(nTx, 1, 4, 0.4);
        const auto rs = Run(csiS, 7, 1);
        NS_TEST_ASSERT_MSG_EQ(rs.status, 0, "single-layer request accepted");

        std::vector<std::complex<double>> hbar(nTx, {0.0, 0.0});
        for (uint32_t sc = 0; sc < csiS.num_subcarriers; ++sc)
        {
            for (uint32_t tx = 0; tx < nTx; ++tx)
            {
                const std::size_t o = csiS.Index(sc, 0, tx);
                hbar[tx] += std::complex<double>(csiS.values[o], csiS.values[o + 1]);
            }
        }
        double hnorm = 0.0;
        for (auto& v : hbar)
        {
            v /= static_cast<double>(csiS.num_subcarriers);
            hnorm += std::norm(v);
        }
        hnorm = std::sqrt(hnorm);

        std::complex<double> gain{0.0, 0.0};
        double wnorm = 0.0;
        for (uint32_t tx = 0; tx < nTx; ++tx)
        {
            const std::complex<double> w(rs.precoder.values[2 * tx],
                                         rs.precoder.values[2 * tx + 1]);
            gain += hbar[tx] * w;
            wnorm += std::norm(w);
        }
        NS_TEST_ASSERT_MSG_EQ_TOL(std::sqrt(wnorm), 1.0, 1e-5,
                                  "the precoder is unit norm, so it does not invent power");
        NS_TEST_ASSERT_MSG_EQ_TOL(std::abs(gain), hnorm, 1e-5,
                                  "MRT achieves the full channel norm as array gain; a lower "
                                  "value means the weights are not matched to the channel");
        NS_TEST_ASSERT_MSG_LT(std::abs(gain.imag()), 1e-5,
                              "and the combined gain is real, which is what conjugating the "
                              "channel buys");

        // 4. A tensor that does not match its declared dimensions must be
        //    REFUSED. Silently beamforming on a mis-shaped buffer is how a
        //    layout error survives to the PHY.
        auto bad = csiA;
        bad.values.pop_back();
        const auto rbad = Run(bad, 3, 2);
        NS_TEST_ASSERT_MSG_NE(rbad.status, 0,
                              "a CSI buffer whose size disagrees with num_tx/num_rx/num_sc is "
                              "not usable and must be reported, not beamformed on");

        // Transposing num_tx with num_subcarriers leaves the total element
        // count unchanged, so a size check alone cannot see it. What catches it
        // is the model's DECLARED input shape, which is what a Triton
        // config.pbtxt carries and what a real deployment would enforce.
        auto swapped = csiA;
        std::swap(swapped.num_tx, swapped.num_subcarriers);
        NS_TEST_ASSERT_MSG_EQ(swapped.values.size(), swapped.ExpectedSize(),
                              "the transposed tensor is the same SIZE, which is precisely why "
                              "a size check cannot catch it");
        const auto rswapUnchecked = Run(swapped, 4, 2);
        NS_TEST_ASSERT_MSG_EQ(rswapUnchecked.status, 0,
                              "with no declared shape the transposition is accepted, and this "
                              "is recorded rather than papered over");
        const auto rswap = Run(swapped, 4, 2, /*expectedNumTx=*/nTx);
        NS_TEST_ASSERT_MSG_NE(rswap.status, 0,
                              "a model that declares its input shape rejects the transposition");
        // And the declared shape must not reject the CORRECT tensor.
        NS_TEST_ASSERT_MSG_EQ(Run(csiA, 5, 2, nTx).status, 0,
                              "the declared shape accepts a conforming tensor");
    }
};

// ===========================================================================
// In-depth cases (Nov 2026 test campaign): codec hardening, model conformance
// against closed-form references, server and client accounting, TCP framing
// under partial delivery and peer loss, overload, determinism, and one closed
// loop through the 5G-LENA stack (ntn-traffic) where the beam model chooses a
// real handover target from measured RSRP.
// ===========================================================================

namespace
{

InferenceRequest
MakeCsiRequest(uint64_t id, uint32_t nTx, uint32_t nRx, uint32_t nSc, double phase)
{
    InferenceRequest r;
    r.model_name = "precoder_csi_to_weights";
    r.request_id = id;
    r.ue_id = 42;
    r.nr_cgi = 0x123456789ULL;
    r.sim_time_s = 1.25;
    r.input_kind = InferenceRequest::InputKind::csi;
    r.csi.num_tx = nTx;
    r.csi.num_rx = nRx;
    r.csi.num_subcarriers = nSc;
    r.csi.doppler_hz = 4321.5;
    r.csi.values.assign(r.csi.ExpectedSize(), 0.0f);
    for (uint32_t sc = 0; sc < nSc; ++sc)
    {
        for (uint32_t rx = 0; rx < nRx; ++rx)
        {
            for (uint32_t tx = 0; tx < nTx; ++tx)
            {
                const double a = phase + 0.9 * tx + 0.37 * rx * (tx + 1) + 0.002 * sc;
                const std::size_t o = r.csi.Index(sc, rx, tx);
                r.csi.values[o] = static_cast<float>((1.0 + 0.1 * tx) * std::cos(a));
                r.csi.values[o + 1] = static_cast<float>((1.0 + 0.1 * tx) * std::sin(a));
            }
        }
    }
    return r;
}

/// Run one request through a fresh server over the in-proc channel.
InferenceResponse
Serve(AiranInferenceServer& server, const InferenceRequest& req)
{
    auto pair = InProcInferenceChannel::CreatePair();
    server.AddChannel(std::move(pair.second));
    pair.first->Send(AiranMessageCodec::EncodeRequest(req));
    server.Poll(0);
    std::vector<uint8_t> bytes;
    InferenceResponse resp;
    resp.status = 999;
    if (pair.first->TryRecv(bytes, 10))
    {
        AiranMessageCodec::DecodeResponse(bytes, resp);
    }
    return resp;
}

std::complex<double>
PrecW(const PrecoderWeights& p, uint32_t tx, uint32_t layer)
{
    const uint32_t off = 2 * (tx * p.num_layers + layer);
    return {p.values[off], p.values[off + 1]};
}

} // namespace

/// The tagged-binary codec under hostile input. Every strict prefix of a valid
/// frame must be rejected; a fixed-width field that declares the wrong length
/// must be rejected (it used to desynchronize the reader); a float array whose
/// declared length exceeds the frame must be rejected before any allocation;
/// unknown field ids are skipped (forward compatibility); and 3000 seeded
/// single-byte mutations never crash the decoder.
class AiranCodecHardeningTest : public TestCase
{
  public:
    AiranCodecHardeningTest()
        : TestCase("Codec hardening: truncation, wrong widths, huge lengths, unknown fields, fuzz")
    {
    }

  private:
    static void PatchU32(std::vector<uint8_t>& b, std::size_t at, uint32_t v)
    {
        for (int k = 0; k < 4; ++k)
        {
            b[at + k] = static_cast<uint8_t>((v >> (8 * (3 - k))) & 0xFF);
        }
    }

    void DoRun() override
    {
        const auto req = MakeCsiRequest(7, 4, 2, 6, 0.3);
        const auto frame = AiranMessageCodec::EncodeRequest(req);
        uint32_t acceptedPrefix = 0;
        for (std::size_t n = 0; n < frame.size(); ++n)
        {
            InferenceRequest out;
            std::vector<uint8_t> pre(frame.begin(), frame.begin() + n);
            acceptedPrefix += AiranMessageCodec::DecodeRequest(pre, out) ? 1 : 0;
        }
        NS_TEST_ASSERT_MSG_EQ(acceptedPrefix, 0u, "no strict prefix of a frame decodes");
        NS_TEST_ASSERT_MSG_EQ(+AiranMessageCodec::PeekKind({}), 0xFF, "empty frame kind");
        InferenceResponse asResp;
        NS_TEST_ASSERT_MSG_EQ(AiranMessageCodec::DecodeResponse(frame, asResp), false,
                              "a request frame is not a response");

        // Layout: kind(1) count(2) | fid(2) wt(1) len(4) payload ...
        // Field 1 is the model name (string); field 2 request_id (u64) follows.
        const std::size_t f1 = 3;
        const uint32_t nameLen = static_cast<uint32_t>(req.model_name.size());
        const std::size_t f2 = f1 + 7 + nameLen;
        NS_TEST_ASSERT_MSG_EQ(frame[f2 + 1], 2, "second field is request_id");
        {
            std::vector<uint8_t> bad = frame;
            PatchU32(bad, f2 + 3, 4); // u64 declared with length 4
            InferenceRequest out;
            NS_TEST_ASSERT_MSG_EQ(AiranMessageCodec::DecodeRequest(bad, out), false,
                                  "a u64 declared 4 bytes wide is rejected, not misread");
        }
        {
            // A packed-float field whose declared length runs past the frame.
            InferenceRequest small = MakeCsiRequest(8, 1, 1, 1, 0.0);
            auto csiOnly = AiranMessageCodec::EncodeRequest(small);
            // Find the CSI values sub-field (fid 11) and inflate its length.
            bool patched = false;
            for (std::size_t i = 0; i + 7 < csiOnly.size() && !patched; ++i)
            {
                if (csiOnly[i] == 0 && csiOnly[i + 1] == 11 && csiOnly[i + 2] == 6)
                {
                    PatchU32(csiOnly, i + 3, 0xFFFFFFF0u);
                    patched = true;
                }
            }
            NS_TEST_ASSERT_MSG_EQ(patched, true, "found the CSI values field");
            InferenceRequest out;
            NS_TEST_ASSERT_MSG_EQ(AiranMessageCodec::DecodeRequest(csiOnly, out), false,
                                  "a 4 GB float array in a short frame is rejected");
        }
        {
            // Unknown field (id 99, string) appended: skipped, rest intact.
            std::vector<uint8_t> ext = frame;
            const uint16_t count = static_cast<uint16_t>((ext[1] << 8) | ext[2]) + 1;
            ext[1] = static_cast<uint8_t>(count >> 8);
            ext[2] = static_cast<uint8_t>(count & 0xFF);
            const std::string extra = "future";
            ext.insert(ext.end(), {0, 99, 5});
            const uint32_t l = static_cast<uint32_t>(extra.size());
            ext.insert(ext.end(),
                       {static_cast<uint8_t>(l >> 24),
                        static_cast<uint8_t>(l >> 16),
                        static_cast<uint8_t>(l >> 8),
                        static_cast<uint8_t>(l)});
            ext.insert(ext.end(), extra.begin(), extra.end());
            InferenceRequest out;
            NS_TEST_ASSERT_MSG_EQ(AiranMessageCodec::DecodeRequest(ext, out), true,
                                  "unknown fields are skipped");
            NS_TEST_ASSERT_MSG_EQ(out.request_id, req.request_id, "known fields intact");
            NS_TEST_ASSERT_MSG_EQ(out.csi.values.size(), req.csi.values.size(), "CSI intact");
        }
        // Seeded fuzz: decode must return, never crash or hang.
        uint64_t lcg = 12345;
        uint32_t decoded = 0;
        for (int k = 0; k < 3000; ++k)
        {
            std::vector<uint8_t> m = frame;
            lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
            const std::size_t pos = (lcg >> 33) % m.size();
            m[pos] = static_cast<uint8_t>(lcg >> 13);
            InferenceRequest out;
            decoded += AiranMessageCodec::DecodeRequest(m, out) ? 1 : 0;
        }
        NS_TEST_ASSERT_MSG_LT(decoded, 3001u, "3000 mutated frames decoded or rejected safely");
    }
};

/// Codec fidelity at a realistic CSI size (16 Tx, 2 Rx, 273 subcarriers: a
/// 100 MHz carrier at 30 kHz SCS has 273 PRBs, TS 38.101-1 Table 5.3.2-1, here
/// one CSI sample per PRB): every float is carried bit for bit, including -0,
/// infinities, NaN payloads and denormals; 64-bit ids at their maximum;
/// doubles exact; a UTF-8 status message intact.
class AiranCodecFidelityTest : public TestCase
{
  public:
    AiranCodecFidelityTest()
        : TestCase("Codec fidelity: 16x2x273 CSI bit-exact incl. -0/inf/NaN/denormal, u64 max")
    {
    }

  private:
    void DoRun() override
    {
        auto req = MakeCsiRequest(std::numeric_limits<uint64_t>::max(), 16, 2, 273, 0.7);
        req.ue_id = std::numeric_limits<uint64_t>::max() - 1;
        req.sim_time_s = 1234.000000001;
        req.csi.values[0] = -0.0f;
        req.csi.values[1] = std::numeric_limits<float>::infinity();
        req.csi.values[2] = -std::numeric_limits<float>::infinity();
        req.csi.values[3] = std::numeric_limits<float>::quiet_NaN();
        req.csi.values[4] = std::numeric_limits<float>::denorm_min();
        const auto bytes = AiranMessageCodec::EncodeRequest(req);
        InferenceRequest out;
        NS_TEST_ASSERT_MSG_EQ(AiranMessageCodec::DecodeRequest(bytes, out), true, "decodes");
        NS_TEST_ASSERT_MSG_EQ(out.request_id, req.request_id, "u64 max id");
        NS_TEST_ASSERT_MSG_EQ(out.ue_id, req.ue_id, "ue id");
        NS_TEST_ASSERT_MSG_EQ(out.sim_time_s, req.sim_time_s, "double exact");
        NS_TEST_ASSERT_MSG_EQ(out.csi.LayoutValid(), true, "layout valid");
        NS_TEST_ASSERT_MSG_EQ(out.csi.values.size(), 2u * 16 * 2 * 273, "17472 floats");
        NS_TEST_ASSERT_MSG_EQ(std::memcmp(out.csi.values.data(),
                                          req.csi.values.data(),
                                          req.csi.values.size() * sizeof(float)),
                              0,
                              "every float bit for bit");
        NS_TEST_ASSERT_MSG_EQ(std::signbit(out.csi.values[0]), true, "-0 keeps its sign");

        InferenceResponse resp;
        resp.request_id = 9;
        resp.status = 1;
        resp.status_message = "CSI rejet\xC3\xA9 (\xCE\xBC=1)";
        resp.inference_latency_ms = 0.123456789;
        resp.output_kind = InferenceResponse::OutputKind::beam;
        resp.beam.best_beam_id = 63;
        resp.beam.score = -87.25f;
        resp.beam.scores_top_k = {-87.25f, -90.5f};
        InferenceResponse back;
        NS_TEST_ASSERT_MSG_EQ(
            AiranMessageCodec::DecodeResponse(AiranMessageCodec::EncodeResponse(resp), back),
            true,
            "response decodes");
        NS_TEST_ASSERT_MSG_EQ(back.status_message, resp.status_message, "UTF-8 intact");
        NS_TEST_ASSERT_MSG_EQ(back.inference_latency_ms, resp.inference_latency_ms, "exact");
        NS_TEST_ASSERT_MSG_EQ(back.beam.best_beam_id, 63u, "beam id");
        NS_TEST_ASSERT_MSG_EQ(back.beam.scores_top_k.size(), 2u, "top-k");
    }
};

/// The mock precoder against closed-form references. Maximum-ratio
/// transmission (Lo, IEEE Trans. Commun. 1999) on a rank-1 channel achieves
/// the Cauchy-Schwarz bound |h^T w| = ||h||; multi-layer precoders are
/// orthonormal up to the 1/sqrt(L) power split (unit total power); a rank-
/// deficient channel still yields orthogonal layers; and more layers than
/// transmit antennas is refused (TS 38.214 section 5.2.2.2: layers <= ports).
class AiranPrecoderConformanceTest : public TestCase
{
  public:
    AiranPrecoderConformanceTest()
        : TestCase("Precoder: MRT meets the Cauchy-Schwarz bound, orthonormal layers, rank limits")
    {
    }

  private:
    void DoRun() override
    {
        AiranInferenceServer server;
        server.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(1, 0.4));
        // Flat channel, one receive antenna: one CSI value per Tx.
        auto req = MakeCsiRequest(1, 8, 1, 4, 0.2);
        for (uint32_t sc = 1; sc < 4; ++sc)
        {
            for (uint32_t tx = 0; tx < 8; ++tx)
            {
                req.csi.values[req.csi.Index(sc, 0, tx)] = req.csi.values[req.csi.Index(0, 0, tx)];
                req.csi.values[req.csi.Index(sc, 0, tx) + 1] =
                    req.csi.values[req.csi.Index(0, 0, tx) + 1];
            }
        }
        const auto r = Serve(server, req);
        NS_TEST_ASSERT_MSG_EQ(r.status, 0u, "rank-1 precoder");
        std::complex<double> gain{0.0, 0.0};
        double hNorm2 = 0.0;
        double wNorm2 = 0.0;
        for (uint32_t tx = 0; tx < 8; ++tx)
        {
            const std::complex<double> h(req.csi.values[req.csi.Index(0, 0, tx)],
                                         req.csi.values[req.csi.Index(0, 0, tx) + 1]);
            gain += h * PrecW(r.precoder, tx, 0);
            hNorm2 += std::norm(h);
            wNorm2 += std::norm(PrecW(r.precoder, tx, 0));
        }
        NS_TEST_ASSERT_MSG_EQ_TOL(wNorm2, 1.0, 1e-5, "unit transmit power");
        NS_TEST_ASSERT_MSG_EQ_TOL(std::abs(gain), std::sqrt(hNorm2), 1e-4 * std::sqrt(hNorm2),
                                  "MRT array gain equals ||h|| (Cauchy-Schwarz bound)");

        // Four layers over 4 Tx / 2 Rx: orthonormal up to the 1/sqrt(4) split.
        AiranInferenceServer s4;
        s4.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(4, 0.4));
        const auto r4 = Serve(s4, MakeCsiRequest(2, 4, 2, 8, 1.0));
        NS_TEST_ASSERT_MSG_EQ(r4.status, 0u, "4-layer precoder");
        double total = 0.0;
        double worstCross = 0.0;
        for (uint32_t a = 0; a < 4; ++a)
        {
            for (uint32_t b = a; b < 4; ++b)
            {
                std::complex<double> ip{0.0, 0.0};
                for (uint32_t tx = 0; tx < 4; ++tx)
                {
                    ip += std::conj(PrecW(r4.precoder, tx, a)) * PrecW(r4.precoder, tx, b);
                }
                if (a == b)
                {
                    total += ip.real();
                    NS_TEST_ASSERT_MSG_EQ_TOL(ip.real(), 0.25, 1e-5, "each layer 1/L power");
                }
                else
                {
                    worstCross = std::max(worstCross, std::abs(ip));
                }
            }
        }
        NS_TEST_ASSERT_MSG_EQ_TOL(total, 1.0, 1e-5, "unit total power");
        NS_TEST_ASSERT_MSG_LT(worstCross, 1e-5, "layers mutually orthogonal (4 Tx, 2 Rx: two "
                                                "layers come from the rank-deficient fallback)");

        // Rank-deficient: both Rx rows identical, two layers on 3 Tx.
        AiranInferenceServer s2;
        s2.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(3, 0.4));
        auto dup = MakeCsiRequest(3, 3, 1, 2, 0.5);
        const auto rd = Serve(s2, dup);
        NS_TEST_ASSERT_MSG_EQ(rd.status, 0u, "rank-1 channel, 3 layers");
        double cross = 0.0;
        for (uint32_t a = 0; a < 3; ++a)
        {
            for (uint32_t b = a + 1; b < 3; ++b)
            {
                std::complex<double> ip{0.0, 0.0};
                for (uint32_t tx = 0; tx < 3; ++tx)
                {
                    ip += std::conj(PrecW(rd.precoder, tx, a)) * PrecW(rd.precoder, tx, b);
                }
                cross = std::max(cross, std::abs(ip));
            }
        }
        NS_TEST_ASSERT_MSG_LT(cross, 1e-5, "fallback layers are orthogonal to the MRT layer");

        // Failure path: 4 layers on 2 Tx.
        AiranInferenceServer sBad;
        sBad.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(4, 0.4));
        const auto rb = Serve(sBad, MakeCsiRequest(4, 2, 2, 4, 0.0));
        NS_TEST_ASSERT_MSG_EQ(rb.status, 1u, "4 layers on 2 Tx ports is refused");
        NS_TEST_ASSERT_MSG_EQ(sBad.RequestsRefusedByModel(), 1u, "and counted as a model refusal");
    }
};

/// The beam classifier: best beam = argmax RSRP (first on a tie), top-K sorted
/// descending and a prefix of the sorted input. Failure paths: an empty RSRP
/// vector, a NaN RSRP, and beam angles that do not match the RSRP vector are
/// refused (status 1) instead of answered.
class AiranBeamConformanceTest : public TestCase
{
  public:
    AiranBeamConformanceTest()
        : TestCase("Beam model: argmax, sorted top-K, tie-break; empty/NaN/mismatched refused")
    {
    }

  private:
    static InferenceRequest Beam(std::vector<float> rsrp, std::size_t nAngles)
    {
        InferenceRequest r;
        r.model_name = "beam_index_classifier";
        r.request_id = 1;
        r.input_kind = InferenceRequest::InputKind::rsrp;
        r.rsrp.rsrp_dbm = std::move(rsrp);
        r.rsrp.beam_az_deg.assign(nAngles, 0.0f);
        r.rsrp.beam_el_deg.assign(nAngles, 45.0f);
        return r;
    }

    void DoRun() override
    {
        AiranInferenceServer server;
        server.RegisterModel(LoadBeamConfig(), AiranMockRuntime::MakeBeamHandler(3, 0.25));
        const auto ok = Serve(server, Beam({-100.f, -85.f, -92.f, -85.f, -120.f}, 5));
        NS_TEST_ASSERT_MSG_EQ(ok.status, 0u, "served");
        NS_TEST_ASSERT_MSG_EQ(ok.beam.best_beam_id, 1u, "argmax, first of the tie");
        NS_TEST_ASSERT_MSG_EQ_TOL(ok.beam.score, -85.f, 1e-6, "score is the best RSRP");
        NS_TEST_ASSERT_MSG_EQ(ok.beam.scores_top_k.size(), 3u, "top-3");
        NS_TEST_ASSERT_MSG_EQ_TOL(ok.beam.scores_top_k[2], -92.f, 1e-6, "sorted descending");
        NS_TEST_ASSERT_MSG_EQ(Serve(server, Beam({-90.f}, 0)).status, 0u,
                              "angles may be omitted");

        const auto empty = Serve(server, Beam({}, 0));
        NS_TEST_ASSERT_MSG_EQ(empty.status, 1u, "empty RSRP refused, not answered beam 0");
        const auto nan = Serve(server, Beam({-90.f, std::nanf(""), -80.f}, 3));
        NS_TEST_ASSERT_MSG_EQ(nan.status, 1u, "NaN RSRP refused");
        const auto mis = Serve(server, Beam({-90.f, -80.f}, 3));
        NS_TEST_ASSERT_MSG_EQ(mis.status, 1u, "3 angles for 2 beams refused");
        NS_TEST_ASSERT_MSG_EQ(server.RequestsRefusedByModel(), 3u, "three model refusals");
    }
};

/// Server accounting. The latency sum must be the sum of the reported
/// latencies (it was a uint64_t that truncated the mock's 0.25 to 0.45 ms to
/// 0); handled + rejected equals what arrived; a malformed frame closes only
/// its own channel, and another client keeps being served.
class AiranServerAccountingTest : public TestCase
{
  public:
    AiranServerAccountingTest()
        : TestCase("Server accounting: exact latency sum, handled+rejected, bad frame isolates channel")
    {
    }

  private:
    void DoRun() override
    {
        AiranInferenceServer server;
        server.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(2, 0.4));
        server.RegisterModel(LoadBeamConfig(), AiranMockRuntime::MakeBeamHandler(4, 0.25));
        auto good = InProcInferenceChannel::CreatePair();
        auto evil = InProcInferenceChannel::CreatePair();
        server.AddChannel(std::move(good.second));
        server.AddChannel(std::move(evil.second));
        AiranInferenceClient client;
        client.Attach(std::move(good.first));
        double sumLat = 0.0;
        uint32_t ok = 0;
        for (uint32_t i = 0; i < 20; ++i)
        {
            auto cb = [&](const InferenceResponse& r) {
                sumLat += r.inference_latency_ms;
                ok += (r.status == 0) ? 1 : 0;
            };
            if (i % 2 == 0)
            {
                client.SubmitPrecoderRequest("precoder_csi_to_weights", i, 1, 0.0, MakeFakeCsi(i), cb);
            }
            else
            {
                client.SubmitBeamRequest("beam_index_classifier", i, 1, 0.0, MakeFakeRsrp(i), cb);
            }
        }
        client.SubmitBeamRequest("no_such_model", 99, 1, 0.0, MakeFakeRsrp(1),
                                 [&](const InferenceResponse& r) { ok += (r.status == 0); });
        evil.first->Send({0x00, 0xFF}); // truncated request frame
        server.Poll(0, 64);
        client.Poll(5, 64);
        NS_TEST_ASSERT_MSG_EQ(ok, 20u, "20 served");
        NS_TEST_ASSERT_MSG_EQ(server.RequestsHandled(), 20u, "20 handled");
        NS_TEST_ASSERT_MSG_EQ(server.RequestsRejected(), 2u, "unknown model + malformed frame");
        NS_TEST_ASSERT_MSG_EQ_TOL(server.InferenceLatencySumMs(), sumLat, 1e-9,
                                  "the latency sum is the sum of the reported latencies");
        NS_TEST_ASSERT_MSG_GT(server.InferenceLatencySumMs(), 20 * 0.25 - 1e-9,
                              "sub-millisecond latencies are not truncated to 0");
        NS_TEST_ASSERT_MSG_EQ(server.NumChannels(), 1u, "the malformed channel was dropped");
        // The good client is still served after the bad one was dropped.
        bool again = false;
        client.SubmitBeamRequest("beam_index_classifier", 100, 1, 0.0, MakeFakeRsrp(3),
                                 [&](const InferenceResponse& r) { again = (r.status == 0); });
        server.Poll(0, 64);
        client.Poll(5, 64);
        NS_TEST_ASSERT_MSG_EQ(again, true, "the healthy channel is unaffected");
        NS_TEST_ASSERT_MSG_EQ(client.PendingCount(), 0u, "nothing left pending");
    }
};

/// Client failure paths: no channel, closed channel, a response nobody asked
/// for (orphan), and unique monotonically increasing request ids.
class AiranClientFailurePathsTest : public TestCase
{
  public:
    AiranClientFailurePathsTest()
        : TestCase("Client failure paths: unattached, closed, orphan and undecodable responses, ids")
    {
    }

  private:
    void DoRun() override
    {
        AiranInferenceClient detached;
        NS_TEST_ASSERT_MSG_EQ(detached.SubmitBeamRequest("beam_index_classifier", 1, 1, 0.0,
                                                         MakeFakeRsrp(1), nullptr),
                              0u,
                              "no channel: request id 0");
        NS_TEST_ASSERT_MSG_EQ(detached.Poll(0), 0u, "nothing to poll");

        auto pair = InProcInferenceChannel::CreatePair();
        auto serverSide = std::move(pair.second);
        AiranInferenceClient client;
        client.Attach(std::move(pair.first));
        uint64_t last = 0;
        bool monotone = true;
        for (int i = 0; i < 50; ++i)
        {
            const uint64_t id = client.SubmitBeamRequest("beam_index_classifier", i, 1, 0.0,
                                                         MakeFakeRsrp(i), [](const InferenceResponse&) {});
            monotone = monotone && id > last;
            last = id;
        }
        NS_TEST_ASSERT_MSG_EQ(monotone, true, "ids strictly increase");
        NS_TEST_ASSERT_MSG_EQ(client.PendingCount(), 50u, "50 pending");
        // Orphan: a response for an id that was never issued.
        InferenceResponse orphan;
        orphan.request_id = 1000000;
        orphan.output_kind = InferenceResponse::OutputKind::beam;
        serverSide->Send(AiranMessageCodec::EncodeResponse(orphan));
        client.Poll(5);
        NS_TEST_ASSERT_MSG_EQ(client.TotalOrphan(), 1u, "orphan counted");
        NS_TEST_ASSERT_MSG_EQ(client.TotalRecv(), 0u, "and not dispatched");
        NS_TEST_ASSERT_MSG_EQ(client.PendingCount(), 50u, "pending untouched");
        serverSide->Send({0x01, 0x00}); // a truncated response frame
        client.Poll(5);
        NS_TEST_ASSERT_MSG_EQ(client.TotalUndecodable(), 1u, "an undecodable frame is counted");
        NS_TEST_ASSERT_MSG_EQ(client.PendingCount(), 50u, "and fulfils nothing");
        client.Close();
        NS_TEST_ASSERT_MSG_EQ(client.IsAttached(), false, "closed");
        NS_TEST_ASSERT_MSG_EQ(client.SubmitBeamRequest("beam_index_classifier", 1, 1, 0.0,
                                                       MakeFakeRsrp(1), nullptr),
                              0u,
                              "a closed client refuses to submit");
    }
};

/// TCP framing under partial delivery and peer loss. A frame written in two
/// halves with a poll in between must be reassembled (the old reader dropped
/// the first half and then parsed payload as a length header); several clients
/// with 70 kB CSI frames are each answered on their own connection; a frame
/// over the 16 MiB limit is refused by the sender; and when the server goes
/// away the client stops cleanly with no hang and no SIGPIPE.
class AiranTcpFramingAndPeerLossTest : public TestCase
{
  public:
    AiranTcpFramingAndPeerLossTest()
        : TestCase("TCP: split frames reassembled, 3 clients x 70 kB CSI, size limit, server loss")
    {
    }

  private:
    void DoRun() override
    {
        // 1. Split frame through a raw socket into a TcpInferenceChannel.
        TcpInferenceListener lis;
        if (!lis.Listen("127.0.0.1", 0))
        {
            NS_LOG_UNCOND("cannot bind loopback, skipping");
            return;
        }
        TcpInferenceChannel raw;
        NS_TEST_ASSERT_MSG_EQ(raw.ConnectTo("127.0.0.1", lis.LocalPort()), true, "connect");
        auto rx = lis.AcceptOne(500);
        NS_TEST_ASSERT_MSG_EQ((rx != nullptr), true, "accepted");
        const auto frame = AiranMessageCodec::EncodeRequest(MakeCsiRequest(5, 4, 2, 64, 0.1));
        std::vector<uint8_t> wire(4);
        const uint32_t lenBe = htonl(static_cast<uint32_t>(frame.size()));
        std::memcpy(wire.data(), &lenBe, 4);
        wire.insert(wire.end(), frame.begin(), frame.end());
        const std::size_t half = wire.size() / 2;
        NS_TEST_ASSERT_MSG_EQ(::send(raw.Fd(), wire.data(), half, MSG_NOSIGNAL),
                              static_cast<ssize_t>(half), "first half sent");
        std::vector<uint8_t> got;
        NS_TEST_ASSERT_MSG_EQ(rx->TryRecv(got, 20), false, "half a frame is not a frame");
        NS_TEST_ASSERT_MSG_GT(rx->StashedBytes(), 0u, "but its bytes are kept");
        NS_TEST_ASSERT_MSG_EQ(::send(raw.Fd(), wire.data() + half, wire.size() - half, MSG_NOSIGNAL),
                              static_cast<ssize_t>(wire.size() - half), "second half sent");
        NS_TEST_ASSERT_MSG_EQ(rx->TryRecv(got, 500), true, "the frame completes");
        NS_TEST_ASSERT_MSG_EQ((got == frame), true, "byte for byte");
        NS_TEST_ASSERT_MSG_EQ(rx->StashedBytes(), 0u, "nothing left over");
        NS_TEST_ASSERT_MSG_EQ(raw.Send(std::vector<uint8_t>(TcpInferenceChannel::kMaxFrameBytes + 1)),
                              false, "an oversized frame is refused by the sender");
        NS_TEST_ASSERT_MSG_EQ(raw.Send({}), false, "an empty frame too");
        NS_TEST_ASSERT_MSG_EQ(raw.IsOpen(), true, "and the channel stays open");

        // 2. Three clients, 70 kB CSI each, one server.
        AiranInferenceServer server;
        server.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(2, 0.4));
        NS_TEST_ASSERT_MSG_EQ(server.StartTcpListener("127.0.0.1", 0), true, "listen");
        std::vector<std::unique_ptr<AiranInferenceClient>> clients;
        std::vector<uint64_t> answered(3, 0);
        for (int c = 0; c < 3; ++c)
        {
            auto ch = std::make_unique<TcpInferenceChannel>();
            NS_TEST_ASSERT_MSG_EQ(ch->ConnectTo("127.0.0.1", server.LocalTcpPort()), true, "connect");
            clients.push_back(std::make_unique<AiranInferenceClient>());
            clients.back()->Attach(std::move(ch));
        }
        server.Poll(20);
        NS_TEST_ASSERT_MSG_EQ(server.NumChannels(), 3u, "three connections");
        for (int c = 0; c < 3; ++c)
        {
            const auto big = MakeCsiRequest(0, 16, 2, 273, 0.1 * c);
            clients[c]->SubmitPrecoderRequest("precoder_csi_to_weights", c, 1, 0.0, big.csi,
                                              [&answered, c](const InferenceResponse& r) {
                                                  answered[c] += (r.status == 0 && r.precoder.num_tx == 16);
                                              });
        }
        for (int k = 0; k < 50 && (answered[0] + answered[1] + answered[2]) < 3; ++k)
        {
            server.Poll(5);
            for (auto& c : clients)
            {
                c->Poll(5);
            }
        }
        for (int c = 0; c < 3; ++c)
        {
            NS_TEST_ASSERT_MSG_EQ(answered[c], 1u, "client " << c << " got its own answer");
        }

        // 3. Server goes away: client polls return, submits fail, no SIGPIPE.
        server.Stop();
        for (auto& c : clients)
        {
            c->Poll(20);
        }
        uint64_t accepted = 0;
        for (int k = 0; k < 3; ++k)
        {
            accepted += clients[0]->SubmitBeamRequest("beam_index_classifier", 1, 1, 0.0,
                                                      MakeFakeRsrp(1), nullptr) != 0;
            clients[0]->Poll(10);
        }
        NS_TEST_ASSERT_MSG_EQ(clients[0]->IsAttached(), false,
                              "the client notices the peer is gone");
        NS_TEST_ASSERT_MSG_LT(accepted, 2u, "and refuses further requests");
    }
};

/// Overload under Simulator::Run: 4000 requests submitted in one instant, the
/// server drains at most 64 per 1 ms tick. Every request is answered exactly
/// once, none is lost, the drain takes ceil(4000 / 64) ticks, and responses
/// arrive in request order (FIFO in-proc channel).
class AiranOverloadDrainTest : public TestCase
{
  public:
    AiranOverloadDrainTest()
        : TestCase("Overload: 4000-request burst drains at 64/tick, no loss, FIFO, exact tick count")
    {
    }

  private:
    void DoRun() override
    {
        auto server = std::make_shared<AiranInferenceServer>();
        server->RegisterModel(LoadBeamConfig(), AiranMockRuntime::MakeBeamHandler(4, 0.25));
        auto pair = InProcInferenceChannel::CreatePair();
        auto client = std::make_shared<AiranInferenceClient>();
        client->Attach(std::move(pair.first));
        server->AddChannel(std::move(pair.second));
        const uint32_t n = 4000;
        uint64_t lastId = 0;
        bool fifo = true;
        uint32_t done = 0;
        Time drainedAt;
        Simulator::Schedule(MilliSeconds(10), [&]() {
            for (uint32_t i = 0; i < n; ++i)
            {
                client->SubmitBeamRequest("beam_index_classifier", i, 1, 0.01, MakeFakeRsrp(i),
                                          [&](const InferenceResponse& r) {
                                              fifo = fifo && r.request_id > lastId;
                                              lastId = r.request_id;
                                              if (++done == n)
                                              {
                                                  drainedAt = Simulator::Now();
                                              }
                                          });
            }
        });
        uint32_t ticks = 0;
        for (uint32_t t = 11; t < 200; ++t)
        {
            Simulator::Schedule(MilliSeconds(t), [&, server, client]() {
                if (server->Poll(0, 64) > 0)
                {
                    ++ticks;
                }
                client->Poll(0, 1000);
            });
        }
        Simulator::Stop(MilliSeconds(200));
        Simulator::Run();
        Simulator::Destroy();
        NS_TEST_ASSERT_MSG_EQ(done, n, "every request answered");
        NS_TEST_ASSERT_MSG_EQ(client->PendingCount(), 0u, "none lost");
        NS_TEST_ASSERT_MSG_EQ(client->TotalOrphan(), 0u, "none duplicated");
        NS_TEST_ASSERT_MSG_EQ(fifo, true, "answered in request order");
        NS_TEST_ASSERT_MSG_EQ(ticks, (n + 63) / 64, "drained at the per-tick limit");
        NS_TEST_ASSERT_MSG_EQ(drainedAt, MilliSeconds(10 + (n + 63) / 64),
                              "the last answer lands on the last drain tick");
    }
};

/// Determinism: two independent server/client pairs fed the same request
/// sequence return bit-identical responses (the mock is a pure function of the
/// request), and a different CSI changes the precoder.
class AiranDeterminismTest : public TestCase
{
  public:
    AiranDeterminismTest()
        : TestCase("Determinism: identical request streams give bit-identical responses")
    {
    }

  private:
    static std::vector<std::vector<uint8_t>> Stream(double phase)
    {
        AiranInferenceServer server;
        server.RegisterModel(LoadPrecoderConfig(), AiranMockRuntime::MakePrecoderHandler(2, 0.4));
        server.RegisterModel(LoadBeamConfig(), AiranMockRuntime::MakeBeamHandler(4, 0.25));
        auto pair = InProcInferenceChannel::CreatePair();
        server.AddChannel(std::move(pair.second));
        std::vector<std::vector<uint8_t>> out;
        for (uint64_t i = 1; i <= 30; ++i)
        {
            InferenceRequest req = MakeCsiRequest(i, 4, 2, 16, phase + 0.1 * i);
            if (i % 3 == 0)
            {
                req.model_name = "beam_index_classifier";
                req.input_kind = InferenceRequest::InputKind::rsrp;
                req.rsrp = MakeFakeRsrp(i);
            }
            pair.first->Send(AiranMessageCodec::EncodeRequest(req));
            server.Poll(0);
            std::vector<uint8_t> b;
            pair.first->TryRecv(b, 10);
            out.push_back(b);
        }
        return out;
    }

    void DoRun() override
    {
        const auto a = Stream(0.0);
        const auto b = Stream(0.0);
        const auto c = Stream(0.5);
        NS_TEST_ASSERT_MSG_EQ(a.size(), 30u, "30 responses");
        NS_TEST_ASSERT_MSG_EQ((a == b), true, "bit-identical across independent instances");
        NS_TEST_ASSERT_MSG_EQ((a[0] != c[0]), true, "a different channel gives another precoder");
        NS_TEST_ASSERT_MSG_EQ((a[2] == c[2]), true, "beam answers depend only on RSRP");
    }
};

/// Cross-module closed loop (ns3-ai-ntn -> TCP inference -> ntn-traffic -> nr):
/// every 200 ms the UE's measured RSRP for each cell (TS 38.215 SS-RSRP, from
/// the RRC measurement reports of the 5G-LENA stack) is sent to the beam
/// classifier over a real loopback TCP channel; the classifier's best beam
/// names a cell, and when it differs from the serving cell a real X2 handover
/// is triggered. The UE is first put on the far satellite (as a stale decision
/// would), and the model must move it to the near one exactly once, with URLLC
/// packets flowing throughout.
class AiranClosedLoopHandoverTest : public TestCase
{
  public:
    AiranClosedLoopHandoverTest()
        : TestCase("Cross-module: beam model over TCP picks the handover target from measured RSRP")
    {
    }

  private:
    void DoRun() override
    {
        AiranInferenceServer server;
        server.RegisterModel(LoadBeamConfig(), AiranMockRuntime::MakeBeamHandler(2, 0.25));
        if (!server.StartTcpListener("127.0.0.1", 0))
        {
            NS_LOG_UNCOND("cannot bind loopback, skipping");
            return;
        }
        auto ch = std::make_unique<TcpInferenceChannel>();
        NS_TEST_ASSERT_MSG_EQ(ch->ConnectTo("127.0.0.1", server.LocalTcpPort()), true, "connect");
        AiranInferenceClient client;
        client.Attach(std::move(ch));
        server.Poll(20);

        NodeContainer sats;
        sats.Create(2);
        NodeContainer ues;
        ues.Create(1);
        // Satellite 0 is 3 deg off the UE, satellite 1 straight above it.
        for (uint32_t i = 0; i < 2; ++i)
        {
            auto m = CreateObject<ConstantPositionMobilityModel>();
            m->SetPosition(NtnRealStackHelper::GeodeticToEcef(i == 0 ? 43.0 : 40.0, -3.0, 600e3));
            sats.Get(i)->AggregateObject(m);
        }
        auto um = CreateObject<ConstantPositionMobilityModel>();
        um->SetPosition(NtnRealStackHelper::GeodeticToEcef(40.0, -3.0, 0.0));
        ues.Get(0)->AggregateObject(um);

        NtnRealStackHelper rs;
        rs.SetRadioBackend(NtnRealStackHelper::RadioBackend::Nr);
        rs.SetNumerology(1);
        rs.SetSimTime(Seconds(6.0));
        rs.SetOutputDir(CreateTempDirFilename("rs"));
        rs.SetHandover(true, 3.0, MilliSeconds(256));
        rs.SetAutonomousHandover(false);
        rs.SetPeriodicMeasurementReports(true, MilliSeconds(120));
        rs.SetSatEirpDensityDbwMhz(NtnRealStackHelper::kTr38821Set1SBandEirpDensityDbwMhz);
        rs.Build(sats, ues);
        rs.InstallTraffic(NtnRealStackHelper::TrafficProfile::UrllcPings, Seconds(0.8), Seconds(5.9));

        const uint16_t c0 = rs.GetGnbCellId(0);
        const uint16_t c1 = rs.GetGnbCellId(1);
        uint32_t decisions = 0;
        uint32_t triggered = 0;
        uint64_t rxAtHo = 0;
        std::function<void()> loop = [&]() {
            RsrpVector v;
            const double r0 = rs.GetUeServingCellId(0) == c0 ? rs.GetUeServingRsrpDbm(0)
                                                             : rs.GetUeNeighbourRsrpDbm(0, c0);
            const double r1 = rs.GetUeServingCellId(0) == c1 ? rs.GetUeServingRsrpDbm(0)
                                                             : rs.GetUeNeighbourRsrpDbm(0, c1);
            if (std::isfinite(r0) && std::isfinite(r1))
            {
                v.rsrp_dbm = {static_cast<float>(r0), static_cast<float>(r1)};
                client.SubmitBeamRequest("beam_index_classifier", 0, 1,
                                         Simulator::Now().GetSeconds(), v,
                                         [&](const InferenceResponse& r) {
                                             if (r.status != 0)
                                             {
                                                 return;
                                             }
                                             ++decisions;
                                             const uint16_t target = r.beam.best_beam_id == 0 ? c0 : c1;
                                             if (target != rs.GetUeServingCellId(0) &&
                                                 rs.TriggerHandover(0, target))
                                             {
                                                 ++triggered;
                                                 rxAtHo = rs.GetUeRxPackets(0);
                                             }
                                         });
                server.Poll(5);
                client.Poll(5);
            }
            Simulator::Schedule(MilliSeconds(200), loop);
        };
        // Put the UE on the WEAK cell first (as a stale or legacy decision
        // would), so the model has something to correct.
        uint16_t attached = 0;
        bool forced = false;
        Simulator::Schedule(Seconds(0.9), [&]() {
            attached = rs.GetUeServingCellId(0);
            if (attached == c1)
            {
                forced = rs.TriggerHandover(0, c0);
            }
        });
        Simulator::Schedule(Seconds(1.8), loop);
        Simulator::Stop(Seconds(6.0));
        Simulator::Run();
        const uint64_t rxEnd = rs.GetUeRxPackets(0);

        NS_TEST_ASSERT_MSG_GT(decisions, 5u, "the model was consulted on measured RSRP");
        NS_TEST_ASSERT_MSG_EQ(rs.GetUeServingCellId(0), c1,
                              "the UE ends on the strongest (overhead) satellite");
        std::cout << "  [closed loop] attached=" << attached << " (c0=" << c0 << ", c1=" << c1
                  << ") decisions=" << decisions << " handovers=" << triggered << "\n";
        NS_TEST_ASSERT_MSG_LT(triggered, 3u, "no ping-pong: the model's choice is stable");
        NS_TEST_ASSERT_MSG_EQ(forced || attached == c0, true, "the UE starts on the weak cell");
        NS_TEST_ASSERT_MSG_EQ(triggered, 1u, "the model moves it to the strong cell, once");
        NS_TEST_ASSERT_MSG_EQ(rs.GetHandoverCount(), triggered + (forced ? 1u : 0u),
                              "every handover completed");
        if (triggered > 0)
        {
            NS_TEST_ASSERT_MSG_GT(rxEnd, rxAtHo + 200u, "URLLC packets flow after the handover");
        }
        NS_TEST_ASSERT_MSG_GT(rxEnd, 300u, "URLLC delivered over 5 s at 10 ms cadence");
        NS_TEST_ASSERT_MSG_EQ(client.TotalOrphan(), 0u, "every response matched a request");
        server.Stop();
        client.Close();
        Simulator::Destroy();
    }
};

class AiranInferenceTestSuite : public TestSuite
{
  public:
    AiranInferenceTestSuite()
        : TestSuite("oran-ntn-airan-inference", Type::UNIT)
    {
        AddTestCase(new MockPrecoderIsAFunctionOfCsiTest, TestCase::Duration::QUICK);
        AddTestCase(new TritonModelConfigParserTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new TritonModelConfigShippedFilesTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new AiranMessageCodecRoundTripTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new InProcChannelRoundTripTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new TcpChannelRoundTripTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new AiranInProcRoundTripTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new AiranTcpRoundTripTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new AiranFailureModesTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new AiranSimulatorWorkloadTest(),
                     TestCase::Duration::QUICK);
        AddTestCase(new AiranCodecHardeningTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranCodecFidelityTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranPrecoderConformanceTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranBeamConformanceTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranServerAccountingTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranClientFailurePathsTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranTcpFramingAndPeerLossTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranOverloadDrainTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranDeterminismTest, TestCase::Duration::QUICK);
        AddTestCase(new AiranClosedLoopHandoverTest, TestCase::Duration::QUICK);
    }
};

static AiranInferenceTestSuite g_airanInferenceTestSuite;
