#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "samplingdata.h"
#include "valseriesevent.h"

class BinIDValueSet;
class IOObj;
class SeisMSCProvider;
class SeisTrc;

mExpClass(Seis) SeisEventSnapper : public Executor
{
public:
				SeisEventSnapper( const Interval<float>& gate);

    void			setEvent( VSEvent::Type tp )
    				{ eventtype_ = tp; }
    VSEvent::Type		getEvent() const	{ return eventtype_; }

    void			setSearchGate( const Interval<float>& gate )
				{ searchgate_ = gate; }
    const Interval<float>&	getSearchGate() const	{ return searchgate_; }

    od_int64			totalNr() const override { return totalnr_; }
    od_int64			nrDone() const override  { return nrdone_; }

protected:

    float			findNearestEvent(const SeisTrc&,
	    					 float tarz) const;

    Interval<float>		searchgate_;
    VSEvent::Type		eventtype_;

    int				totalnr_;
    int				nrdone_;

};


mExpClass(Seis) SeisEventSnapper3D : public SeisEventSnapper
{
public:
				SeisEventSnapper3D(const IOObj&,BinIDValueSet&,
						   const Interval<float>& gate);
				~SeisEventSnapper3D();

protected:
    int				nextStep() override;

    BinIDValueSet&		positions_;
    SeisMSCProvider*		mscprov_;
};
