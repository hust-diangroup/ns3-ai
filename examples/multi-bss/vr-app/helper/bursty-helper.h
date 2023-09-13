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
#ifndef BURSTY_HELPER_H
#define BURSTY_HELPER_H

#include "ns3/address.h"
#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/bursty-application.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"

#include <stdint.h>
#include <string>

namespace ns3
{

class DataRate;

/**
 * \ingroup bursty
 * \brief A helper to make it easier to instantiate an ns3::BurstyApplication
 * on a set of nodes.
 */
class BurstyHelper
{
  public:
    /**
     * Create an BurstyHelper to make it easier to work with BurstyApplications
     *
     * \param protocol the name of the protocol to use to send traffic
     *        by the applications. This string identifies the socket
     *        factory type used to create sockets for the applications.
     *        A typical value would be ns3::UdpSocketFactory.
     * \param address the address of the remote node to send traffic
     *        to.
     */
    BurstyHelper(std::string protocol, Address address);

    /**
     * Helper function used to set the underlying application attributes.
     *
     * \param name the name of the application attribute to set
     * \param value the value of the application attribute to set
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    /**
     * Each BurstyApplication must have a BurstGenerator.
     * This method allows one to set the type of the BurstGenerator that is automatically
     * created when the application is created and attached to a node.
     *
     * \param type the type of BurstGenerator
     * \param n1 the name of the attribute to set on the BurstGenerator
     * \param v1 the value of the attribute to set on the BurstGenerator
     * \param n2 the name of the attribute to set on the BurstGenerator
     * \param v2 the value of the attribute to set on the BurstGenerator
     * \param n3 the name of the attribute to set on the BurstGenerator
     * \param v3 the value of the attribute to set on the BurstGenerator
     * \param n4 the name of the attribute to set on the BurstGenerator
     * \param v4 the value of the attribute to set on the BurstGenerator
     *
     * Set the type of BurstGenerator to create and associated to each
     * BurstApplication created through BurstyHelper::Install.
     */
    void SetBurstGenerator(std::string type,
                           std::string n1 = "",
                           const AttributeValue& v1 = EmptyAttributeValue(),
                           std::string n2 = "",
                           const AttributeValue& v2 = EmptyAttributeValue(),
                           std::string n3 = "",
                           const AttributeValue& v3 = EmptyAttributeValue(),
                           std::string n4 = "",
                           const AttributeValue& v4 = EmptyAttributeValue());

    /**
     * \param n1 the name of the attribute to set
     * \param v1 the value of the attribute to set
     *
     * Set these attributes on each ns3::BurstGenerator created
     * by BurstyHelper::Install
     */
    void SetBurstGeneratorAttribute(std::string n1, const AttributeValue& v1);

    /**
     * Install an ns3::BurstyApplication on each node of the input container
     * configured with all the attributes set with SetAttribute.
     *
     * \param c NodeContainer of the set of nodes on which an BurstyApplication
     * will be installed.
     * \returns Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(NodeContainer c) const;

    /**
     * Install an ns3::BurstyApplication on the node configured with all the
     * attributes set with SetAttribute.
     *
     * \param node The node on which an BurstyApplication will be installed.
     * \returns Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(Ptr<Node> node) const;

    /**
     * Install an ns3::BurstyApplication on the node configured with all the
     * attributes set with SetAttribute.
     *
     * \param nodeName The node on which an BurstyApplication will be installed.
     * \returns Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(std::string nodeName) const;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.  The Install() method should have previously been
     * called by the user.
     *
     * \param stream first stream index to use
     * \param c NodeContainer of the set of nodes for which the BurstyApplication
     *          should be modified to use a fixed stream
     * \return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

  private:
    /**
     * Install an ns3::BurstyApplication on the node configured with all the
     * attributes set with SetAttribute.
     *
     * \param node The node on which an BurstyApplication will be installed.
     * \returns Ptr to the application installed.
     */
    Ptr<Application> InstallPriv(Ptr<Node> node) const;

    ObjectFactory m_burstyApplicationFactory; //!< BurstyApplication factory
    ObjectFactory m_burstGeneratorFactory;    //!< BurstGenerator factory
};

} // namespace ns3

#endif /* BURSTY_HELPER_H */
