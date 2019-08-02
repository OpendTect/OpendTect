#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "samplingdata.h"
#include "valseriesevent.h"

class BinnedValueSet;
class SeisTrc;
namespace Seis { class MSCProvider; }


mExpClass(Seis) SeisEventSnapper : public Executor
{
public:

    typedef VSEvent::Type	EvType;

			SeisEventSnapper( const Interval<float>& gate);

    void		setEvent( EvType tp )	{ eventtype_ = tp; }
    EvType		getEvent() const	{ return eventtype_; }

    void		setSearchGate( const Interval<float>& gate )
						{ searchgate_ = gate; }
    Interval<float>	getSearchGate() const	{ return searchgate_; }

    virtual od_int64	totalNr() const		{ return totalnr_; }
    virtual od_int64	nrDone() const		{ return nrdone_; }

    static float	findNearestEvent(const SeisTrc&,float tarz,
					 const Interval<float>& srchgt,EvType);

protected:

    float		findNearestEvent(const SeisTrc&,float tarz) const;

    Interval<float>	searchgate_;
    EvType		eventtype_;

    int			totalnr_;
    int			nrdone_;

};


mExpClass(Seis) SeisEventSnapper3D : public SeisEventSnapper
{
public:
			SeisEventSnapper3D(const IOObj&,BinnedValueSet&,
					   const Interval<float>& gate);
			~SeisEventSnapper3D();

    uiString		message() const;
    uiString		nrDoneText() const;

protected:

    virtual int		nextStep();

    BinnedValueSet&	positions_;
    Seis::MSCProvider*	mscprov_;

};
