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


mExpClass(Seis) SeisStatsCollector
{
public:

    typedef DataDistribution<float> DistribType;

			SeisStatsCollector(int icomp=-1);
			~SeisStatsCollector();

    void		setEmpty();

    void		useTrace(const SeisTrc&);

    const TrcKeyZSampling& trcKeyZSampling() const	{ return tkzs_; }
    const DistribType&	distribution() const;

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

    bool		finish() const;
    void		addPosition(const TrcKey&,const Interval<float>&);

};
