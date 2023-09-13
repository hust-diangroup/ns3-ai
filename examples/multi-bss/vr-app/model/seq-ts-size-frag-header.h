/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef SEQ_TS_SIZE_FRAG_HEADER_H
#define SEQ_TS_SIZE_FRAG_HEADER_H

#include <ns3/seq-ts-size-header.h>

namespace ns3
{
/**
 * \ingroup applications
 * \brief Header with a sequence, a timestamp, a size, a fragment sequence and a number of fragments
 * attribute
 *
 * This header adds a fragment sequence and a number of fragments attribute to the (burst) sequence
 * number, the timestamp and the size of class \c SeqTsSizeFragHeader. Fragment sequence and number
 * of fragments can be used to track large fragments packets over protocols not guaranteeing packet
 * ordering, e.g., BurstyApplication over UDP.
 *
 * \sa ns3::SeqTsHeader
 */
class SeqTsSizeFragHeader : public SeqTsSizeHeader
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId(void);

    /**
     * \brief constructor
     */
    SeqTsSizeFragHeader();

    /**
     * \brief Set the sequence number of the fragment
     * \param size sequence number of the fragment
     */
    void SetFragSeq(uint16_t fragSeq);

    /**
     * \brief Get the sequence number of the fragment
     * \return the sequence number of the fragment
     */
    uint16_t GetFragSeq(void) const;

    /**
     * \brief Set the total number fragments in the burst
     * \param frags the total number of fragments in the burst
     */
    void SetFrags(uint16_t frags);

    /**
     * \brief Get the total number of fragments in the burst
     * \return the total number of fragments in the burst
     */
    uint16_t GetFrags(void) const;

    // Inherited
    virtual TypeId GetInstanceTypeId(void) const override;
    virtual void Print(std::ostream& os) const override;
    virtual uint32_t GetSerializedSize(void) const override;
    virtual void Serialize(Buffer::Iterator start) const override;
    virtual uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    uint16_t m_fragSeq{0}; //!< The sequence number of the fragment
    uint16_t m_frags{0};   //!< The total number of fragments in the burst
};

} // namespace ns3

#endif /* SEQ_TS_SIZE_FRAG_HEADER_H */
