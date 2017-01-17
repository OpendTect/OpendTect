/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		September 2016
___________________________________________________________________

-*/

#include "randomlineprobe.h"
#include "randomlinegeom.h"
#include "keystrs.h"
#include "survinfo.h"
#include "uistrings.h"


static int newrdlid =0;

Geometry::RandomLine* RandomLineProbe::createNewDefaultRDL()
{
    newrdlid++;
    uiString newrdlname =
	uiStrings::phrJoinStrings( uiStrings::sRandomLine(),
				   toUiString(newrdlid) );
    Geometry::RandomLine* rl =
	new Geometry::RandomLine( newrdlname.getFullString() );
    const StepInterval<int> inlrange = SI().inlRange( true );
    const StepInterval<int> crlrange = SI().crlRange( true );
    const BinID start( inlrange.snappedCenter(), crlrange.start );
    const BinID stop( start.inl(), crlrange.stop );
    rl->addNode( start );
    rl->addNode( stop );
    rl->setZRange( SI().zRange(true) );
    return rl;
}


mDefineInstanceCreatedNotifierAccess( RandomLineProbe );

RandomLineProbe::RandomLineProbe( int rdlid )
    : Probe()
    , rdlid_(rdlid)
{
    if ( rdlid_<0 )
    {
	Geometry::RandomLine* newrl = createNewDefaultRDL();
	rdlid_ = newrl->ID();
    }

    mTriggerInstanceCreatedNotifier();
}

const char* RandomLineProbe::sFactoryKey()
{ return IOPar::compKey(sKey::Random(),sKey::Line()); }


const char* RandomLineProbe::sRandomLineID()
{ return "RandomLineID"; }


BufferString RandomLineProbe::getDisplayName() const
{
    BufferString rdlname;
    const Geometry::RandomLine* rdl = Geometry::RLM().get( rdlid_ );
    if ( rdl )
	rdlname = rdl->name();
    return rdlname;
}


void RandomLineProbe::geomUpdated()
{
    mLock4Write();

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cEntireObjectChangeID() );
}


void RandomLineProbe::setRandomLineID( int rdlid )
{
    mLock4Read();

    if ( rdlid_ == rdlid )
	return;

    if ( !mLock2Write() && rdlid_ == rdlid )
	return;

    rdlid_ = rdlid;

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cEntireObjectChangeID() );
}


void RandomLineProbe::fillPar( IOPar& par ) const
{
    Probe::fillPar( par );
    par.set( sRandomLineID(), rdlid_ );
}

bool RandomLineProbe::usePar( const IOPar& par )
{
    if ( !Probe::usePar(par) )
	return false;

    return par.get( sRandomLineID(), rdlid_ );
}


mImplMonitorableAssignment( RandomLineProbe, Probe )

void RandomLineProbe::copyClassData( const RandomLineProbe& oth )
{
    rdlid_ = oth.randomeLineID();
}


Probe* RandomLineProbe::createFrom( const IOPar& par )
{
    RandomLineProbe* probe = new RandomLineProbe();
    if ( !probe->usePar(par) )
    {
	delete probe;
	return 0;
    }

    return probe;
}


void RandomLineProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}
