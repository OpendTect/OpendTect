/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "commandlineparser.h"
#include "mathproperty.h"
#include "ioman.h"
#include "genc.h"
#include "elasticpropseltransl.h"
#include "mathformulatransl.h"
#include "ioobjselectiontransl.h"
#include "preloads.h"
#include "geometryio.h"
#include "rangeposprovider.h"
#include "simpletimedepthmodel.h"
#include "survgeometrytransl.h"
#include "batchjobdispatch.h"
#include "googlexmlwriter.h"
#include "geojsonwriter.h"


mDefSimpleTranslators(IOObjSelection,"Object selection",od,Misc)
mDefSimpleTranslators(PosProviders,"Subselection",dgb,Misc)
mDefSimpleTranslators(PreLoads,"Object Pre-Loads",dgb,Misc)
mDefSimpleTranslators(PreLoadSurfaces,"Object HorPre-Loads",dgb,Misc)


mDefModInitFn(General)
{
    mIfNotFirstTime( return );

    ElasticPropSelectionTranslatorGroup::initClass();
    MathFormulaTranslatorGroup::initClass();
    IOObjSelectionTranslatorGroup::initClass();
    PosProvidersTranslatorGroup::initClass();
    PreLoadsTranslatorGroup::initClass();
    PreLoadSurfacesTranslatorGroup::initClass();

    odElasticPropSelectionTranslator::initClass();
    odMathFormulaTranslator::initClass();
    odIOObjSelectionTranslator::initClass();
    dgbPosProvidersTranslator::initClass();
    dgbPreLoadsTranslator::initClass();
    dgbPreLoadSurfacesTranslator::initClass();

    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();

    if ( !NeedDataBase() )
	return;

    Survey::GeometryWriter2D::initClass();
    Survey::GeometryWriter3D::initClass();
    Survey::GeometryReader3D::initClass();
    Survey::GeometryReader2D::initClass();
    SurvGeom2DTranslatorGroup::initClass();
    dgbSurvGeom2DTranslator::initClass();

    SimpleTimeDepthModelTranslatorGroup::initClass();
    odSimpleTimeDepthModelTranslator::initClass();
    SimpleT2DTransform::initClass();
    SimpleD2TTransform::initClass();

    GeoJSONWriter::initClass();
    ODGoogle::KMLWriter::initClass();

    IOM().setDataSource( CommandLineParser() );
}
