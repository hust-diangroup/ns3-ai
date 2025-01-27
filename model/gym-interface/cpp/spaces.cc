/*
 * Copyright (c) 2018 Piotr Gawlowicz
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
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 * Modify: Muyuan Shen <muyuan_shen@hust.edu.cn>
 *
 */

#include "spaces.h"

#include "ns3/log.h"
#include "ns3/object.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("OpenGymSpace");
NS_OBJECT_ENSURE_REGISTERED(OpenGymSpace);

TypeId
OpenGymSpace::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymSpace").SetParent<Object>().SetGroupName("OpenGym");
    return tid;
}

OpenGymSpace::OpenGymSpace()
{
    NS_LOG_FUNCTION(this);
}

OpenGymSpace::~OpenGymSpace()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymSpace::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymSpace::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

TypeId
OpenGymDiscreteSpace::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymDiscreteSpace")
                            .SetParent<OpenGymSpace>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<OpenGymDiscreteSpace>();
    return tid;
}

OpenGymDiscreteSpace::OpenGymDiscreteSpace()
{
    NS_LOG_FUNCTION(this);
}

OpenGymDiscreteSpace::OpenGymDiscreteSpace(int n)
    : m_n(n)
{
    NS_LOG_FUNCTION(this);
}

OpenGymDiscreteSpace::~OpenGymDiscreteSpace()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymDiscreteSpace::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymDiscreteSpace::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

int
OpenGymDiscreteSpace::GetN()
{
    NS_LOG_FUNCTION(this);
    return m_n;
}

ns3_ai_gym::SpaceDescription
OpenGymDiscreteSpace::GetSpaceDescription()
{
    NS_LOG_FUNCTION(this);
    ns3_ai_gym::SpaceDescription desc;
    desc.set_type(ns3_ai_gym::Discrete);
    ns3_ai_gym::DiscreteSpace discreteSpace;
    discreteSpace.set_n(GetN());
    desc.mutable_space()->PackFrom(discreteSpace);
    return desc;
}

void
OpenGymDiscreteSpace::Print(std::ostream& where) const
{
    where << " DiscreteSpace N: " << m_n;
}

TypeId
OpenGymBoxSpace::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymBoxSpace")
                            .SetParent<OpenGymSpace>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<OpenGymBoxSpace>();
    return tid;
}

OpenGymBoxSpace::OpenGymBoxSpace()
{
    NS_LOG_FUNCTION(this);
}

OpenGymBoxSpace::OpenGymBoxSpace(float low,
                                 float high,
                                 std::vector<uint32_t> shape,
                                 std::string dtype)
    : m_low(low),
      m_high(high),
      m_shape(shape),
      m_dtypeName(dtype)
{
    NS_LOG_FUNCTION(this);
    SetDtype();
}

OpenGymBoxSpace::OpenGymBoxSpace(std::vector<float> low,
                                 std::vector<float> high,
                                 std::vector<uint32_t> shape,
                                 std::string dtype)
    : m_low(0),
      m_high(0),
      m_shape(shape),
      m_dtypeName(dtype),
      m_lowVec(low),
      m_highVec(high)

{
    NS_LOG_FUNCTION(this);
    SetDtype();
}

OpenGymBoxSpace::~OpenGymBoxSpace()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymBoxSpace::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymBoxSpace::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymBoxSpace::SetDtype()
{
    std::string name = m_dtypeName;
    if (name == "int8_t" || name == "int16_t" || name == "int32_t" || name == "int64_t")
    {
        m_dtype = ns3_ai_gym::INT;
    }
    else if (name == "uint8_t" || name == "uint16_t" || name == "uint32_t" || name == "uint64_t")
    {
        m_dtype = ns3_ai_gym::UINT;
    }
    else if (name == "float")
    {
        m_dtype = ns3_ai_gym::FLOAT;
    }
    else if (name == "double")
    {
        m_dtype = ns3_ai_gym::DOUBLE;
    }
    else
    {
        m_dtype = ns3_ai_gym::FLOAT;
    }
}

float
OpenGymBoxSpace::GetLow()
{
    NS_LOG_FUNCTION(this);
    return m_low;
}

float
OpenGymBoxSpace::GetHigh()
{
    NS_LOG_FUNCTION(this);
    return m_high;
}

std::vector<uint32_t>
OpenGymBoxSpace::GetShape()
{
    NS_LOG_FUNCTION(this);
    return m_shape;
}

ns3_ai_gym::SpaceDescription
OpenGymBoxSpace::GetSpaceDescription()
{
    NS_LOG_FUNCTION(this);
    ns3_ai_gym::SpaceDescription desc;
    desc.set_type(ns3_ai_gym::Box);

    ns3_ai_gym::BoxSpace boxSpacePb;
    boxSpacePb.set_low(GetLow());
    boxSpacePb.set_high(GetHigh());

    std::vector<uint32_t> shape = GetShape();
    for (auto i = shape.begin(); i != shape.end(); ++i)
    {
        boxSpacePb.add_shape(*i);
    }

    for (const auto& low : m_lowVec)
    {
        boxSpacePb.add_lows(low);
    }
    for (const auto& high : m_highVec)
    {
        boxSpacePb.add_highs(high);
    }

    boxSpacePb.set_dtype(m_dtype);
    desc.mutable_space()->PackFrom(boxSpacePb);
    return desc;
}

void
OpenGymBoxSpace::Print(std::ostream& where) const
{
    where << " BoxSpace Low: " << m_low << " High: " << m_high << " Shape: (";

    for (auto i = m_shape.begin(); i != m_shape.end(); ++i)
    {
        where << *i << ",";
    }
    where << ") Dtype: " << m_dtypeName;
}

TypeId
OpenGymTupleSpace::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymTupleSpace")
                            .SetParent<OpenGymSpace>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<OpenGymTupleSpace>();
    return tid;
}

OpenGymTupleSpace::OpenGymTupleSpace()
{
    NS_LOG_FUNCTION(this);
}

OpenGymTupleSpace::~OpenGymTupleSpace()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymTupleSpace::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymTupleSpace::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

bool
OpenGymTupleSpace::Add(Ptr<OpenGymSpace> space)
{
    NS_LOG_FUNCTION(this);
    m_tuple.push_back(space);
    return true;
}

Ptr<OpenGymSpace>
OpenGymTupleSpace::Get(uint32_t idx)
{
    NS_LOG_FUNCTION(this);
    Ptr<OpenGymSpace> space;

    if (idx < m_tuple.size())
    {
        space = m_tuple.at(idx);
    }

    return space;
}

ns3_ai_gym::SpaceDescription
OpenGymTupleSpace::GetSpaceDescription()
{
    NS_LOG_FUNCTION(this);
    ns3_ai_gym::SpaceDescription desc;
    desc.set_type(ns3_ai_gym::Tuple);

    ns3_ai_gym::TupleSpace tupleSpacePb;

    for (auto i = m_tuple.begin(); i != m_tuple.end(); ++i)
    {
        Ptr<OpenGymSpace> subSpace = *i;
        ns3_ai_gym::SpaceDescription subDesc = subSpace->GetSpaceDescription();
        tupleSpacePb.add_element()->CopyFrom(subDesc);
    }

    desc.mutable_space()->PackFrom(tupleSpacePb);
    return desc;
}

void
OpenGymTupleSpace::Print(std::ostream& where) const
{
    where << " TupleSpace: " << std::endl;

    for (auto i = m_tuple.begin(); i != m_tuple.end(); ++i)
    {
        where << "---";
        (*i)->Print(where);
        where << std::endl;
    }
}

TypeId
OpenGymDictSpace::GetTypeId()
{
    static TypeId tid = TypeId("OpenGymDictSpace")
                            .SetParent<OpenGymSpace>()
                            .SetGroupName("OpenGym")
                            .AddConstructor<OpenGymDictSpace>();
    return tid;
}

OpenGymDictSpace::OpenGymDictSpace()
{
    NS_LOG_FUNCTION(this);
}

OpenGymDictSpace::~OpenGymDictSpace()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymDictSpace::DoDispose()
{
    NS_LOG_FUNCTION(this);
}

void
OpenGymDictSpace::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

bool
OpenGymDictSpace::Add(std::string key, Ptr<OpenGymSpace> space)
{
    NS_LOG_FUNCTION(this);
    m_dict.insert(std::pair<std::string, Ptr<OpenGymSpace>>(key, space));
    return true;
}

Ptr<OpenGymSpace>
OpenGymDictSpace::Get(std::string key)
{
    NS_LOG_FUNCTION(this);
    Ptr<OpenGymSpace> space;
    std::map<std::string, Ptr<OpenGymSpace>>::iterator it = m_dict.find(key);
    if (it != m_dict.end())
    {
        space = it->second;
    }

    return space;
}

ns3_ai_gym::SpaceDescription
OpenGymDictSpace::GetSpaceDescription()
{
    NS_LOG_FUNCTION(this);
    ns3_ai_gym::SpaceDescription desc;
    desc.set_type(ns3_ai_gym::Dict);

    ns3_ai_gym::DictSpace dictSpacePb;

    std::map<std::string, Ptr<OpenGymSpace>>::iterator it;
    for (it = m_dict.begin(); it != m_dict.end(); ++it)
    {
        std::string name = it->first;
        Ptr<OpenGymSpace> subSpace = it->second;

        ns3_ai_gym::SpaceDescription subDesc = subSpace->GetSpaceDescription();
        subDesc.set_name(name);

        dictSpacePb.add_element()->CopyFrom(subDesc);
    }

    desc.mutable_space()->PackFrom(dictSpacePb);
    return desc;
}

void
OpenGymDictSpace::Print(std::ostream& where) const
{
    where << " DictSpace: " << std::endl;

    std::map<std::string, Ptr<OpenGymSpace>> myMap = m_dict;
    std::map<std::string, Ptr<OpenGymSpace>>::iterator it;
    for (it = myMap.begin(); it != myMap.end(); ++it)
    {
        where << "---" << it->first << ":";
        it->second->Print(where);
        where << std::endl;
    }
}

} // namespace ns3
