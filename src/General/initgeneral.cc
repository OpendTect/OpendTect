/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "commandlineparser.h"
#include "dbman.h"
#include "elasticpropseltransl.h"
#include "genc.h"
#include "googlexmlwriter.h"
#include "geojsonwriter.h"
#include "ioobjselectiontransl.h"
#include "mathformulatransl.h"
#include "mathproperty.h"
#include "pickset.h"
#include "preloads.h"
#include "probeimpl.h"
#include "probetr.h"
#include "rangeposprovider.h"
#include "survgeometryio.h"
#include "survgeometrytransl.h"


typedef BufferString (*strFromDBKeyFn)(const DBKey&);
typedef bool (*boolFromDBKeyFn)(const DBKey&);
typedef IOObj* (*ioObjPtrFromDBKeyFn)(const DBKey&);
typedef void (*handleIOObjPtrFn)(IOObj*);
mGlobal(Basic) void setDBMan_DBKey_Fns(strFromDBKeyFn,strFromDBKeyFn,
			boolFromDBKeyFn,ioObjPtrFromDBKeyFn,handleIOObjPtrFn);
mGlobal(General) BufferString DBMan_nameOf(const DBKey&);
mGlobal(General) BufferString DBMan_mainFileOf(const DBKey&);
mGlobal(General) bool DBMan_implExist(const DBKey&);
mGlobal(General) IOObj* DBMan_getIOObj(const DBKey&);
static void deleteIOObj( IOObj* ioobj ) { delete ioobj; }
namespace Survey { void GeometryIO_init2DGeometry(); }


mDefSimpleTranslators(IOObjSelection,"Object selection",od,Misc)
mDefSimpleTranslators(PosProviders,"Subselection",dgb,Misc)
mDefSimpleTranslators(PreLoads,"Object Pre-Loads",dgb,Misc)
mDefSimpleTranslators(PreLoadSurfaces,"Object HorPre-Loads",dgb,Misc)
class GeneralModuleIniter { public: GeneralModuleIniter(); };

mDefModInitFn(General)
{
    mIfNotFirstTime( return );
    GeneralModuleIniter initer;
}


GeneralModuleIniter::GeneralModuleIniter()
{
    setDBMan_DBKey_Fns( DBMan_nameOf, DBMan_mainFileOf, DBMan_implExist,
			DBMan_getIOObj, deleteIOObj );

    ElasticPropSelectionTranslatorGroup::initClass();
    MathFormulaTranslatorGroup::initClass();
    IOObjSelectionTranslatorGroup::initClass();
    PosProvidersTranslatorGroup::initClass();
    PreLoadsTranslatorGroup::initClass();
    PreLoadSurfacesTranslatorGroup::initClass();
    ProbeTranslatorGroup::initClass();

    odElasticPropSelectionTranslator::initClass();
    odMathFormulaTranslator::initClass();
    odIOObjSelectionTranslator::initClass();
    dgbPosProvidersTranslator::initClass();
    dgbPreLoadsTranslator::initClass();
    dgbPreLoadSurfacesTranslator::initClass();
    dgbProbeTranslator::initClass();

    Pick::SetPresentationInfo::initClass();
    ProbePresentationInfo::initClass();
    InlineProbe::initClass();
    CrosslineProbe::initClass();
    ZSliceProbe::initClass();
    Line2DProbe::initClass();
    VolumeProbe::initClass();
    Pos::RangeProvider3D::initClass();
    Pos::RangeProvider2D::initClass();
    ValueProperty::initClass();
    RangeProperty::initClass();
    MathProperty::initClass();
    GeoJSONWriter::initClass();
    ODGoogle::KMLWriter::initClass();

    if ( !NeedDataBase() )
	return;

    Survey::GeometryIO_init2DGeometry();
    DBM().initFirst(); //Trigger creation & reading of geometries
    DBM().setDataSource( CommandLineParser() );
}
