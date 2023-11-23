/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "batchjobdispatch.h"
#include "elasticpropseltransl.h"
#include "genc.h"
#include "geojsonwriter.h"
#include "geometryio.h"
#include "googlexmlwriter.h"
#include "imagedeftr.h"
#include "ioobjselectiontransl.h"
#include "mathformulatransl.h"
#include "mathproperty.h"
#include "oddirs.h"
#include "plugins.h"
#include "preloads.h"
#include "rangeposprovider.h"
#include "simpletimedepthmodel.h"
#include "survgeometrytransl.h"


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

    GeoJSONWriter::initClass();
    ODGoogle::KMLWriter::initClass();

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

    ImageDefTranslatorGroup::initClass();
    ODImageDefTranslator::initClass();

    BufferString libnm; libnm.setMinBufSize( 32 );
    SharedLibAccess::getLibName( "CRS", libnm.getCStr(), libnm.bufSize() );
    const FilePath libfp( GetLibPlfDir(), libnm );
    if ( libfp.exists() )
	PIM().load( libfp.fullPath(), PluginManager::Data::AppDir,
		    PI_AUTO_INIT_EARLY );
}
