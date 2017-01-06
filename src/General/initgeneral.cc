/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "moddepmgr.h"

#include "coordsystem.h"
#include "dbman.h"
#include "elasticpropseltransl.h"
#include "geometryio.h"
#include "ioobjselectiontransl.h"
#include "mathformulatransl.h"
#include "mathproperty.h"
#include "pickset.h"
#include "preloads.h"
#include "probeimpl.h"
#include "probetr.h"
#include "rangeposprovider.h"
#include "survgeometrytransl.h"


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

    Survey::GeometryWriter2D::initClass();
    Survey::GeometryWriter3D::initClass();
    Survey::GeometryReader3D::initClass();
    Survey::GeometryReader2D::initClass();
    SurvGeom2DTranslatorGroup::initClass();
    dgbSurvGeom2DTranslator::initClass();

    DBM().initFirst(); //Trigger creation & reading of geometries

    //After DBM is created
    Coords::PositionSystem::initRepository( &DBM().afterSurveyChange );
}
