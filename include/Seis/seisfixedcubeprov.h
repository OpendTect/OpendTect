#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jan 2007
________________________________________________________________________

*/


#include "seisdatapack.h"
#include "perthreadrepos.h"
#include "survsubsel.h"
#include "uistring.h"

template <class T> class Array2D;
class SeisTrc;
class TaskRunner;


mExpClass(Seis) SeisFixedCubeProvider
{ mODTextTranslationClass(SeisFixedCubeProvider);
public:

    typedef double	dist_type;
    mUseType( Survey,	GeomSubSel );
    mUseType( Survey::HorSubSel, trcnr_type );

			SeisFixedCubeProvider(const DBKey&);
			~SeisFixedCubeProvider();

    void		clear();
    bool		isEmpty() const;
    bool		readData(const GeomSubSel&,TaskRunner* tskr=0);

    const SeisTrc*	getTrace(const BinID&) const;
    const SeisTrc*	getTrace(trcnr_type) const;
    const SeisTrc*	getTrace(const TrcKey&) const;
    dist_type		getTrcDist() const		{ return trcdist_; }
    uiString		errMsg() const;

protected:

    Array2D<SeisTrc*>*	data_		= nullptr;;
    RefMan<SeisVolumeDataPack> seisdp_;
    mutable PerThreadObjectRepository<SeisTrc> dptrc_;

    GeomSubSel*		gss_		= nullptr;
    IOObj*		ioobj_		= nullptr;
    uiString		errmsg_;
    dist_type		trcdist_;

    bool		calcTrcDist(const Pos::GeomID);

};
