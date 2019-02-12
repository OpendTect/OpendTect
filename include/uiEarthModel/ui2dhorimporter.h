#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/
#include "earthmodelmod.h"
#include "executor.h"
#include "bufstringset.h"
#include "posinfo2d.h"
#include "binidvalset.h"

namespace EM { class Horizon2D;}
namespace Survey { class Geometry2D;}

class Horizon2DBulkImporter : public Executor
{ mODTextTranslationClass(Horizon2DBulkImporter);
public:

    enum UndefTreat		{ Skip, Adopt, Interpolate };

    Horizon2DBulkImporter(const BufferStringSet& lnms,
			ObjectSet<EM::Horizon2D>& hors,
			const BinIDValueSet* valset, UndefTreat udftreat);

protected:

    ObjectSet<EM::Horizon2D>&	hors_;
    const BinIDValueSet*	bvalset_;
    GeomIDSet			geomids_;
    const Survey::Geometry2D*	curlinegeom_;
    int				nrdone_;
    TypeSet<int>		prevtrcnrs_;
    TypeSet<float>		prevtrcvals_;
    int				prevlineidx_;
    BinIDValueSet::SPos		pos_;
    UndefTreat			udftreat_;
    void			interpolateAndSetVals(int hidx,Pos::GeomID,
				int curtrcnr,int prevtrcnr,float curval,
				float prevval );
    int				nextStep();
    od_int64			nrDone() const;
    uiString			nrDoneText() const;
    od_int64			totalNr() const;
    uiString			message() const;


};
