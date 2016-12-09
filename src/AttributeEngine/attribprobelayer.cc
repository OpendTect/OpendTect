/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "attribprobelayer.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "dbman.h"
#include "keystrs.h"
#include "seisioobjinfo.h"

mImplMonitorableAssignment( AttribProbeLayer, ProbeLayer )

void AttribProbeLayer::copyClassData( const AttribProbeLayer& oth )
{
    disptype_ = oth.disptype_;
    attrspec_ = oth.attrspec_;
    attribdpid_ = oth.attribdpid_;
    attrcoltab_ = oth.attrcoltab_;
    attrmapper_ = oth.attrmapper_;
}


AttribProbeLayer::AttribProbeLayer( DispType dt )
    : ProbeLayer()
    , attrcoltab_( ColTab::Sequence(ColTab::defSeqName()) )
    , attrspec_(*new Attrib::SelSpec())
    , attribdpid_(DataPack::ID::getInvalid())
    , disptype_(dt)
    , attrdp_(0)
{
}


AttribProbeLayer::~AttribProbeLayer()
{
    delete &attrspec_;
}


ProbeLayer* AttribProbeLayer::createFrom(
	const IOPar& par )
{
    AttribProbeLayer* attrprinfo = new AttribProbeLayer;
    attrprinfo->usePar( par );
    return attrprinfo;
}


const char* AttribProbeLayer::sFactoryKey()
{ return sKey::Attribute(); }


const char* AttribProbeLayer::layerType() const
{
    return sFactoryKey();
}


const char* AttribProbeLayer::sAttribDataPackID()
{ return IOPar::compKey(sKey::Attribute(),"DataPackID"); }


void AttribProbeLayer::initClass()
{
    PrLayFac().addCreateFunc( createFrom, sFactoryKey() );
}


void AttribProbeLayer::fillPar( IOPar& par ) const
{
    mLock4Read();

    ProbeLayer::fillPar( par );
    attrspec_.fillPar( par );
    par.set( sAttribDataPackID(), attribdpid_ );
    attrcoltab_.fillPar( par );
    attrmapper_.fillPar( par );
}


bool AttribProbeLayer::usePar( const IOPar& par )
{
    mLock4Write();

    attrspec_.usePar( par );
    par.get( sAttribDataPackID(), attribdpid_ );
    attrcoltab_.usePar( par );
    attrmapper_.usePar( par );
    return true;
}


void AttribProbeLayer::updateDataPack()
{
    const Probe* parentprobe = getProbe();
    if ( !parentprobe )
    { pErrMsg( "Parent probe not set" ); return; }

    DataPackMgr& dpm = DPM( getDataPackManagerID() );
    attrdp_ = dpm.get( attribdpid_ );
}


void AttribProbeLayer::setAttribDataPack( DataPack::ID dpid )
{
    mLock4Read();

    if ( attribdpid_ == dpid )
	return;

    if ( !mLock2Write() && attribdpid_ == dpid )
	return;

    attribdpid_ = dpid;
    updateDataPack();
    mSendChgNotif( cDataChange(), cEntireObjectChangeID() );

}


void AttribProbeLayer::setSelSpec( const Attrib::SelSpec& as )
{
    mLock4Read();

    if ( attrspec_ == as )
	return;

    if ( !mLock2Write() && attrspec_ == as )
	return;

    attrspec_ = as;
    attribdpid_ = DataPack::cNoID();
    name_ = attrspec_.userRef();
    mSendChgNotif( cDataChange(), cEntireObjectChangeID() );
}


void AttribProbeLayer::invalidateData()
{
    mLock4Write();

    attribdpid_ = DataPack::cNoID();
}


bool AttribProbeLayer::useStoredColTabPars()
{
    mLock4Read();

    if ( attrspec_.isNLA() ) return false;

    const Attrib::DescSet* attrset =
	Attrib::DSHolder().getDescSet( attrspec_.is2D(), true );
    const Attrib::Desc* desc =
	attrset ? attrset->getDesc( attrspec_.id() ) : 0;
    if ( !desc )
    {
	attrset = Attrib::DSHolder().getDescSet( attrspec_.is2D(), false );
	desc = attrset ? attrset->getDesc( attrspec_.id() ) : 0;
	if ( !desc )
	    return false;
    }

    DBKey storedid = desc->getStoredID();
    if ( !desc->isStored() || storedid.isInvalid() )
	return false;

    if ( !DBM().get(storedid) )
	return false;

    SeisIOObjInfo seisobj( storedid );
    IOPar iop;
    if ( !seisobj.getDisplayPars(iop) )
	return false;

    mLock2Write();

    if ( !attrmapper_.usePar(iop) )
	return false;

    BufferString coltabnm = iop.find( sKey::Name() );
    if ( !coltabnm.isEmpty() )
	attrcoltab_ = ColTab::Sequence( coltabnm );

    mSendChgNotif( cColTabSeqChange(), cEntireObjectChangeID() );
    return true;
}
