/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "geom2dinit.h"
#include "moddepmgr.h"
#include "picksettr.h"
#include "polyposprovider.h"
#include "posvecdatasettr.h"
#include "probdenfunctr.h"
#include "randomlinetr.h"
#include "tableposprovider.h"

using voidFromVoidFn = void(*)(void);
mGlobal(General) void setGlobal_General_Fns(voidFromVoidFn);

mDefModInitFn(Geometry)
{
    mIfNotFirstTime( return );

    PickSetTranslatorGroup::initClass();
    PosVecDataSetTranslatorGroup::initClass();
    ProbDenFuncTranslatorGroup::initClass();
    RandomLineSetTranslatorGroup::initClass();

    dgbPickSetTranslator::initClass();
    odPosVecDataSetTranslator::initClass();
    odProbDenFuncTranslator::initClass();
    dgbRandomLineSetTranslator::initClass();

    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();

    Geometry::PrimitiveSetCreator::setCreator(
				new Geometry::PrimitiveSetCreatorDefImpl );

    setGlobal_General_Fns( Geom2DInit::Start );
}
