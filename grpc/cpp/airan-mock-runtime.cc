/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// Copyright (c) 2026 Muhammad Uzair
// SPDX-License-Identifier: GPL-2.0-only

#include "airan-mock-runtime.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <vector>

namespace ns3
{
namespace oranntn
{
namespace airan
{

AiranInferenceServer::ModelHandler
AiranMockRuntime::MakePrecoderHandler(uint32_t num_layers,
                                       double base_latency_ms,
                                       uint32_t expected_num_tx)
{
    return [num_layers, base_latency_ms, expected_num_tx](const InferenceRequest& req,
                                                          InferenceResponse& out) {
        const CsiTensor& csi = req.csi;
        const uint32_t num_tx = std::max(csi.num_tx, 1u);
        out.output_kind = InferenceResponse::OutputKind::precoder;
        out.precoder.num_tx = num_tx;
        out.precoder.num_layers = num_layers;
        out.precoder.values.assign(2 * num_tx * num_layers, 0.0f);

        // AI-06: compute the precoder from the CSI.
        //
        // This kernel used to be cos/sin of `seed = req.request_id`. The output
        // depended on the request counter and nothing else - only csi.num_tx was
        // read - so feeding it a transposed tensor, a stale channel, or no
        // channel at all produced exactly the same beam. Calling that an
        // inference model overstated it, and worse, it meant a wrong tensor
        // layout could never be detected downstream.
        //
        // It is still a mock, and says so: what it computes is maximum-ratio
        // transmission with Gram-Schmidt orthogonalisation for the additional
        // layers, which is a closed-form beamformer and not a learned one. But
        // it is a genuine function of the channel, so a wrong layout now
        // produces a wrong beam and the SINR shows it.
        // TS 38.214 section 5.2.2.2: the number of layers cannot exceed the
        // number of antenna ports; more layers than transmit antennas cannot
        // be mutually orthogonal, so such a precoder would be a fabrication.
        if (!csi.LayoutValid() || csi.num_tx != num_tx || num_layers == 0 ||
            num_layers > num_tx ||
            (expected_num_tx != 0 && csi.num_tx != expected_num_tx))
        {
            // Refuse rather than fabricate. A model handed a tensor that does
            // not match its declared dimensions must not return a confident
            // precoder.
            out.status = 1;
            out.inference_latency_ms = base_latency_ms;
            return;
        }

        // Average the channel over subcarriers: h_rx[tx] = mean_sc H[sc][rx][tx].
        // Wideband MRT, which is what a single precoder per report implies.
        std::vector<std::vector<std::complex<double>>> hbar(
            csi.num_rx, std::vector<std::complex<double>>(num_tx, {0.0, 0.0}));
        for (uint32_t sc = 0; sc < csi.num_subcarriers; ++sc)
        {
            for (uint32_t rx = 0; rx < csi.num_rx; ++rx)
            {
                for (uint32_t tx = 0; tx < num_tx; ++tx)
                {
                    const std::size_t off = csi.Index(sc, rx, tx);
                    hbar[rx][tx] += std::complex<double>(csi.values[off], csi.values[off + 1]);
                }
            }
        }
        const double invSc = 1.0 / static_cast<double>(csi.num_subcarriers);
        for (auto& row : hbar)
        {
            for (auto& v : row)
            {
                v *= invSc;
            }
        }

        // Layer L takes receive branch (L mod num_rx), MRT-conjugated, then
        // orthogonalised against the layers already chosen so the streams do
        // not collapse onto one direction.
        std::vector<std::vector<std::complex<double>>> w;
        w.reserve(num_layers);
        for (uint32_t layer = 0; layer < num_layers; ++layer)
        {
            std::vector<std::complex<double>> v(num_tx, {0.0, 0.0});
            const uint32_t rx = layer % csi.num_rx;
            for (uint32_t tx = 0; tx < num_tx; ++tx)
            {
                v[tx] = std::conj(hbar[rx][tx]); // MRT
            }
            for (const auto& prev : w)
            {
                std::complex<double> proj{0.0, 0.0};
                for (uint32_t tx = 0; tx < num_tx; ++tx)
                {
                    proj += std::conj(prev[tx]) * v[tx];
                }
                for (uint32_t tx = 0; tx < num_tx; ++tx)
                {
                    v[tx] -= proj * prev[tx];
                }
            }
            double norm = 0.0;
            for (uint32_t tx = 0; tx < num_tx; ++tx)
            {
                norm += std::norm(v[tx]);
            }
            norm = std::sqrt(norm);
            if (norm < 1e-12)
            {
                // The branch collapsed (rank deficient channel). Fall back to
                // the first antenna direction that is not already spanned,
                // orthogonalized like any other layer. The old fallback took
                // e_(layer mod num_tx) as is, which overlaps the earlier layers
                // whenever they have energy on that antenna, so the "extra"
                // stream interfered with the first one.
                for (uint32_t k = 0; k < num_tx && norm < 1e-6; ++k)
                {
                    std::fill(v.begin(), v.end(), std::complex<double>{0.0, 0.0});
                    v[(layer + k) % num_tx] = {1.0, 0.0};
                    for (const auto& prev : w)
                    {
                        const std::complex<double> proj = std::conj(prev[(layer + k) % num_tx]);
                        for (uint32_t tx = 0; tx < num_tx; ++tx)
                        {
                            v[tx] -= proj * prev[tx];
                        }
                    }
                    norm = 0.0;
                    for (uint32_t tx = 0; tx < num_tx; ++tx)
                    {
                        norm += std::norm(v[tx]);
                    }
                    norm = std::sqrt(norm);
                }
            }
            for (uint32_t tx = 0; tx < num_tx; ++tx)
            {
                v[tx] /= norm;
            }
            w.push_back(std::move(v));
        }

        // Total transmit power is split across layers, so the whole precoder is
        // unit-norm rather than each layer being.
        const double layerScale = 1.0 / std::sqrt(static_cast<double>(num_layers));
        for (uint32_t tx = 0; tx < num_tx; ++tx)
        {
            for (uint32_t layer = 0; layer < num_layers; ++layer)
            {
                const uint32_t off = 2 * (tx * num_layers + layer);
                out.precoder.values[off] = static_cast<float>(w[layer][tx].real() * layerScale);
                out.precoder.values[off + 1] = static_cast<float>(w[layer][tx].imag() * layerScale);
            }
        }
        out.status = 0;
        const double seed = static_cast<double>(req.request_id);
        out.inference_latency_ms =
            base_latency_ms +
            0.05 * std::abs(std::sin(seed * 0.01));
    };
}

AiranInferenceServer::ModelHandler
AiranMockRuntime::MakeBeamHandler(uint32_t top_k, double latency_ms)
{
    return [top_k, latency_ms](const InferenceRequest& req,
                                InferenceResponse& out) {
        const RsrpVector& v = req.rsrp;
        out.output_kind = InferenceResponse::OutputKind::beam;
        // Refuse rather than fabricate, as the precoder does. An empty RSRP
        // vector used to come back status 0 with beam 0 and score 0 dBm, a
        // confident answer with no measurement behind it; a NaN RSRP made
        // max_element's result unspecified; and beam angles that do not line
        // up one to one with the RSRP entries cannot name a beam.
        bool finite = true;
        for (float x : v.rsrp_dbm)
        {
            finite = finite && std::isfinite(x);
        }
        const bool anglesOk =
            (v.beam_az_deg.empty() || v.beam_az_deg.size() == v.rsrp_dbm.size()) &&
            (v.beam_el_deg.empty() || v.beam_el_deg.size() == v.rsrp_dbm.size());
        if (v.rsrp_dbm.empty() || !finite || !anglesOk)
        {
            out.beam.best_beam_id = 0;
            out.beam.score = 0.0f;
            out.status = 1;
            out.status_message = v.rsrp_dbm.empty() ? "empty RSRP vector"
                                 : !finite          ? "non-finite RSRP"
                                                    : "beam angles do not match the RSRP vector";
            out.inference_latency_ms = latency_ms;
            return;
        }
        const auto max_it =
            std::max_element(v.rsrp_dbm.begin(), v.rsrp_dbm.end());
        out.beam.best_beam_id = static_cast<uint32_t>(
            std::distance(v.rsrp_dbm.begin(), max_it));
        out.beam.score = *max_it;
        // Build sorted top-K
        std::vector<float> sorted = v.rsrp_dbm;
        std::sort(sorted.begin(),
                   sorted.end(),
                   [](float a, float b) { return a > b; });
        const uint32_t k = std::min<uint32_t>(
            top_k, static_cast<uint32_t>(sorted.size()));
        out.beam.scores_top_k.assign(sorted.begin(),
                                      sorted.begin() + k);
        out.status = 0;
        out.inference_latency_ms = latency_ms;
    };
}

} // namespace airan
} // namespace oranntn
} // namespace ns3
