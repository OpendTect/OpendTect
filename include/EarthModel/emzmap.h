#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override;

    void		setCoordSystem(const Coords::CoordSystem*);
    void		setUOM(const UnitOfMeasure*);

    const Array2D<float>* data() const			{ return data_; }
    Array2D<float>*	data()				{ return data_; }

    Coord		minCoord() const;
    Coord		maxCoord() const;
    Coord		step() const;
    TrcKeySampling	sampling() const;

    int			nrRows() const			{ return nrrows_; }
    int			nrCols() const			{ return nrcols_; }

    const PosInfo::Detector& detector() const		{ return posdetector_; }

private:
    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool,od_ostream* =nullptr) override;

    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totalnr_; }

    bool		initHeader();
    void		applyCRS();

    od_int64		nrdone_		= 0;
    od_int64		totalnr_;
    uiString		msg_;

    BufferString	fnm_;
    od_istream*		istrm_;
    Array2D<float>*	data_		= nullptr;
    ConstRefMan<Coords::CoordSystem> coordsystem_;
    const UnitOfMeasure* uom_		= nullptr;
    PosInfo::Detector&	posdetector_;

    // header line 1
    int			nrnodesperline_ = mUdf(int);

    // header line 2
    int			nrchars_	= mUdf(int);
    float		undefval_	= -999.f;
    BufferString	undeftxt_;
    int			nrdec_		= mUdf(int);
    int			firstcol_	= mUdf(int);

    // header line 3
    int			nrrows_		= mUdf(int);
    int			nrcols_		= mUdf(int);
    double		xmin_		= mUdf(double);
    double		xmax_		= mUdf(double);
    double		ymin_		= mUdf(double);
    double		ymax_		= mUdf(double);

    // derived
    double		dx_		= mUdf(double);
    double		dy_		= mUdf(double);

    // Coords in survey's CRS
    Coord		mincrd_		= Coord::udf();
    Coord		maxcrd_		= Coord::udf();
};

} // namespace EM
