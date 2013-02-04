/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "moddepmgr.h"
#include "rangeposprovider.h"
#include "price.h"
#include "mathproperty.h"
#include "elasticpropseltransl.h"
#include "preloads.h"
#include "helpview.h"
#include "geometryio.h"
#include "survgeometrytransl.h"

mDefModInitFn(General)
{
    mIfNotFirstTime( return );
    
    HelpViewer::init();
    
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

    Survey::GeometryWriter2D::initClass();
    Survey::GeometryWriter3D::initClass();
    Survey::GeometryReader3D::initClass();
    Survey::GeometryReader2D::initClass();
    Survey::SurvGeomTranslatorGroup::initClass();
    Survey::dgb2DSurvGeomTranslator::initClass();

    Currency::repository_ += new Currency( "EUR", 100 );
    Currency::repository_ += new Currency( "USD", 100 );
}
