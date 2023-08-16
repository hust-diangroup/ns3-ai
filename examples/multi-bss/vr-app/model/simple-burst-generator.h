/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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
 */

#ifndef SIMPLE_BURST_GENERATOR_H
#define SIMPLE_BURST_GENERATOR_H

#include <ns3/burst-generator.h>

namespace ns3
{

class RandomVariableStream;

/**
 * \ingroup applications
 *
 * \brief Simple burst generator
 *
 * This burst generator implements the BurstGenerator interface.
 * While being simple, it allows the user to customize the distributions
 * of the burst size and period with independent RandomVariableStreams.
 *
 */
class SimpleBurstGenerator : public BurstGenerator
{
  public:
    SimpleBurstGenerator();
    virtual ~SimpleBurstGenerator();

    // inherited from Object
    static TypeId GetTypeId();

    // inherited from BurstGenerator
    virtual std::pair<uint32_t, Time> GenerateBurst(void) override;
    /**
     * \brief This generator has not limits
     * \return always true
     */
    virtual bool HasNextBurst(void) override;

    /**
     * \brief Assign a fixed random variable stream number to the random variables
     * used by this model.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    virtual void DoDispose(void) override;

  private:
    Ptr<RandomVariableStream> m_periodRv{0};    //!< rng for period duration [s]
    Ptr<RandomVariableStream> m_burstSizeRv{0}; //!< rng for burst size [B]
};

} // namespace ns3

#endif // SIMPLE_BURST_GENERATOR_H
