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
 *
 */

#include "seq-ts-size-frag-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SeqTsSizeFragHeader");

NS_OBJECT_ENSURE_REGISTERED(SeqTsSizeFragHeader);

SeqTsSizeFragHeader::SeqTsSizeFragHeader()
    : SeqTsSizeHeader()
{
    NS_LOG_FUNCTION(this);
}

TypeId
SeqTsSizeFragHeader::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::SeqTsSizeFragHeader")
                            .SetParent<SeqTsSizeHeader>()
                            .SetGroupName("Applications")
                            .AddConstructor<SeqTsSizeFragHeader>();
    return tid;
}

TypeId
SeqTsSizeFragHeader::GetInstanceTypeId(void) const
{
    return GetTypeId();
}

void
SeqTsSizeFragHeader::SetFragSeq(uint16_t fragSeq)
{
    m_fragSeq = fragSeq;
}

uint16_t
SeqTsSizeFragHeader::GetFragSeq(void) const
{
    return m_fragSeq;
}

void
SeqTsSizeFragHeader::SetFrags(uint16_t frags)
{
    m_frags = frags;
}

uint16_t
SeqTsSizeFragHeader::GetFrags(void) const
{
    return m_frags;
}

void
SeqTsSizeFragHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(fragSeq=" << m_fragSeq << ", frags=" << m_frags << ") AND ";
    SeqTsSizeHeader::Print(os);
}

uint32_t
SeqTsSizeFragHeader::GetSerializedSize(void) const
{
    return SeqTsSizeHeader::GetSerializedSize() + 4;
}

void
SeqTsSizeFragHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU16(m_fragSeq);
    i.WriteHtonU16(m_frags);
    SeqTsSizeHeader::Serialize(i);
}

uint32_t
SeqTsSizeFragHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_fragSeq = i.ReadNtohU16();
    m_frags = i.ReadNtohU16();
    SeqTsSizeHeader::Deserialize(i);
    return GetSerializedSize();
}

} // namespace ns3
