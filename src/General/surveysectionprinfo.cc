/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		August 2016
___________________________________________________________________

-*/

#include "surveysectionprinfo.h"
#include "survgeom.h"

const char* SurveySectionPresentationInfo::sKeySectionID()
{ return "Survey Section ID"; }
const char* SurveySectionPresentationInfo::sKeySectionType()
{ return "Survey Section Type"; }
const char* SurveySectionPresentationInfo::sKeyNrLayers()
{ return "Nr of Layers"; }
const char* SurveySectionPresentationInfo::sKeyLayer()
{ return "Section Layer"; }

mDefineEnumUtils( SurveySectionPresentationInfo, SectionType,
		  "Survey Section Type" )
{ "In-line", "Cross-line", "2D-line", "Z-slice", "Random-line", 0 };

static SectionLayerPresentationInfoFactory* slprinfofac_ = 0;

void SectionLayerPresentationInfo::fillPar( IOPar& par ) const
{
    par.set( IOPar::compKey(SurveySectionPresentationInfo::sKeyLayer(),
			    sKey::Type()), sectionLayerType() );
}


bool SectionLayerPresentationInfo::usePar( const IOPar& par )
{
    return par.get( IOPar::compKey(SurveySectionPresentationInfo::sKeyLayer(),
				   sKey::Type()),
		    sectionlayertypekey_ );
}


SectionLayerPresentationInfo* SectionLayerPresentationInfo::clone() const
{
    IOPar seclayerprinfo;
    fillPar( seclayerprinfo );
    return SLPRIFac().create( seclayerprinfo );
}


SectionLayerPresentationInfoFactory& SLPRIFac()
{
    if ( !slprinfofac_ )
	slprinfofac_ = new SectionLayerPresentationInfoFactory;
    return *slprinfofac_;
}


void SectionLayerPresentationInfoFactory::addCreateFunc( CreateFunc crfn,
							 const char* key )
{
    const int keyidx = keys_.indexOf( key );
    if ( keyidx >= 0 )
    {
	createfuncs_[keyidx] = crfn;
	return;
    }

    createfuncs_ += crfn;
    keys_.add( key );
}


SectionLayerPresentationInfo* SectionLayerPresentationInfoFactory::create(
	const IOPar& par )
{
    BufferString keystr;
    if ( !par.get(IOPar::compKey(SurveySectionPresentationInfo::sKeyLayer(),
				 sKey::Type()),keystr) )
	return 0;

    const int keyidx = keys_.indexOf( keystr );
    if ( keyidx < 0 )
	return 0;

    return (*createfuncs_[keyidx])( par );
}


SurveySectionPresentationInfo::SurveySectionPresentationInfo()
    : ObjPresentationInfo()
    , sectionid_(SurveySectionID::getInvalid())
{
    objtypekey_ = sFactoryKey();
}


SurveySectionPresentationInfo::~SurveySectionPresentationInfo()
{
    deepErase( layerprinfos_ );
}

SurveySectionID
	SurveySectionPresentationInfo::getNewSectionID()
{
    mDefineStaticLocalObject( SurveySectionID, lastsurveysecitonid_,
			      = SurveySectionID::get(0) );
    lastsurveysecitonid_.setI( lastsurveysecitonid_.getI()+1 );
    return lastsurveysecitonid_;
}


uiString SurveySectionPresentationInfo::getName() const
{
    const TrcKeySampling& horpos = sectionpos_.hsamp_;
    if ( sectionpos_.is2D() )
	return ::toUiString(Survey::GM().getName(horpos.getGeomID()));
    else if ( sectiontype_ == SurveySectionPresentationInfo::InLine )
	return ::toUiString( horpos.inlRange().start );
    else if ( sectiontype_ == SurveySectionPresentationInfo::CrossLine )
	return ::toUiString( horpos.crlRange().start );
    else if ( sectiontype_ == SurveySectionPresentationInfo::ZSlice )
	return ::toUiString( sectionpos_.zsamp_.start );

    return uiString::emptyString(); //TODO handle randomline
}


OD::ObjPresentationInfo* SurveySectionPresentationInfo::createFrom(
	const IOPar& par )
{
    SurveySectionPresentationInfo* psdispinfo =
	new SurveySectionPresentationInfo;
    psdispinfo->usePar( par );
    return psdispinfo;
}


void SurveySectionPresentationInfo::initClass()
{
    OD::PRIFac().addCreateFunc( createFrom, sFactoryKey() );
}


void SurveySectionPresentationInfo::fillPar( IOPar& par ) const
{
    OD::ObjPresentationInfo::fillPar( par );
    par.set( sKeySectionID(), sectionid_.getI() );
    par.set( sKeySectionType(), toString(sectiontype_) );
    sectionpos_.fillPar( par );
    par.set( sKeyNrLayers(), layerprinfos_.size() );
    for ( int idx =0; idx<layerprinfos_.size(); idx++ )
    {
	IOPar seclayerpar;
	layerprinfos_[idx]->fillPar( seclayerpar );
	par.mergeComp( seclayerpar, IOPar::compKey(sKeyLayer(),idx) );
    }
}


bool SurveySectionPresentationInfo::usePar( const IOPar& par )
{
    if ( !OD::ObjPresentationInfo::usePar(par) )
	return false;

    int sectionid;
    par.get( sKeySectionID(), sectionid );
    sectionid_.setI( sectionid );
    SectionTypeDef().parse( par, sKeySectionType(), sectiontype_ );
    sectionpos_.usePar( par );
    int nrlayers = 0;
    par.get( sKeyNrLayers(), nrlayers );
    for ( int idx =0; idx<nrlayers; idx++ )
    {
	PtrMan<IOPar> seclayerpar =
	    par.subselect( IOPar::compKey(sKeyLayer(),idx) );
	if ( !seclayerpar )
	    continue;

	layerprinfos_ += SLPRIFac().create( *seclayerpar );
    }

    return true;
}


bool SurveySectionPresentationInfo::isSameObj(
	const OD::ObjPresentationInfo& prinfo ) const
{
    mDynamicCastGet(const SurveySectionPresentationInfo*,secprinfo,&prinfo);
    if ( !secprinfo )
	return false;

    return sectiontype_==secprinfo->sectionType() &&
	   sectionid_==secprinfo->sectionID();
}
