/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2009 Andrea Odetti

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#ifndef quantlib_american_path_montecarlo_engine_hpp
#define quantlib_american_path_montecarlo_engine_hpp

#include <ql/experimental/mcbasket/longstaffschwartzmultipathpricer.hpp>
#include <ql/experimental/mcbasket/mclongstaffschwartzpathengine.hpp>
#include <ql/experimental/mcbasket/pathmultiassetoption.hpp>
#include <ql/processes/blackscholesprocess.hpp>
#include <ql/processes/stochasticprocessarray.hpp>
#include <boost/function.hpp>

namespace QuantLib {

    //! least-square Monte Carlo engine
    /*! \warning This method is intrinsically weak for out-of-the-money
                 options.

        \ingroup basketengines
    */
    template <class RNG = PseudoRandom>
    class MCAmericanPathEngine
        : public MCLongstaffSchwartzPathEngine<
                            PathMultiAssetOption::engine,MultiVariate,RNG> {
      public:
        MCAmericanPathEngine(const boost::shared_ptr<StochasticProcessArray>&,
                               Size timeSteps,
                               Size timeStepsPerYear,
                               bool brownianBridge,
                               bool antitheticVariate,
                               Size requiredSamples,
                               Real requiredTolerance,
                               Size maxSamples,
                               BigNatural seed,
                               Size nCalibrationSamples = Null<Size>());
      protected:
        boost::shared_ptr<LongstaffSchwartzMultiPathPricer>
                                                      lsmPathPricer() const;
    };


    //! Monte Carlo American basket-option engine factory
    template <class RNG = PseudoRandom>
    class MakeMCAmericanPathEngine {
      public:
        MakeMCAmericanPathEngine(
                            const boost::shared_ptr<StochasticProcessArray>&);
        // named parameters
        MakeMCAmericanPathEngine& withSteps(Size steps);
        MakeMCAmericanPathEngine& withStepsPerYear(Size steps);
        MakeMCAmericanPathEngine& withBrownianBridge(bool b = true);
        MakeMCAmericanPathEngine& withAntitheticVariate(bool b = true);
        MakeMCAmericanPathEngine& withSamples(Size samples);
        MakeMCAmericanPathEngine& withAbsoluteTolerance(Real tolerance);
        MakeMCAmericanPathEngine& withMaxSamples(Size samples);
        MakeMCAmericanPathEngine& withSeed(BigNatural seed);
        MakeMCAmericanPathEngine& withCalibrationSamples(Size samples);
        // conversion to pricing engine
        operator boost::shared_ptr<PricingEngine>() const;
      private:
        boost::shared_ptr<StochasticProcessArray> process_;
        bool brownianBridge_, antithetic_;
        Size steps_, stepsPerYear_, samples_, maxSamples_, calibrationSamples_;
        Real tolerance_;
        BigNatural seed_;
    };


    template <class RNG> inline
    MCAmericanPathEngine<RNG>::MCAmericanPathEngine(
                   const boost::shared_ptr<StochasticProcessArray>& processes,
                   Size timeSteps,
                   Size timeStepsPerYear,
                   bool brownianBridge,
                   bool antitheticVariate,
                   Size requiredSamples,
                   Real requiredTolerance,
                   Size maxSamples,
                   BigNatural seed,
                   Size nCalibrationSamples)
        : MCLongstaffSchwartzPathEngine<PathMultiAssetOption::engine,
                                    MultiVariate,RNG>(processes,
                                                      timeSteps,
                                                      timeStepsPerYear,
                                                      brownianBridge,
                                                      antitheticVariate,
                                                      false,
                                                      requiredSamples,
                                                      requiredTolerance,
                                                      maxSamples,
                                                      seed,
                                                      nCalibrationSamples) {}

    template <class RNG>
    inline boost::shared_ptr<LongstaffSchwartzMultiPathPricer>
    MCAmericanPathEngine<RNG>::lsmPathPricer() const {

        boost::shared_ptr<StochasticProcessArray> processArray =
            boost::dynamic_pointer_cast<StochasticProcessArray>(this->process_);
        QL_REQUIRE(processArray && processArray->size()>0,
                   "Stochastic process array required");

        boost::shared_ptr<GeneralizedBlackScholesProcess> process =
            boost::dynamic_pointer_cast<GeneralizedBlackScholesProcess>(
               processArray->process(0));
        QL_REQUIRE(process, "generalized Black-Scholes process required");

        const TimeGrid theTimeGrid = this->timeGrid();

        const std::vector<Time> & times = theTimeGrid.mandatoryTimes();
        const Size numberOfTimes = times.size();

        std::vector<Size> timePositions(numberOfTimes);
        Array discountFactors(numberOfTimes);

        for (Size i = 0; i < numberOfTimes; ++i) {
            timePositions[i] = theTimeGrid.index(times[i]);
            discountFactors[i] = process->riskFreeRate()->discount(times[i]);
        }

        const Size polynomialOrder = 2;
        const LsmBasisSystem::PolynomType polynomType = LsmBasisSystem::Monomial;

        return boost::shared_ptr<LongstaffSchwartzMultiPathPricer> (
            new LongstaffSchwartzMultiPathPricer(this->arguments_.payoff,
                                                 timePositions,
                                                 discountFactors,
                                                 polynomialOrder,
                                                 polynomType));
    }


    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>::MakeMCAmericanPathEngine(
                     const boost::shared_ptr<StochasticProcessArray>& process)
    : process_(process), brownianBridge_(false), antithetic_(false),
      steps_(Null<Size>()), stepsPerYear_(Null<Size>()),
      samples_(Null<Size>()), maxSamples_(Null<Size>()),
      calibrationSamples_(Null<Size>()),
      tolerance_(Null<Real>()), seed_(0) {}

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withSteps(Size steps) {
        steps_ = steps;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withStepsPerYear(Size steps) {
        stepsPerYear_ = steps;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withBrownianBridge(bool brownianBridge) {
        brownianBridge_ = brownianBridge;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withAntitheticVariate(bool b) {
        antithetic_ = b;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withSamples(Size samples) {
        QL_REQUIRE(tolerance_ == Null<Real>(),
                   "tolerance already set");
        samples_ = samples;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withAbsoluteTolerance(Real tolerance) {
        QL_REQUIRE(samples_ == Null<Size>(),
                   "number of samples already set");
        QL_REQUIRE(RNG::allowsErrorEstimate,
                   "chosen random generator policy "
                   "does not allow an error estimate");
        tolerance_ = tolerance;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withMaxSamples(Size samples) {
        maxSamples_ = samples;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withSeed(BigNatural seed) {
        seed_ = seed;
        return *this;
    }

    template <class RNG>
    inline MakeMCAmericanPathEngine<RNG>&
    MakeMCAmericanPathEngine<RNG>::withCalibrationSamples(Size samples) {
        calibrationSamples_ = samples;
        return *this;
    }

    template <class RNG>
    inline
    MakeMCAmericanPathEngine<RNG>::operator
    boost::shared_ptr<PricingEngine>() const {
        QL_REQUIRE(steps_ != Null<Size>() || stepsPerYear_ != Null<Size>(),
                   "number of steps not given");
        QL_REQUIRE(steps_ == Null<Size>() || stepsPerYear_ == Null<Size>(),
                   "number of steps overspecified");
        return boost::shared_ptr<PricingEngine>(new
            MCAmericanPathEngine<RNG>(process_,
                                        steps_,
                                        stepsPerYear_,
                                        brownianBridge_,
                                        antithetic_,
                                        samples_,
                                        tolerance_,
                                        maxSamples_,
                                        seed_,
                                        calibrationSamples_));
    }

}

#endif