#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

 
#include "seismod.h"
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

    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Traces written"); }
    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totalnr_; }
    Pos::GeomID		curGeomID() const;

    int			nextStep() override;

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
