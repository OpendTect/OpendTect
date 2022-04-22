#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/

#include "seismod.h"
#include "executor.h"
#include "binidvalset.h"
#include "randomlinegeom.h"
#include "uistring.h"

class IOObj;
class od_ostream;
class LineKey;
class SeisTrcReader;
class SeisTrcWriter;
class SeisTrcBuf;

namespace Seis { class TableSelData; }
namespace Geometry { class RandomLine; }

mExpClass(Seis) SeisRandLineTo2D : public Executor
{ mODTextTranslationClass(SeisRandLineTo2D)
public:
			SeisRandLineTo2D(const IOObj&,const IOObj&,
					 const Pos::GeomID, int trcinit,
					 const Geometry::RandomLine&);
			~SeisRandLineTo2D();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    int			nextStep() override;

protected:

    SeisTrcReader*	rdr_ = nullptr;
    SeisTrcWriter*	wrr_ = nullptr;
    Pos::GeomID		geomid_;
    int			nrdone_ = 0;
    int			totnr_;

    BinIDValueSet::SPos	pos_;
    Seis::TableSelData&	seldata_;

private:

    SeisTrcBuf*		buf_;

    bool		writeTraces();
};


mExpClass(Seis) SeisRandLineTo2DGrid
{ mODTextTranslationClass(SeisRandLineTo2DGrid)
public:
			SeisRandLineTo2DGrid(const IOPar&,od_ostream&);
			~SeisRandLineTo2DGrid();

    bool		isOK()			{ return isok_; }
    bool		createGrid();

    static const char*	sKeyInputID()		{ return "Input ID"; }
    static const char*	sKeyOutputID()		{ return "Output ID"; }
    static const char*	sKeyOutpAttrib()	{ return "Output attribute"; }
    static const char*	sKeyGridSpacing()	{ return "Grid Spacing"; }
    static const char*	sKeyParPrefix()		{ return "Parallel prefix"; }
    static const char*	sKeyPerpPrefix()	{ return "Perp prefix"; }
    static const char*	sKeyRandomLine()	{ return "Random line"; }
    static const char*	sKeyStartBinID()	{ return "Start BinID"; }
    static const char*	sKeyStopBinID()		{ return "Stop BinID"; }

protected:

    bool		isok_;
    od_ostream&		strm_;

    IOObj*		inpobj_;
    IOObj*		outpobj_;
    BufferString	outpattrib_;
    BufferString	parprefix_;
    BufferString	perprefix_;
    double		gridspacing_;

    Geometry::RandomLine& rln_;

    bool		mk2DLines(const Geometry::RandomLineSet&,bool);
};


