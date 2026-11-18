/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#include "airan-messages.h"

#include <cstring>

namespace ns3
{
namespace oranntn
{
namespace airan
{

namespace
{

constexpr uint8_t kKindRequest = 0;
constexpr uint8_t kKindResponse = 1;

constexpr uint8_t kWireU32 = 1;
constexpr uint8_t kWireU64 = 2;
constexpr uint8_t kWireF32 = 3;
constexpr uint8_t kWireF64 = 4;
constexpr uint8_t kWireString = 5;
constexpr uint8_t kWireBytes = 6;

// ----- writer -----

struct Writer
{
    std::vector<uint8_t> buf;
    uint16_t field_count = 0;
    size_t fc_pos = 0; // index where field_count is patched in

    void Begin(uint8_t msg_kind)
    {
        buf.reserve(64);
        buf.push_back(msg_kind);
        fc_pos = buf.size();
        buf.push_back(0); // hi
        buf.push_back(0); // lo
    }

    void End()
    {
        buf[fc_pos] = static_cast<uint8_t>((field_count >> 8) & 0xFF);
        buf[fc_pos + 1] = static_cast<uint8_t>(field_count & 0xFF);
    }

    void PushU16(uint16_t v)
    {
        buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        buf.push_back(static_cast<uint8_t>(v & 0xFF));
    }
    void PushU32(uint32_t v)
    {
        for (int i = 3; i >= 0; --i)
        {
            buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
        }
    }
    void PushU64(uint64_t v)
    {
        for (int i = 7; i >= 0; --i)
        {
            buf.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
        }
    }

    void TagU32(uint16_t fid, uint32_t v)
    {
        PushU16(fid);
        buf.push_back(kWireU32);
        PushU32(4);
        PushU32(v);
        ++field_count;
    }
    void TagU64(uint16_t fid, uint64_t v)
    {
        PushU16(fid);
        buf.push_back(kWireU64);
        PushU32(8);
        PushU64(v);
        ++field_count;
    }
    void TagF32(uint16_t fid, float v)
    {
        PushU16(fid);
        buf.push_back(kWireF32);
        PushU32(4);
        uint32_t bits;
        std::memcpy(&bits, &v, 4);
        PushU32(bits);
        ++field_count;
    }
    void TagF64(uint16_t fid, double v)
    {
        PushU16(fid);
        buf.push_back(kWireF64);
        PushU32(8);
        uint64_t bits;
        std::memcpy(&bits, &v, 8);
        PushU64(bits);
        ++field_count;
    }
    void TagString(uint16_t fid, const std::string& s)
    {
        PushU16(fid);
        buf.push_back(kWireString);
        PushU32(static_cast<uint32_t>(s.size()));
        buf.insert(buf.end(), s.begin(), s.end());
        ++field_count;
    }
    void TagBytes(uint16_t fid, const std::vector<uint8_t>& b)
    {
        PushU16(fid);
        buf.push_back(kWireBytes);
        PushU32(static_cast<uint32_t>(b.size()));
        buf.insert(buf.end(), b.begin(), b.end());
        ++field_count;
    }
    /// Packed float32 array as bytes.
    void TagFloatArray(uint16_t fid, const std::vector<float>& vs)
    {
        PushU16(fid);
        buf.push_back(kWireBytes);
        PushU32(static_cast<uint32_t>(vs.size() * 4));
        for (float v : vs)
        {
            uint32_t bits;
            std::memcpy(&bits, &v, 4);
            for (int i = 3; i >= 0; --i)
            {
                buf.push_back(
                    static_cast<uint8_t>((bits >> (i * 8)) & 0xFF));
            }
        }
        ++field_count;
    }
};

std::vector<uint8_t>
EncodeCsi(const CsiTensor& csi)
{
    Writer w;
    w.Begin(0); // sub-msg kind: ignored
    w.TagU32(fid::kCsiNumTx, csi.num_tx);
    w.TagU32(fid::kCsiNumRx, csi.num_rx);
    w.TagU32(fid::kCsiNumSc, csi.num_subcarriers);
    w.TagF64(fid::kCsiDoppler, csi.doppler_hz);
    w.TagFloatArray(fid::kCsiValues, csi.values);
    w.End();
    return w.buf;
}

std::vector<uint8_t>
EncodeRsrp(const RsrpVector& v)
{
    Writer w;
    w.Begin(0);
    w.TagFloatArray(fid::kRsrpRsrp, v.rsrp_dbm);
    w.TagFloatArray(fid::kRsrpAz, v.beam_az_deg);
    w.TagFloatArray(fid::kRsrpEl, v.beam_el_deg);
    w.End();
    return w.buf;
}

std::vector<uint8_t>
EncodePrecoder(const PrecoderWeights& p)
{
    Writer w;
    w.Begin(0);
    w.TagU32(fid::kPrecoderNumTx, p.num_tx);
    w.TagU32(fid::kPrecoderNumLayers, p.num_layers);
    w.TagFloatArray(fid::kPrecoderValues, p.values);
    w.End();
    return w.buf;
}

std::vector<uint8_t>
EncodeBeam(const BeamPrediction& b)
{
    Writer w;
    w.Begin(0);
    w.TagU32(fid::kBeamBestId, b.best_beam_id);
    w.TagF32(fid::kBeamScore, b.score);
    w.TagFloatArray(fid::kBeamTopK, b.scores_top_k);
    w.End();
    return w.buf;
}

/// A fixed-width field must declare exactly its width. The decoders read the
/// value by its wire type and not by the declared length, so a frame that said
/// u32 with length 8 was consumed 4 bytes short and every later field was read
/// from the wrong offset.
bool
LenMatchesWire(uint8_t wt, uint32_t len)
{
    switch (wt)
    {
    case kWireU32:
    case kWireF32:
        return len == 4;
    case kWireU64:
    case kWireF64:
        return len == 8;
    default:
        return true;
    }
}

// ----- reader -----

struct Reader
{
    const std::vector<uint8_t>& buf;
    size_t i = 0;
    bool err = false;

    explicit Reader(const std::vector<uint8_t>& b) : buf(b) {}

    bool Eof() const { return i >= buf.size(); }

    uint8_t U8()
    {
        if (i + 1 > buf.size())
        {
            err = true;
            return 0;
        }
        return buf[i++];
    }
    uint16_t U16()
    {
        if (i + 2 > buf.size())
        {
            err = true;
            return 0;
        }
        const uint16_t v = (static_cast<uint16_t>(buf[i]) << 8) |
                           static_cast<uint16_t>(buf[i + 1]);
        i += 2;
        return v;
    }
    uint32_t U32()
    {
        if (i + 4 > buf.size())
        {
            err = true;
            return 0;
        }
        uint32_t v = 0;
        for (int k = 0; k < 4; ++k)
        {
            v = (v << 8) | buf[i + k];
        }
        i += 4;
        return v;
    }
    uint64_t U64()
    {
        if (i + 8 > buf.size())
        {
            err = true;
            return 0;
        }
        uint64_t v = 0;
        for (int k = 0; k < 8; ++k)
        {
            v = (v << 8) | static_cast<uint64_t>(buf[i + k]);
        }
        i += 8;
        return v;
    }
    float F32()
    {
        const uint32_t bits = U32();
        float v;
        std::memcpy(&v, &bits, 4);
        return v;
    }
    double F64()
    {
        const uint64_t bits = U64();
        double v;
        std::memcpy(&v, &bits, 8);
        return v;
    }
    std::string Str(uint32_t len)
    {
        if (i + len > buf.size())
        {
            err = true;
            return {};
        }
        std::string out(reinterpret_cast<const char*>(&buf[i]), len);
        i += len;
        return out;
    }
    std::vector<uint8_t> Bytes(uint32_t len)
    {
        if (i + len > buf.size())
        {
            err = true;
            return {};
        }
        std::vector<uint8_t> out(buf.begin() + i,
                                 buf.begin() + i + len);
        i += len;
        return out;
    }
    std::vector<float> FloatArray(uint32_t len_bytes)
    {
        // Bounds first: a corrupt length used to reach reserve() and could
        // request up to 4 GB before the per-element reads noticed the frame
        // was short.
        if (len_bytes % 4 != 0 || i + len_bytes > buf.size())
        {
            err = true;
            return {};
        }
        const uint32_t n = len_bytes / 4;
        std::vector<float> out;
        out.reserve(n);
        for (uint32_t k = 0; k < n; ++k)
        {
            out.push_back(F32());
        }
        return out;
    }
};

bool
DecodeCsi(const std::vector<uint8_t>& bytes, CsiTensor& out)
{
    Reader r(bytes);
    r.U8(); // sub-msg kind, ignored
    const uint16_t fc = r.U16();
    if (r.err)
    {
        return false;
    }
    for (uint16_t k = 0; k < fc; ++k)
    {
        const uint16_t fid = r.U16();
        const uint8_t wt = r.U8();
        const uint32_t len = r.U32();
        if (r.err || !LenMatchesWire(wt, len))
        {
            return false;
        }
        switch (fid)
        {
        case fid::kCsiNumTx:
            if (wt != kWireU32)
            {
                return false;
            }
            out.num_tx = r.U32();
            break;
        case fid::kCsiNumRx:
            if (wt != kWireU32)
            {
                return false;
            }
            out.num_rx = r.U32();
            break;
        case fid::kCsiNumSc:
            if (wt != kWireU32)
            {
                return false;
            }
            out.num_subcarriers = r.U32();
            break;
        case fid::kCsiDoppler:
            if (wt != kWireF64)
            {
                return false;
            }
            out.doppler_hz = r.F64();
            break;
        case fid::kCsiValues:
            if (wt != kWireBytes)
            {
                return false;
            }
            out.values = r.FloatArray(len);
            break;
        default:
            r.Bytes(len);
            break;
        }
    }
    return !r.err;
}

bool
DecodeRsrp(const std::vector<uint8_t>& bytes, RsrpVector& out)
{
    Reader r(bytes);
    r.U8();
    const uint16_t fc = r.U16();
    if (r.err)
    {
        return false;
    }
    for (uint16_t k = 0; k < fc; ++k)
    {
        const uint16_t fid = r.U16();
        const uint8_t wt = r.U8();
        const uint32_t len = r.U32();
        if (r.err || !LenMatchesWire(wt, len))
        {
            return false;
        }
        if (wt != kWireBytes)
        {
            r.Bytes(len);
            continue;
        }
        switch (fid)
        {
        case fid::kRsrpRsrp:
            out.rsrp_dbm = r.FloatArray(len);
            break;
        case fid::kRsrpAz:
            out.beam_az_deg = r.FloatArray(len);
            break;
        case fid::kRsrpEl:
            out.beam_el_deg = r.FloatArray(len);
            break;
        default:
            r.Bytes(len);
            break;
        }
    }
    return !r.err;
}

bool
DecodePrecoder(const std::vector<uint8_t>& bytes, PrecoderWeights& out)
{
    Reader r(bytes);
    r.U8();
    const uint16_t fc = r.U16();
    if (r.err)
    {
        return false;
    }
    for (uint16_t k = 0; k < fc; ++k)
    {
        const uint16_t fid = r.U16();
        const uint8_t wt = r.U8();
        const uint32_t len = r.U32();
        if (r.err || !LenMatchesWire(wt, len))
        {
            return false;
        }
        switch (fid)
        {
        case fid::kPrecoderNumTx:
            if (wt != kWireU32)
            {
                return false;
            }
            out.num_tx = r.U32();
            break;
        case fid::kPrecoderNumLayers:
            if (wt != kWireU32)
            {
                return false;
            }
            out.num_layers = r.U32();
            break;
        case fid::kPrecoderValues:
            if (wt != kWireBytes)
            {
                return false;
            }
            out.values = r.FloatArray(len);
            break;
        default:
            r.Bytes(len);
            break;
        }
    }
    return !r.err;
}

bool
DecodeBeam(const std::vector<uint8_t>& bytes, BeamPrediction& out)
{
    Reader r(bytes);
    r.U8();
    const uint16_t fc = r.U16();
    if (r.err)
    {
        return false;
    }
    for (uint16_t k = 0; k < fc; ++k)
    {
        const uint16_t fid = r.U16();
        const uint8_t wt = r.U8();
        const uint32_t len = r.U32();
        if (r.err || !LenMatchesWire(wt, len))
        {
            return false;
        }
        switch (fid)
        {
        case fid::kBeamBestId:
            if (wt != kWireU32)
            {
                return false;
            }
            out.best_beam_id = r.U32();
            break;
        case fid::kBeamScore:
            if (wt != kWireF32)
            {
                return false;
            }
            out.score = r.F32();
            break;
        case fid::kBeamTopK:
            if (wt != kWireBytes)
            {
                return false;
            }
            out.scores_top_k = r.FloatArray(len);
            break;
        default:
            r.Bytes(len);
            break;
        }
    }
    return !r.err;
}

} // namespace

std::vector<uint8_t>
AiranMessageCodec::EncodeRequest(const InferenceRequest& r)
{
    Writer w;
    w.Begin(kKindRequest);
    w.TagString(fid::kReqModelName, r.model_name);
    w.TagU64(fid::kReqRequestId, r.request_id);
    w.TagU64(fid::kReqUeId, r.ue_id);
    w.TagU64(fid::kReqNrCgi, r.nr_cgi);
    w.TagF64(fid::kReqSimTimeS, r.sim_time_s);
    if (r.input_kind == InferenceRequest::InputKind::csi)
    {
        const std::vector<uint8_t> sub = EncodeCsi(r.csi);
        w.TagBytes(fid::kReqCsi, sub);
    }
    else if (r.input_kind == InferenceRequest::InputKind::rsrp)
    {
        const std::vector<uint8_t> sub = EncodeRsrp(r.rsrp);
        w.TagBytes(fid::kReqRsrp, sub);
    }
    w.End();
    return w.buf;
}

std::vector<uint8_t>
AiranMessageCodec::EncodeResponse(const InferenceResponse& r)
{
    Writer w;
    w.Begin(kKindResponse);
    w.TagU64(fid::kRespRequestId, r.request_id);
    w.TagF64(fid::kRespLatency, r.inference_latency_ms);
    w.TagU32(fid::kRespStatus, r.status);
    w.TagString(fid::kRespStatusMsg, r.status_message);
    if (r.output_kind == InferenceResponse::OutputKind::precoder)
    {
        const std::vector<uint8_t> sub = EncodePrecoder(r.precoder);
        w.TagBytes(fid::kRespPrecoder, sub);
    }
    else if (r.output_kind == InferenceResponse::OutputKind::beam)
    {
        const std::vector<uint8_t> sub = EncodeBeam(r.beam);
        w.TagBytes(fid::kRespBeam, sub);
    }
    w.End();
    return w.buf;
}

bool
AiranMessageCodec::DecodeRequest(const std::vector<uint8_t>& bytes,
                                  InferenceRequest& out)
{
    out = InferenceRequest{};
    Reader r(bytes);
    const uint8_t kind = r.U8();
    if (kind != kKindRequest)
    {
        return false;
    }
    const uint16_t fc = r.U16();
    if (r.err)
    {
        return false;
    }
    for (uint16_t k = 0; k < fc; ++k)
    {
        const uint16_t fid = r.U16();
        const uint8_t wt = r.U8();
        const uint32_t len = r.U32();
        if (r.err || !LenMatchesWire(wt, len))
        {
            return false;
        }
        switch (fid)
        {
        case fid::kReqModelName:
            if (wt != kWireString)
            {
                return false;
            }
            out.model_name = r.Str(len);
            break;
        case fid::kReqRequestId:
            if (wt != kWireU64)
            {
                return false;
            }
            out.request_id = r.U64();
            break;
        case fid::kReqUeId:
            if (wt != kWireU64)
            {
                return false;
            }
            out.ue_id = r.U64();
            break;
        case fid::kReqNrCgi:
            if (wt != kWireU64)
            {
                return false;
            }
            out.nr_cgi = r.U64();
            break;
        case fid::kReqSimTimeS:
            if (wt != kWireF64)
            {
                return false;
            }
            out.sim_time_s = r.F64();
            break;
        case fid::kReqCsi:
        {
            if (wt != kWireBytes)
            {
                return false;
            }
            const auto sub = r.Bytes(len);
            if (!DecodeCsi(sub, out.csi))
            {
                return false;
            }
            out.input_kind = InferenceRequest::InputKind::csi;
            break;
        }
        case fid::kReqRsrp:
        {
            if (wt != kWireBytes)
            {
                return false;
            }
            const auto sub = r.Bytes(len);
            if (!DecodeRsrp(sub, out.rsrp))
            {
                return false;
            }
            out.input_kind = InferenceRequest::InputKind::rsrp;
            break;
        }
        default:
            r.Bytes(len);
            break;
        }
    }
    return !r.err;
}

bool
AiranMessageCodec::DecodeResponse(const std::vector<uint8_t>& bytes,
                                   InferenceResponse& out)
{
    out = InferenceResponse{};
    Reader r(bytes);
    const uint8_t kind = r.U8();
    if (kind != kKindResponse)
    {
        return false;
    }
    const uint16_t fc = r.U16();
    if (r.err)
    {
        return false;
    }
    for (uint16_t k = 0; k < fc; ++k)
    {
        const uint16_t fid = r.U16();
        const uint8_t wt = r.U8();
        const uint32_t len = r.U32();
        if (r.err || !LenMatchesWire(wt, len))
        {
            return false;
        }
        switch (fid)
        {
        case fid::kRespRequestId:
            if (wt != kWireU64)
            {
                return false;
            }
            out.request_id = r.U64();
            break;
        case fid::kRespLatency:
            if (wt != kWireF64)
            {
                return false;
            }
            out.inference_latency_ms = r.F64();
            break;
        case fid::kRespStatus:
            if (wt != kWireU32)
            {
                return false;
            }
            out.status = r.U32();
            break;
        case fid::kRespStatusMsg:
            if (wt != kWireString)
            {
                return false;
            }
            out.status_message = r.Str(len);
            break;
        case fid::kRespPrecoder:
        {
            if (wt != kWireBytes)
            {
                return false;
            }
            const auto sub = r.Bytes(len);
            if (!DecodePrecoder(sub, out.precoder))
            {
                return false;
            }
            out.output_kind = InferenceResponse::OutputKind::precoder;
            break;
        }
        case fid::kRespBeam:
        {
            if (wt != kWireBytes)
            {
                return false;
            }
            const auto sub = r.Bytes(len);
            if (!DecodeBeam(sub, out.beam))
            {
                return false;
            }
            out.output_kind = InferenceResponse::OutputKind::beam;
            break;
        }
        default:
            r.Bytes(len);
            break;
        }
    }
    return !r.err;
}

uint8_t
AiranMessageCodec::PeekKind(const std::vector<uint8_t>& bytes)
{
    if (bytes.empty())
    {
        return 0xFF;
    }
    return bytes[0];
}

} // namespace airan
} // namespace oranntn
} // namespace ns3
