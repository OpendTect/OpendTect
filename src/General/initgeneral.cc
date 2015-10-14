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
#include "mathproperty.h"
#include "ioman.h"
#include "elasticpropseltransl.h"
#include "mathformulatransl.h"
#include "ioobjselectiontransl.h"
#include "preloads.h"
#include "geometryio.h"
#include "survgeometrytransl.h"
#include "batchjobdispatch.h"


mDefSimpleTranslators(IOObjSelection,"Object selection",od,Misc);


mDefModInitFn(General)
{
    mIfNotFirstTime( return );

    ElasticPropSelectionTranslatorGroup::initClass();
    MathFormulaTranslatorGroup::initClass();
    IOObjSelectionTranslatorGroup::initClass();
    PreLoadsTranslatorGroup::initClass();
    PreLoadSurfacesTranslatorGroup::initClass();

    dgbPreLoadsTranslator::initClass();
    dgbPreLoadSurfacesTranslator::initClass();
    odElasticPropSelectionTranslator::initClass();
    odMathFormulaTranslator::initClass();
    odIOObjSelectionTranslator::initClass();

    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();

    Survey::GeometryWriter2D::initClass();
    Survey::GeometryWriter3D::initClass();
    Survey::GeometryReader3D::initClass();
    Survey::GeometryReader2D::initClass();
    SurvGeom2DTranslatorGroup::initClass();
    dgbSurvGeom2DTranslator::initClass();

    IOM(); //Trigger createion & reading of geometries

    Batch::SingleJobDispatcher::initClass();
}

