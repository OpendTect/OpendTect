#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "executor.h"
#include "uistring.h"

class IOObj;
class Seis2DDataSet;
class SeisTrcReader;
class SeisTrcWriter;
namespace Survey { class Geometry2D; }

/*!\brief Extracts 2D data from 3D Cube */

mExpClass(Seis) Seis2DFrom3DExtractor : public Executor
{ mODTextTranslationClass(Seis2DFrom3DExtractor);
public:
			Seis2DFrom3DExtractor(const IOObj& cubein,
					      const IOObj& lsout,
					      const TypeSet<Pos::GeomID>&);
			~Seis2DFrom3DExtractor();

    uiString		message() const	{ return msg_; }
    uiString		nrDoneText() const	{ return tr("Traces written"); }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const	{ return totalnr_; }
    Pos::GeomID		curGeomID() const;

    int		nextStep();

protected:

    SeisTrcReader&	rdr_;
    SeisTrcWriter&	wrr_;
    uiString		msg_;

    od_int64		nrdone_;
    od_int64		totalnr_;

    int			curlineidx_;
    int			curtrcidx_;

    const TypeSet<Pos::GeomID>& geomids_;
    const Survey::Geometry2D*	curgeom2d_;

    int			goToNextLine();
    int			handleTrace();

};
