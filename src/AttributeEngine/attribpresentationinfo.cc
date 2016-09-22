/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "attribpresentationinfo.h"
#include "attribsel.h"
#include "keystrs.h"

AttribPresentationInfo::AttribPresentationInfo()
    : SectionLayerPresentationInfo()
    , attrspec_(new Attrib::SelSpec())
    , attribdpid_(DataPack::ID::getInvalid())
{
    sectionlayertypekey_ = sFactoryKey();
}


AttribPresentationInfo::~AttribPresentationInfo()
{
    delete attrspec_;
}


SectionLayerPresentationInfo* AttribPresentationInfo::createFrom(
	const IOPar& par )
{
    AttribPresentationInfo* attrprinfo = new AttribPresentationInfo;
    attrprinfo->usePar( par );
    return attrprinfo;
}


const char* AttribPresentationInfo::sFactoryKey()
{
    return sKey::Attribute();
}


const char* AttribPresentationInfo::sAttribDataPackID()
{
    return IOPar::compKey(sKey::Attribute(),"DataPackID");
}


void AttribPresentationInfo::initClass()
{
    SLPRIFac().addCreateFunc( createFrom, sFactoryKey() );
}


void AttribPresentationInfo::fillPar( IOPar& par ) const
{
    SectionLayerPresentationInfo::fillPar( par );
    attrspec_->fillPar( par );
    par.set( sAttribDataPackID(), attribdpid_ );
    attrcoltab_.fillPar( par );
    attrmapper_.fillPar( par );
}


bool AttribPresentationInfo::usePar( const IOPar& par )
{
    if ( !SectionLayerPresentationInfo::usePar(par) )
	return false;

    attrspec_->usePar( par );
    par.get( sAttribDataPackID(), attribdpid_ );
    attrcoltab_.usePar( par );
    attrmapper_.usePar( par );
    return true;
}

void AttribPresentationInfo::setSelSpec( const Attrib::SelSpec& sp )
{
    *attrspec_ = sp;
}
