/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "indexedshape.h"
#include "dbman.h"
#include "polyposprovider.h"
#include "tableposprovider.h"
#include "picksettr.h"
#include "posvecdatasettr.h"
#include "probdenfunctr.h"
#include "randomlineprobe.h"
#include "randomlinetr.h"
namespace Pick { void startSetCategoryFromTypeInOMFPutter(); }

mDefModInitFn(Geometry)
{
    mIfNotFirstTime( return );

    DBM(); //Trigger createion & reading of geometries

    PickSetTranslatorGroup::initClass();
    PosVecDataSetTranslatorGroup::initClass();
    ProbDenFuncTranslatorGroup::initClass();
    RandomLineSetTranslatorGroup::initClass();

    dgbPickSetTranslator::initClass();
    odPosVecDataSetTranslator::initClass();
    odProbDenFuncTranslator::initClass();
    dgbRandomLineSetTranslator::initClass();

    RandomLineProbe::initClass();
    Pos::PolyProvider3D::initClass();
    Pos::TableProvider3D::initClass();

    Geometry::PrimitiveSetCreator::setCreator(
				new Geometry::PrimitiveSetCreatorDefImpl );

    Pick::startSetCategoryFromTypeInOMFPutter();
}
