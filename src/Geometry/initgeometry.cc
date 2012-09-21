/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "moddepmgr.h"
#include "polyposprovider.h"
#include "tableposprovider.h"
#include "picksettr.h"
#include "posvecdatasettr.h"
#include "probdenfunctr.h"
#include "randomlinetr.h"

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
}
