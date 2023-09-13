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

#ifndef BURST_GENERATOR_H
#define BURST_GENERATOR_H

#include <ns3/object.h>

namespace ns3
{

class Time;

/**
 * \ingroup applications
 *
 * \brief Virtual interface for burst generators.
 *
 * A virtual interface defining the API for burst generators.
 * Burst generators are thought to be used by BurstyApplication,
 * although their interface might be re-used by other applications.
 *
 * The interface defines two purely virtual methods:
 * - GenerateBurst: generates a new burst, returning its total burst
 * size and the time before the next burst
 * - HasNextBurst: to check whether GenerateBurst can be safely called.
 *
 * The interface is meant to allow for great flexibility in its child
 * classes: such classes can include complex behavior, such as arbitrary
 * probability distributions for burst size and period, correlations
 * among successive burst sizes and periods, cross-correlation between
 * burst size and period, etc.
 *
 */
class BurstGenerator : public Object
{
  public:
    BurstGenerator();
    virtual ~BurstGenerator();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Generate the next burst, composed of a burst size, expressed in Bytes,
     * and a time before the next burst.
     *
     * \return pair with burst size [B] and the time before the next burst
     */
    virtual std::pair<uint32_t, Time> GenerateBurst(void) = 0;

    /**
     * Check whether a next burst can be generated.
     * If not, an error may occur.
     *
     * \return true if a new burst can be generated
     */
    virtual bool HasNextBurst(void) = 0;

  protected:
    virtual void DoDispose(void) override;
};

} // namespace ns3

#endif // BURST_GENERATOR_H
