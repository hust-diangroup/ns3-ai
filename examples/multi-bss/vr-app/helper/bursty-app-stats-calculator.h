/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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
 *
 */

#ifndef BURSTY_APP_STATS_CALCULATOR_H_
#define BURSTY_APP_STATS_CALCULATOR_H_

#include "ns3/basic-data-calculators.h"
#include "ns3/internet-module.h"
#include "ns3/lte-common.h"
#include "ns3/network-module.h"
#include "ns3/object.h"
#include "ns3/seq-ts-size-frag-header.h"
#include "ns3/uinteger.h"

#include <fstream>
#include <map>
#include <string>

namespace ns3
{

struct AppResults
{
    uint16_t imsi;
    uint16_t txBursts;
    uint32_t txData;
    uint16_t rxBursts;
    uint32_t rxData;
    double delayMean;
    double delayStdev;
    double delayMin;
    double delayMax;
};

/**
 * \ingroup application
 *
 */
class BurstyAppStatsCalculator : public Object
{
  public:
    /**
     * Class constructor
     */
    BurstyAppStatsCalculator();

    /**
     * Class constructor
     */
    BurstyAppStatsCalculator(std::string protocolType);

    /**
     * Class destructor
     */
    virtual ~BurstyAppStatsCalculator();

    // Inherited from ns3::Object
    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId(void);
    void DoDispose();

    /**
     *
     * \param e the epoch duration
     */
    void SetEpoch(Time e);

    /**
     *
     * \return the epoch duration
     */
    Time GetEpoch() const;

    /**
     *
     * \param t the value of the StartTime attribute
     */
    void SetStartTime(Time t);

    /**
     *
     * \return the value of the StartTime attribute
     */
    Time GetStartTime() const;

    /**
     * Reschedules EndEpoch event. Usually used after
     * execution of SetStartTime() or SetEpoch()
     */
    void RescheduleEndEpoch();

    /**
     *
     * \param u set the update mode (true if manual, automatic otherwise)
     */
    void SetManualUpdate(bool u);

    /**
     *
     * \return whether the update is manual or not
     */
    bool GetManualUpdate() const;

    /**
     * Notifies the stats calculator that a burst of packets has been transmitted.
     * \param nodeId ID of the node sending the burst
     * \param burst packet representing the complete burst
     * \param from address of the transmitting terminal
     * \param to address of the receiving terminal
     * \param header burst header containing trasmission information
     */
    void TxBurst(uint32_t nodeId,
                 Ptr<const Packet> burst,
                 const Address& from,
                 const Address& to,
                 const SeqTsSizeFragHeader& header);

    /**
     * Notifies the stats calculator that a burst of packets has been transmitted.
     * \param nodeId ID of the node receiving the burst
     * \param burst packet representing the complete burst
     * \param from address of the transmitting terminal
     * \param to address of the receiving terminal
     * \param header burst header containing trasmission information
     */
    void RxBurst(uint32_t nodeId,
                 Ptr<const Packet> burst,
                 const Address& from,
                 const Address& to,
                 const SeqTsSizeFragHeader& header);

    /**
     *
     */
    std::map<uint16_t, AppResults> ReadResults(void);

    /**
     * Called after each epoch to write collected
     * statistics to output files. During first call
     * it opens output files and write columns descriptions.
     * During next calls it opens output files in append mode.
     */
    void ShowResults(void);

    /**
     * Writes collected statistics to output file and closes it.
     * @param outFile ofstream for statistics
     */
    void WriteResults(std::ofstream& outFile);

    /**
     * Erases collected statistics
     */
    void ResetResults();

    /**
     *
     * \param filename name of the output file
     */
    void SetOutputFilename(std::string filename);

    /**
     *
     * \return name of the output file
     */
    std::string GetOutputFilename() const;

  private:
    /**
     * Function called in every endEpochEvent. It calls
     * ShowResults() to write statistics to output files
     * and ResetResults() to clear collected statistics.
     */
    void EndEpoch(void);

    EventId m_endEpochEvent; //!< Event id for next end epoch event
    Time m_startTime;        //!< Start time of the on going epoch
    Time m_epochDuration;    //!< Epoch duration

    bool m_firstWrite;      //! true if output files have not been opened yet
    bool m_pendingOutput;   //!< true if any output is pending
    bool m_aggregatedStats; //!< true if results are shown aggregated
    bool m_manualUpdate;    //!< true if update is triggered by external entity (such as RAN-AI)
    bool m_writeToFile; //!< determine if the traces must be output to file or just evaluated and
                        //!< exchanged with external classes

    std::map<uint32_t, uint32_t>
        m_txBursts; //!< number of bursts sent in a specific epoch per node ID
    std::map<uint32_t, uint64_t> m_txData; //!< number of bytes sent in a specific epoch per node ID

    std::map<uint32_t, uint32_t>
        m_rxBursts; //!< number of bursts received in a specific epoch per node ID
    std::map<uint32_t, uint64_t>
        m_rxData; //!< number of bytes received in a specific epoch per node ID

    std::map<uint32_t, Ptr<MinMaxAvgTotalCalculator<uint64_t>>>
        m_delay; //!< delay statistics calculator for a specific epoch, per node ID

    std::string m_outputFilename; //!< name of the output file
};

} // namespace ns3

#endif /* BURSTY_APP_STATS_CALCULATOR_H_ */
