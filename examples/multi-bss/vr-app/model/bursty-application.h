/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
// University of Padova
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#ifndef BURSTY_APPLICATION_H
#define BURSTY_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/data-rate.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/seq-ts-size-frag-header.h"
#include "ns3/traced-callback.h"

namespace ns3
{

class Address;
class RandomVariableStream;
class Socket;
class BurstGenerator;

/**
 * \ingroup applications
 * \defgroup bursty BurstyApplication
 *
 * This traffic generator supports large packets to be sent into smaller
 * packet (fragment) bursts.
 * The fragment size can be chosen arbitrarily, although it usually set
 * to match the MTU of the chosen network.
 * The burst size and period are controlled by a class extending the
 * BurstGenerator interface.
 *
 */
/**
 * \ingroup bursty
 *
 * \brief Generate traffic to a single destination in bursty fashion.
 *
 * This traffic generator supports large packets to be sent into smaller
 * packet (fragment) bursts.
 * The fragment size can be chosen arbitrarily, although it usually set
 * to match the MTU of the chosen network.
 * The burst size and period are controlled by a class extending the
 * BurstGenerator interface.
 *
 * This application assumes to operate on top of a UDP socket, sending
 * data to a BurstSink.
 * These two classes coexist, one fragmenting a large packet into a burst
 * of smaller fragments, the other by re-assembling the fragments into
 * the full packet.
 *
 * Fragments have all the same length, which can be set via its attribute.
 * The last two segments of the burst might be shorter: the last one
 * because it represents the remainder of the burst size with respect to
 * the maximum frame size, the second to last because packets cannot be
 * smaller that the SeqTsSizeFragHeader size. If the last fragment is
 * too short, the second to last fragment is shortened in order to
 * increase the size of the last fragment.
 * Also, if a BurstGenerator generates a burst of size less than the
 * SeqTsSizeFragHeader size, the burst is discarded and a new burst is
 * queried to the generator.
 *
 */
class BurstyApplication : public Application
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    BurstyApplication();

    virtual ~BurstyApplication();

    /**
     * \brief Return a pointer to associated socket.
     * \return pointer to associated socket
     */
    Ptr<Socket> GetSocket(void) const;

    /**
     * \brief Returns a pointer to the associated BurstGenerator
     * \return pointer to associated BurstGenerator
     */
    Ptr<BurstGenerator> GetBurstGenerator(void) const;

    /**
     * \brief Return the total number of transmitted bursts.
     * \return number of transmitted bursts
     */
    uint64_t GetTotalTxBursts(void) const;

    /**
     * \brief Return the total number of transmitted fragments.
     * \return number of transmitted fragments
     */
    uint64_t GetTotalTxFragments(void) const;

    /**
     * \brief Return the total number of transmitted bytes.
     * \return number of transmitted bytes
     */
    uint64_t GetTotalTxBytes(void) const;

  protected:
    virtual void DoDispose(void);

  private:
    // inherited from Application base class.
    virtual void StartApplication(void); // Called at time specified by Start
    virtual void StopApplication(void);  // Called at time specified by Stop

    // helpers
    /**
     * \brief Cancel all pending events.
     */
    void CancelEvents();

    // Event handlers
    /**
     * \brief Sends a packet burst and schedules the next one
     */
    void SendBurst();

    /**
     * \brief Send burst fragmented into multiple packets
     * \param burstSize the size of the burst in Bytes
     */
    void SendFragmentedBurst(uint32_t burstSize);

    /**
     * \brief Send a single fragment
     * \param fragment the fragment to send
     * \param burstSize size of the entire burst in bytes
     * \param totFrags the number of fragments composing the burst
     * \param fragmentSeq the sequence number of the fragment
     */
    void SendFragment(Ptr<Packet> fragment,
                      uint64_t burstSize,
                      uint16_t totFrags,
                      uint16_t fragmentSeq);

    /**
     * \brief Handle a Connection Succeed event
     * \param socket the connected socket
     */
    void ConnectionSucceeded(Ptr<Socket> socket);

    /**
     * \brief Handle a Connection Failed event
     * \param socket the not connected socket
     */
    void ConnectionFailed(Ptr<Socket> socket);

    Ptr<Socket> m_socket;                 //!< Associated socket
    Address m_peer;                       //!< Peer address
    Address m_local;                      //!< Local address to bind to
    bool m_connected;                     //!< True if connected
    Ptr<BurstGenerator> m_burstGenerator; //!< Burst generator class
    uint32_t m_fragSize;                  //!< Size of fragments including SeqTsSizeFragHeader
    EventId m_nextBurstEvent;             //!< Event id for the next packet burst
    TypeId m_socketTid;                   //!< Type of the socket used
    uint64_t m_totTxBursts;               //!< Total bursts sent
    uint64_t m_totTxFragments;            //!< Total fragments sent
    uint64_t m_totTxBytes;                //!< Total bytes sent

    // Traced Callbacks
    /// Callback for transmitted burst
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeFragHeader&>
        m_txBurstTrace;
    /// Callback for transmitted fragment
    TracedCallback<Ptr<const Packet>, const Address&, const Address&, const SeqTsSizeFragHeader&>
        m_txFragmentTrace;
};

} // namespace ns3

#endif /* BURSTY_APPLICATION_H */
