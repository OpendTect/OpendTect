#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2014
________________________________________________________________________

-*/


#include "seiscommon.h"
#include "executor.h"
#include "geomid.h"

class Scaler;
class SeisSingleTraceProc;
namespace Seis { class Provider; class Storer; class RangeSelData; }


/*!\brief Copies cubes. The IOPar constructor wants an IOPar as you would pass
	to a SeisSingleTraceProc. */

mExpClass(Seis) SeisCubeCopier : public Executor
{ mODTextTranslationClass(SeisCubeCopier);
public:

				SeisCubeCopier(const IOObj& inobj,
					    const IOObj& outobj,const IOPar&,
					    int compnr=-1);
				SeisCubeCopier(SeisSingleTraceProc*,
						int compnr=-1);
						//!< trcproc becomes mine
				~SeisCubeCopier();

    od_int64			totalNr() const;
    od_int64			nrDone() const;
    uiString			message() const;
    uiString			nrDoneText() const;
    int				nextStep();

protected:

    SeisSingleTraceProc*	stp_;
    uiString			errmsg_;
    int				compnr_;
    int				veltype_;

    void			doProc(CallBacker*);

private:

    void			init();

};


/*!\brief Copies line sets.

  Expects an IOPar with subpars Line.N.*, each of those containing:
  - sKey::GeomID()
  - sKey::TrcRange()
  - sKey::ZRange()
  Failing to provide one of those will exclude the line.

*/

mExpClass(Seis) Seis2DCopier : public Executor
{ mODTextTranslationClass(Seis2DCopier);
public:

    mUseType( Seis,	RangeSelData );
    mUseType( Seis,	Provider );
    mUseType( Seis,	Storer );

			Seis2DCopier(const IOObj& inobj,const IOObj& outob);
			Seis2DCopier(const IOObj& inobj,const IOObj& outob,
					  const IOPar&);

			~Seis2DCopier();

    RangeSelData&	selData()		{ return seldata_; }

    od_int64		totalNr() const		{ return totalnr_; }
    od_int64		nrDone() const		{ return nrdone_; }
    uiString		message() const;
    uiString		nrDoneText() const;
    int			nextStep();

protected:

    Provider*		prov_;
    Storer&		storer_;
    uiRetVal		uirv_;
    RangeSelData&	seldata_;
    bool		inited_			= false;

    Scaler*		scaler_			= 0;
    od_int64		nrdone_			= 0;
    od_int64		totalnr_		= 0;

private:

    bool		init();

};
