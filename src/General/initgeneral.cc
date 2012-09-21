/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "moddepmgr.h"
#include "rangeposprovider.h"
#include "price.h"
#include "mathproperty.h"
#include "elasticpropseltransl.h"
#include "preloads.h"

mDefModInitFn(General)
{
    mIfNotFirstTime( return );
    
    ElasticPropSelectionTranslatorGroup::initClass();
    PreLoadsTranslatorGroup::initClass();
    PreLoadSurfacesTranslatorGroup::initClass();
    
    dgbPreLoadsTranslator::initClass();
    dgbPreLoadSurfacesTranslator::initClass();
    odElasticPropSelectionTranslator::initClass();

    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();

    Currency::repository_ += new Currency( "EUR", 100 );
    Currency::repository_ += new Currency( "USD", 100 );
}
