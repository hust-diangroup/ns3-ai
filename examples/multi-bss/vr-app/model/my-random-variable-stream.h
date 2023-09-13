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
#ifndef MY_RANDOM_VARIABLE_STREAM_H
#define MY_RANDOM_VARIABLE_STREAM_H

#include <ns3/attribute-helper.h>
#include <ns3/object.h>
#include <ns3/random-variable-stream.h>
#include <ns3/type-id.h>

#include <stdint.h>

namespace ns3
{
/**
 * \ingroup randomvariable
 * \brief The logistic distribution Random Number Generator (RNG).
 *
 * This class supports the creation of objects that return random numbers
 * from a fixed logistic distribution.
 *
 * The density probability function is defined over the interval (\f$-\infty\f$,\f$+\infty\f$)
 * as: \f$ \frac{e^{-(x-\mu)/s}}{s(1 + e^{-(x-\mu)/s})} \f$
 * where \f$ location = \mu \f$ and \f$ scale = s \f$
 *
 * Since logistic distributions can theoretically return unbounded
 * values, it is sometimes useful to specify a fixed bound. The
 * LogisticRandomVariable is bounded symmetrically about the mean (location) by
 * this bound, i.e. its values are confined to the interval
 * [\f$mean-bound\f$,\f$mean+bound\f$].
 *
 * Here is an example of how to use this class:
 * \code
 *   double location = 5.0;
 *   double scale = 2.0;
 *
 *   Ptr<LogisticRandomVariable> x = CreateObject<LogisticRandomVariable> ();
 *   x->SetAttribute ("Location", DoubleValue (location));
 *   x->SetAttribute ("Scale", DoubleValue (scale));
 *
 *   // The expected value for the mean of the values returned by a
 *   // logistic distributed random variable is equal to location.
 *   double value = x->GetValue ();
 * \endcode
 */
class LogisticRandomVariable : public RandomVariableStream
{
  public:
    /** Large constant to bound the range. */
    static const double INFINITE_VALUE;

    /**
     * \brief Register this type.
     * \return The object TypeId.
     */
    static TypeId GetTypeId(void);

    /**
     * \brief Creates a laplace distribution RNG with the default
     * values for the location and scale.
     */
    LogisticRandomVariable();

    /**
     * Get the location parameter of the distribution
     * \return the location parameter
     */
    double GetLocation(void) const;

    /**
     * Get the scale parameter of the distribution
     * \return the scale parameter
     */
    double GetScale(void) const;

    /**
     * \brief Returns the bound on values that can be returned by this RNG stream.
     * \return The bound on values that can be returned by this RNG stream.
     */
    double GetBound(void) const;

    /**
     * \brief Get the next random value, as a double within the specified bound
     * \f$[location - bound, location + max]\f$.
     *
     * \param location the location of the logistic random variable
     * \param scale the scale of the logistic random variable
     * \param bound the bound of the logistic random variable
     * \return A floating point random value.
     */
    double GetValue(double location,
                    double scale,
                    double bound = LogisticRandomVariable::INFINITE_VALUE);

    /**
     * \brief Get the next random value, as an unsigned integer within the specified bound
     * \f$[location - bound, location + max]\f$.
     *
     * \param location the location of the logistic random variable
     * \param scale the scale of the logistic random variable
     * \param bound the bound of the logistic random variable
     * \return A random unsigned integer value.
     */
    uint32_t GetInteger(uint32_t location, uint32_t scale, uint32_t bound);

    // Inherited from RandomVariableStream
    /**
     * \brief Get the next random value as a double drawn from the distribution.
     * \return A floating point random value.
     */
    virtual double GetValue(void) override;
    /**
     * \brief Get the next random value as an unsigned integer drawn from the distribution.
     * \return A random unsigned integer value.
     */
    virtual uint32_t GetInteger(void) override;

  private:
    double m_location; //!< The location value of the logistic distribution.
    double m_scale;    //!< The scale of the logistic distribution.
    double m_bound;    //!< The bound on values that can be returned by this RNG stream.

}; // class LogisticRandomVariable

/**
 * \ingroup randomvariable
 * \brief The mixture Random Number Generator (RNG).
 *
 * A mixture distribution is the probability distribution of a random
 * variable that is derived from a collection of other random variables
 * as follows: first, a random variable is selected by chance from the
 * collection according to given probabilities of selection, and then
 * the value of the selected random variable is realized.
 *
 * Here is an example of how to use this class:
 * \code
 *   // setup weights cdf
 *   std::vector<double> weightsCdf {0.7, 1.0}; // p1 = 0.7, p2 = 0.3
 *   // setup random variables
 *   std::vector<Ptr<RandomVariableStream> > rvs;
 *   rvs.push_back (CreateObjectWithAttributes<NormalRandomVariable> ("Mean", DoubleValue (5),
 * "Variance", DoubleValue (1))); rvs.push_back (CreateObjectWithAttributes<NormalRandomVariable>
 * ("Mean", DoubleValue (10), "Variance", DoubleValue (4)));
 *
 *   Ptr<MixtureRandomVariable> x = CreateObject<MixtureRandomVariable> ();
 *   x->SetRvs (weightsCdf, rvs);
 *   double value = x->GetValue ();
 * \endcode
 */
class MixtureRandomVariable : public RandomVariableStream
{
  public:
    /**
     * \brief Register this type.
     * \return The object TypeId.
     */
    static TypeId GetTypeId(void);

    /**
     * \brief Creates an empty mixture distribution RNG
     */
    MixtureRandomVariable();
    ~MixtureRandomVariable();

    /**
     * Set the random variables and their respective probabilities as weights of a CDF.
     * Check the documentation of EmpiricalRandomVariable for more information regarding weightsCdf.
     *
     * \param weightsCdf the cumulative distribution function of the mixture of random variables
     * \param rvs the RandomVariableStreams to draw from
     */
    void SetRvs(std::vector<double> weightsCdf, std::vector<Ptr<RandomVariableStream>> rvs);

    // Inherited from RandomVariableStream
    /**
     * \brief Get the next random value as a double drawn from the distribution.
     * \return A floating point random value.
     */
    virtual double GetValue(void) override;
    /**
     * \brief Get the next random value as an unsigned integer drawn from the distribution.
     * \return A random unsigned integer value.
     */
    virtual uint32_t GetInteger(void) override;

  private:
    Ptr<EmpiricalRandomVariable> m_wCdf{
        0}; //!< The random variable that extracts the index of one of the RandomVariablStreams
    std::vector<Ptr<RandomVariableStream>>
        m_rvs; //!< The vector of RandomVariableStreams to draw from

}; // class MixtureRandomVariable

} // namespace ns3

#endif /* MY_RANDOM_VARIABLE_STREAM_H */
