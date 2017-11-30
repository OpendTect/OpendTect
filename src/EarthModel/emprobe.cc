/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "emprobe.h"
#include "keystrs.h"
#include "emmanager.h"

mDefineInstanceCreatedNotifierAccess( EMProbe );


EMProbe::EMProbe( const ObjectID& objid )
    : Probe()
    , objid_(objid)
{
    mTriggerInstanceCreatedNotifier();
}


EMProbe::EMProbe( const EMProbe& oth )
    : Probe(oth)
    , objid_(oth.objid_)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


EMProbe::~EMProbe()
{
    sendDelNotif();
}


mImplMonitorableAssignment( EMProbe, Probe )

void EMProbe::copyClassData( const EMProbe& oth )
{
    objid_ = oth.objid_;
}


Monitorable::ChangeType EMProbe::compareClassData(
					const EMProbe& oth ) const
{
    mDeliverYesNoMonitorableCompare( objid_ == oth.objid_ );
}


BufferString EMProbe::getDisplayName() const
{
    return EM::EMM().objectName( EM::EMM().getDBKey(objid_) );
}


void EMProbe::updateAll()
{
    mLock4Write();

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cUnspecChgID() );
}


void EMProbe::setID( const EM::ObjectID& objid )
{
    mLock4Read();

    if ( objid_ == objid )
	return;

    if ( !mLock2Write() && objid_ == objid )
	return;

    objid_ = objid;

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cUnspecChgID() );
}


void EMProbe::fillPar( IOPar& par ) const
{
    Probe::fillPar( par );
    par.set( sKey::Object(), EM::EMM().getDBKey(objid_) );
}


bool EMProbe::usePar( const IOPar& par )
{
    if ( !Probe::usePar(par) )
	return false;

    DBKey dbky( EM::EMM().getDBKey(objid_) );
    if ( !par.get(sKey::Object(),dbky) || dbky.isInvalid() )
	return false;

    setID( EM::EMM().getObjectID(dbky) );
    return true;
}


mDefineInstanceCreatedNotifierAccess(Horizon3DProbe)


Horizon3DProbe::Horizon3DProbe( const ObjectID& objid )
    : EMProbe(objid)
{
    mTriggerInstanceCreatedNotifier();
}


Horizon3DProbe::Horizon3DProbe( const Horizon3DProbe& oth )
    : EMProbe(oth)
{
    copyClassData( oth );
    mTriggerInstanceCreatedNotifier();
}


Horizon3DProbe::~Horizon3DProbe()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Horizon3DProbe, Probe )

void Horizon3DProbe::copyClassData( const Horizon3DProbe& oth )
{
}


Monitorable::ChangeType Horizon3DProbe::compareClassData(
					const Horizon3DProbe& oth ) const
{
    mDeliverYesNoMonitorableCompare( true );
}


const char* Horizon3DProbe::sFactoryKey()
{
    return "Horizon3D"; //TODO impl proper
}


void Horizon3DProbe::fillPar( IOPar& par ) const
{
    EMProbe::fillPar( par );
}


bool Horizon3DProbe::usePar( const IOPar& par )
{
    return EMProbe::usePar( par );
}


Probe* Horizon3DProbe::createFrom( const IOPar& par )
{
    Horizon3DProbe* probe = new Horizon3DProbe();
    if ( !probe->usePar(par) )
	{ delete probe; return 0; }

    return probe;
}


void Horizon3DProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}
