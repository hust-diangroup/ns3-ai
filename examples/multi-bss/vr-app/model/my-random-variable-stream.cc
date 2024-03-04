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
#include "my-random-variable-stream.h"

#include <ns3/assert.h>
#include <ns3/boolean.h>
#include <ns3/double.h>
#include <ns3/integer.h>
#include <ns3/log.h>
#include <ns3/object-factory.h>
#include <ns3/pointer.h>
#include <ns3/rng-seed-manager.h>
#include <ns3/rng-stream.h>
#include <ns3/string.h>
// #include <ns3/unused.h>

#include <algorithm> // upper_bound
#include <cmath>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MyRandomVariableStream");

NS_OBJECT_ENSURE_REGISTERED(LogisticRandomVariable);

const double LogisticRandomVariable::INFINITE_VALUE = 1e307;

TypeId
LogisticRandomVariable::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::LogisticRandomVariable")
            .SetParent<RandomVariableStream>()
            .SetGroupName("Core")
            .AddConstructor<LogisticRandomVariable>()
            .AddAttribute(
                "Location",
                "The location value for the logistic distribution returned by this RNG stream.",
                DoubleValue(0.0),
                MakeDoubleAccessor(&LogisticRandomVariable::m_location),
                MakeDoubleChecker<double>())
            .AddAttribute(
                "Scale",
                "The scale value for the logistic distribution returned by this RNG stream.",
                DoubleValue(1.0),
                MakeDoubleAccessor(&LogisticRandomVariable::m_scale),
                MakeDoubleChecker<double>())
            .AddAttribute("Bound",
                          "The bound on the values returned by this RNG stream.",
                          DoubleValue(INFINITE_VALUE),
                          MakeDoubleAccessor(&LogisticRandomVariable::m_bound),
                          MakeDoubleChecker<double>());
    return tid;
}

LogisticRandomVariable::LogisticRandomVariable()
{
    // m_location, m_scale, and m_bound are initialized after constructor
    // by attributes
    NS_LOG_FUNCTION(this);
}

double
LogisticRandomVariable::GetLocation(void) const
{
    NS_LOG_FUNCTION(this);
    return m_location;
}

double
LogisticRandomVariable::GetScale(void) const
{
    NS_LOG_FUNCTION(this);
    return m_scale;
}

double
LogisticRandomVariable::GetBound(void) const
{
    NS_LOG_FUNCTION(this);
    return m_bound;
}

double
LogisticRandomVariable::GetValue(double location, double scale, double bound)
{
    NS_LOG_FUNCTION(this << location << scale << bound);

    while (1)
    {
        // Get a uniform random variable in [0,1].
        double v = Peek()->RandU01();
        if (IsAntithetic())
        {
            v = (1 - v);
        }

        // Calculate the logistic random variable.
        double x = location + scale * std::log(v / (1 - v));

        // Use this value if it's acceptable.
        if (std::fabs(x - m_location) <= bound)
        {
            return x;
        }
    }
}

uint32_t
LogisticRandomVariable::GetInteger(uint32_t location, uint32_t scale, uint32_t bound)
{
    NS_LOG_FUNCTION(this << location << scale << bound);
    return static_cast<uint32_t>(GetValue(location, scale, bound));
}

double
LogisticRandomVariable::GetValue(void)
{
    NS_LOG_FUNCTION(this);
    return GetValue(m_location, m_scale, m_bound);
}

uint32_t
LogisticRandomVariable::GetInteger(void)
{
    NS_LOG_FUNCTION(this);
    return (uint32_t)GetValue(m_location, m_scale, m_bound);
}

NS_OBJECT_ENSURE_REGISTERED(MixtureRandomVariable);

TypeId
MixtureRandomVariable::GetTypeId(void)
{
    static TypeId tid = TypeId("ns3::MixtureRandomVariable")
                            .SetParent<RandomVariableStream>()
                            .SetGroupName("Core")
                            .AddConstructor<MixtureRandomVariable>();
    return tid;
}

MixtureRandomVariable::MixtureRandomVariable(void)
{
    NS_LOG_FUNCTION(this);
}

MixtureRandomVariable::~MixtureRandomVariable(void)
{
    NS_LOG_FUNCTION(this);
    m_wCdf = 0;
    m_rvs.clear();
}

void
MixtureRandomVariable::SetRvs(std::vector<double> weightsCdf,
                              std::vector<Ptr<RandomVariableStream>> rvs)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(weightsCdf.size() != rvs.size(),
                    "CDF of weights and random variables must have the same size");

    m_rvs = rvs;

    // create empirical random variable to ease the extraction of m_rvs
    m_wCdf =
        CreateObjectWithAttributes<EmpiricalRandomVariable>("Interpolate", BooleanValue(false));
    for (uint32_t i = 0; i < weightsCdf.size(); i++)
    {
        m_wCdf->CDF(i, weightsCdf[i]);
    }
}

uint32_t
MixtureRandomVariable::GetInteger(void)
{
    NS_LOG_FUNCTION(this);
    return static_cast<uint32_t>(GetValue());
}

double
MixtureRandomVariable::GetValue(void)
{
    NS_LOG_FUNCTION(this);

    uint32_t rvIdx = m_wCdf->GetInteger();
    double value = m_rvs[rvIdx]->GetValue();

    return value;
}

} // namespace ns3
