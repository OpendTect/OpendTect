#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2014
________________________________________________________________________

-*/



#include "earthmodelmod.h"

#include "coordsystem.h"
#include "executor.h"
#include "trckeysampling.h"

class UnitOfMeasure;
template<class T> class Array2D;
namespace PosInfo { class Detector; }

namespace EM
{

mExpClass(EarthModel) ZMapImporter : public Executor
{ mODTextTranslationClass(ZMapImporter)
public:
			ZMapImporter(const char* fnm);
			~ZMapImporter();

    od_int64		nrDone() const			{ return nrdone_; }
    od_int64		totalNr() const			{ return totalnr_; }
    uiString		uiMessage() const		{ return msg_; }
    uiString		uiNrDoneText() const		{ return nrdonetxt_; }

    void		setCoordSystem(Coords::CoordSystem*);
    void		setUOM(const UnitOfMeasure*);

    const Array2D<float>* data() const			{ return data_; }
    Array2D<float>*	data()				{ return data_; }

    Coord		minCoord() const;
    Coord		maxCoord() const;
    Coord		step() const;
    TrcKeySampling	sampling() const;

    int			nrRows() const			{ return nrrows_; }
    int			nrCols() const			{ return nrcols_; }

    const PosInfo::Detector& detector() const;

protected:

    bool		initHeader();
    void		applyCRS();
    int			nextStep();

    od_int64		nrdone_		= 0;
    od_int64		totalnr_	= 0;
    uiString		msg_;
    uiString		nrdonetxt_;

    Array2D<float>*	data_		= nullptr;
    BufferString	fnm_;
    bool		initdone_	= false;
    od_istream*		istrm_		= nullptr;
    RefMan<Coords::CoordSystem>	coordsystem_;

    // header line 1
    int			nrnodesperline_	= 0;

    // header line 2
    int			nrchars_	= 0;
    float		undefval_	= -999.f;
    BufferString	undeftxt_;
    int			nrdec_		= 0;
    int			firstcol_	= 0;

    // header line 3
    int			nrrows_		= 0;
    int			nrcols_		= 0;
    double		xmin_		= 0.;
    double		xmax_		= 0.;
    double		ymin_		= 0.;
    double		ymax_		= 0.;

    // derived
    double		dx_		= 0.;
    double		dy_		= 0.;

    // Coords in survey's CRS
    Coord		mincrd_;
    Coord		maxcrd_;
};

} // namespace EM

