#ifndef seisrandlineto2d_h
#define seisrandlineto2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: seisrandlineto2d.h,v 1.5 2009-03-30 06:56:34 cvsraman Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "binidvalset.h"
#include "randomlinegeom.h"

class IOObj;
class IOPar;
class LineKey;
class SeisTrcReader;
class SeisTrcWriter;

namespace Seis { class TableSelData; }
namespace Geometry { class RandomLine; }

mClass SeisRandLineTo2D : public Executor
{
public:
    			SeisRandLineTo2D(IOObj*,IOObj*,const LineKey&,
					 const int& trcinit,
					 const Geometry::RandomLine&);
			~SeisRandLineTo2D();

    const char*		message() const;
    const char*		nrDoneText() const;
    od_int64		totalNr() const;
    od_int64		nrDone() const;
    int			nextStep();

    bool		execute(std::ostream* log=0,bool isfirst=true,
	    			bool islast=true,int delaybetwnstepsinms=0);
protected:

    SeisTrcReader*	rdr_;
    SeisTrcWriter*	wrr_;
    int			nrdone_;
    int			totnr_;

    BinIDValueSet::Pos	pos_;
    Seis::TableSelData&	seldata_;
};


mClass SeisRandLineTo2DGrid
{
public:
    			SeisRandLineTo2DGrid(const IOPar&,std::ostream&);

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
    std::ostream&	strm_;

    IOObj*		inpobj_;
    IOObj*		outpobj_;
    BufferString	outpattrib_;
    BufferString	parprefix_;
    BufferString	perprefix_;
    double		gridspacing_;

    Geometry::RandomLine	rln_;

    bool		mk2DLines(const Geometry::RandomLineSet&,bool);
};


#endif
