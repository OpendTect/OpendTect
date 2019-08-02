#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Raman Singh
 Date:		May 2008
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "binnedvalueset.h"
#include "randomlinegeom.h"
#include "uistring.h"

class od_ostream;
class SeisTrcBuf;

namespace Seis { class Provider; class TableSelData; class Storer; }
namespace Geometry { class RandomLine; }

mExpClass(Seis) SeisRandLineTo2D : public Executor
{ mODTextTranslationClass(SeisRandLineTo2D)
public:
			SeisRandLineTo2D(const IOObj&,const IOObj&,
					 const Pos::GeomID, int trcinit,
					 const Geometry::RandomLine&);
			~SeisRandLineTo2D();

    uiString		message() const;
    uiString		nrDoneText() const;
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    int			nextStep();

protected:

    Seis::Provider*	prov_;
    Seis::Storer*	storer_;
    Pos::GeomID		geomid_;
    uiString		errmsg_;
    int			nrdone_;
    int			totnr_;

    BinnedValueSet::SPos pos_;
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
    BufferString	parprefix_;
    BufferString	perprefix_;
    double		gridspacing_;

    Geometry::RandomLine& rln_;

    bool		mk2DLines(const Geometry::RandomLineSet&,bool);
};
