#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		January 2008
________________________________________________________________________

*/


#include "seiscommon.h"
#include "ranges.h"
#include "executor.h"

class CubeHorSubSel;
class LineHorSubSel;
class SeisTrc;
class ZAxisTransform;
namespace Seis { class Provider; class RangeSelData; class Storer; }


/*!Stretches the zaxis from the input cube with a ZAxisTransform and writes it
   out into another volume. If stretchz is false, the stretching will
   be done on the inverse of the values. */

mExpClass(Seis) SeisZAxisStretcher : public Executor
{ mODTextTranslationClass(SeisZAxisStretcher);
public:

    mUseType( Seis,			Provider );
    mUseType( Seis,			RangeSelData );
    typedef Pos::Z_Type			z_type;
    typedef StepInterval<z_type>	z_steprg_type;

			SeisZAxisStretcher(const IOObj& in,const IOObj& out,
					   ZAxisTransform&,
					   bool forward,bool stretchz,
					   const RangeSelData* sd=0);
			~SeisZAxisStretcher();

    bool		isOK() const;

    void		setOutputZRange( const z_steprg_type& zrg )
							{ outzrg_ = zrg; }

    Provider*		seisProvider()			{ return provider_; }
    const Provider*	seisProvider() const		{ return provider_; }
    const RangeSelData*	selData() const;

    uiString		message() const			{ return msg_; }
    uiString		nrDoneText() const;
    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const			{ return totalnr_; }

    void		setVelTypeIsVint( bool yn )	{ isvint_ = yn; }
    void		setVelTypeIsVrms( bool yn )	{ isvrms_ = yn; }

protected:

    mUseType( Seis,	Storer );

    bool		doPrepare(int);
    bool		doFinish(bool);
    int			nextStep();

    bool		getInputTrace(SeisTrc&);
    bool		ensureTransformSet(const TrcKey&);

    ZAxisTransform&	ztransform_;
    z_steprg_type	outzrg_;
    int			outsz_;
    float*		resamplebuf_		= nullptr;
    SeisTrc*		outtrc_			= nullptr;

    Provider*		provider_		= nullptr;
    Storer*		storer_			= nullptr;
    uiString		msg_;

    LineHorSubSel*	curlhss_		= nullptr;
    CubeHorSubSel*	curchss_		= nullptr;
    int			voiid_			= -1;
    bool		ist2d_;
    bool		is2d_;
    bool		stretchz_;
    bool		isvint_			= false;
    bool		isvrms_			= false;

    od_int64		nrdone_			= 0;
    od_int64		totalnr_		= -1;

};
