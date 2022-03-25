#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "trckeyzsampling.h"
#include "datadistribution.h"

class SeisTrc;
namespace Stats { class RandGen; }

namespace Seis
{


mExpClass(Seis) StatsCollector
{
public:

    typedef FloatDistrib DistribType;

			StatsCollector(int icomp=-1);
			~StatsCollector();

    void		setEmpty();

    void		useTrace(const SeisTrc&);
    od_int64		nrSamplesUsed() const	{ return nrvalshandled_; }

    const TrcKeyZSampling& trcKeyZSampling() const	{ return tkzs_; }
    DistribType&	distribution();
    const DistribType&	distribution() const { return mSelf().distribution(); }

    bool		fillPar(IOPar&) const;

    static RefMan<DistribType>	getDistribution(const IOPar&);
    static Interval<float>	getExtremes(const IOPar&);
    static od_int64		getNrTraces(const IOPar&);
    static od_int64		getNrSamples(const IOPar&,bool valid=true);

protected:


    float*		vals_;
    Interval<float>	valrg_;
    Interval<float>	offsrg_;
    const int		selcomp_;
    od_int64		nrtrcshandled_;
    od_int64		nrvalshandled_;
    od_int64		totalnrsamples_;
    int			nrvalscollected_;
    TrcKeyZSampling	tkzs_;
    RefMan<DistribType>	distrib_;
    Stats::RandGen&	gen_;

    bool		finish() const;
    void		addPosition(const TrcKey&,const Interval<float>&);

};

} // namespace Seis
