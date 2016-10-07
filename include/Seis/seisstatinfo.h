#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		September 2016
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "statparallelcalc.h"

template <class T> class Array2DImpl;
class SeisTrc;

mExpClass(Seis) SeisStatInfo
{
public:
			SeisStatInfo();
			~SeisStatInfo();

    void		setEmpty();
    uiString		uiMessage() const	{ return errmsg_; }

    void		useTrace(const SeisTrc&);
    bool		fillPar(IOPar&) const;

protected:

    void		fillStats(IOPar&,const TypeSet<float>& histdata,
				  const Interval<float>& xrg) const;

    Stats::ParallelCalc<float>& rc_;
    mutable uiString		errmsg_;
    PtrMan<Array2DImpl<float> > trcvals_;
    Interval<float>		valrange_;
    int				nrvals_;
};
