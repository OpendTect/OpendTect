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


AttribProbeLayer::AttribProbeLayer( DispType dt )
    : ProbeLayer()
    , colseq_(ColTab::SeqMGR().getDefault(false))
    , mappersetup_(new ColTab::MapperSetup)
    , attrspec_(*new Attrib::SelSpec())
    , disptype_(dt)
{
}


AttribProbeLayer::AttribProbeLayer( const AttribProbeLayer& oth )
    : ProbeLayer(oth)
    , attrspec_(*new Attrib::SelSpec())
{
    copyClassData( oth );
}

mImplMonitorableAssignment( AttribProbeLayer, ProbeLayer )

void AttribProbeLayer::copyClassData( const AttribProbeLayer& oth )
{
    disptype_ = oth.disptype_;
    attrspec_ = oth.attrspec_;
    attribdpid_ = oth.attribdpid_;
    colseq_ = oth.colseq_;
    if ( mappersetup_ && oth.mappersetup_ )
	*mappersetup_ = *oth.mappersetup_;
    else if ( mappersetup_ )
	mappersetup_ = 0;
    attrdp_ = oth.attrdp_;
}


Monitorable::ChangeType AttribProbeLayer::compareClassData(
	                                const AttribProbeLayer& oth ) const
{
    if ( disptype_ != oth.disptype_ )
	return cEntireObjectChange();

    mStartMonitorableCompare();
    mHandleMonitorableCompare( attribdpid_, cDataChange() );
    mHandleMonitorableCompare( attrspec_, cDataChange() );
    mHandleMonitorableCompare( colseq_, cColSeqChange() );
    mHandleMonitorableComparePtrContents( mappersetup_, cMapperSetupChange() );
    mDeliverMonitorableCompare();
}


AttribProbeLayer::~AttribProbeLayer()
{
    sendDelNotif();
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
{
    return sKey::Attribute();
}


const char* AttribProbeLayer::layerType() const
{
    return sFactoryKey();
}


const char* AttribProbeLayer::sAttribDataPackID()
{
    return IOPar::compKey( sKey::Attribute(), "DataPackID" );
}


const char* AttribProbeLayer::sAttribColTabName()
{
    return IOPar::compKey( sKey::Attribute(), "ColTabName" );
}


void AttribProbeLayer::initClass()
{
    PrLayFac().addCreateFunc( createFrom, sFactoryKey() );
}


ConstRefMan<ColTab::MapperSetup> AttribProbeLayer::mapperSetup() const
{
    mLock4Read();
    return mappersetup_;
}


void AttribProbeLayer::setMapperSetup( const MapperSetup& msu )
{
    mLock4Read();
    if ( *mappersetup_ == msu )
	return;
    if ( !mLock2Write() && *mappersetup_ == msu )
	return;

    *mappersetup_ = msu;
    mSendChgNotif( cMapperSetupChange(), 0 );
}


void AttribProbeLayer::fillPar( IOPar& par ) const
{
    mLock4Read();

    ProbeLayer::fillPar( par );
    attrspec_.fillPar( par );
    par.set( sAttribDataPackID(), attribdpid_ );
    par.set( sAttribColTabName(), colseq_->name() );
    mappersetup_->fillPar( par );
}


bool AttribProbeLayer::usePar( const IOPar& par )
{
    mLock4Write();

    attrspec_.usePar( par );
    par.get( sAttribDataPackID(), attribdpid_ );
    BufferString seqnm( colseq_->name() );
    par.get( sAttribColTabName(), seqnm );
    colseq_ = ColTab::SeqMGR().getAny( seqnm );
    mappersetup_->usePar( par );
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


void AttribProbeLayer::setDataPackID( DataPack::ID dpid )
{
    mLock4Read();

    if ( attribdpid_ == dpid )
	return;

    if ( !mLock2Write() && attribdpid_ == dpid )
	return;

    attribdpid_ = dpid;
    updateDataPack();
    mSendChgNotif( cDataChange(), cUnspecChgID() );
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
    mSendChgNotif( cDataChange(), cUnspecChgID() );
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

    mappersetup_->usePar( iop );

    BufferString seqnm = iop.find( sKey::Name() );
    if ( !seqnm.isEmpty() )
	colseq_ = ColTab::SeqMGR().getAny( seqnm );

    mSendChgNotif( cColSeqChange(), cUnspecChgID() );
    return true;
}
