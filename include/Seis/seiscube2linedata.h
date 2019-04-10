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
#include "geomid.h"
#include "uistring.h"

class Seis2DDataSet;
namespace Seis { class Provider; class Storer; }
namespace Survey { class Geometry2D; }


/*!\brief Extracts 2D data from 3D Cube */

mExpClass(Seis) Seis2DFrom3DExtractor : public Executor
{ mODTextTranslationClass(Seis2DFrom3DExtractor);
public:

    mUseType( Seis,	Provider );
    mUseType( Seis,	Storer );
    mUseType( Pos,	GeomID );
    mUseType( Survey,	Geometry2D );

			Seis2DFrom3DExtractor(const IOObj& cubein,
					      const IOObj& lsout,
					      const GeomIDSet&);
			~Seis2DFrom3DExtractor();

    uiString		message() const		{ return uiString(uirv_); }
    uiString		nrDoneText() const	{ return tr("Traces written"); }
    od_int64		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totalnr_; }
    GeomID		curGeomID() const;

    int			nextStep();

protected:

    Provider*		prov_;
    Storer&		storer_;
    uiRetVal		uirv_;

    od_int64		nrdone_;
    od_int64		totalnr_;
    int			curlineidx_;
    int			curtrcidx_;

    const GeomIDSet&	geomids_;
    const Geometry2D*	curgeom2d_;

    int			goToNextLine();
    int			handleTrace();

};
