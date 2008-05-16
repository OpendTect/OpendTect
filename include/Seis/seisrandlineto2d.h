#ifndef seisrandlineto2d_h
#define seisrandlineto2d_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		May 2008
 RCS:		$Id: seisrandlineto2d.h,v 1.1 2008-05-16 11:33:19 cvsraman Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "binidvalset.h"

class IOObj;
class LineKey;
class SeisTrcReader;
class SeisTrcWriter;

namespace Seis { class TableSelData; }
namespace Geometry { class RandomLine; }

class SeisRandLineTo2D : public Executor
{
public:
    			SeisRandLineTo2D(IOObj*,IOObj*,const LineKey&,
					 Interval<int>,
					 const Geometry::RandomLine&);
			~SeisRandLineTo2D();

    const char*		message() const;
    const char*		nrDoneText() const;
    int			totalNr() const;
    int			nrDone() const;
    int			nextStep();

protected:

    SeisTrcReader*	rdr_;
    SeisTrcWriter*	wrr_;
    int			nrdone_;
    int			totnr_;

    BinIDValueSet::Pos	pos_;
    Seis::TableSelData&	seldata_;
};

#endif
