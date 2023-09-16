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

namespace EM { class Horizon3D; }
class IOObj;
class SeisMSCProvider;
class SeisTrc;

mExpClass(Seis) SeisEventSnapper : public Executor
{ mODTextTranslationClass(SeisEventSnapper);
public:
				SeisEventSnapper(const Interval<float>& gate,
						 bool eraseundef=true);
				~SeisEventSnapper();

    void			setEvent( VSEvent::Type tp )
    				{ eventtype_ = tp; }
    VSEvent::Type		getEvent() const	{ return eventtype_; }

    void			setSearchGate( const Interval<float>& gate )
				{ searchgate_ = gate; }
    const Interval<float>&	getSearchGate() const	{ return searchgate_; }

    od_int64			totalNr() const override { return totalnr_; }
    od_int64			nrDone() const override  { return nrdone_; }
    uiString			uiNrDoneText() const;

protected:

    float			findNearestEvent(const SeisTrc&,
	    					 float tarz) const;

    Interval<float>		searchgate_;
    VSEvent::Type		eventtype_;
    bool			eraseundef_	= true;

    int				totalnr_	= 0;
    int				nrdone_		= 0;

};


mExpClass(Seis) SeisEventSnapper3D : public SeisEventSnapper
{
public:
			SeisEventSnapper3D(const IOObj&,
					   const EM::Horizon3D&,
					   EM::Horizon3D&,
					   const Interval<float>& gate,
					   bool eraseundef=true);
				~SeisEventSnapper3D();

protected:
    int				nextStep() override;

    SeisMSCProvider*		mscprov_;

    ConstRefMan<EM::Horizon3D>	inhorizon_;
    RefMan<EM::Horizon3D>	outhorizon_;

};
