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

static Threads::Atomic<int> newrdlid(0);
const char* sKeyRandomLineID = "RandomLineID";

mDefineInstanceCreatedNotifierAccess( RandomLineProbe );


Geometry::RandomLine* RandomLineProbe::createNewDefaultRDL()
{
    newrdlid++;
    uiString newrdlname = toUiString( "%1 %2" ).arg(uiStrings::sRandomLine())
						.arg(newrdlid);
    Geometry::RandomLine* rl =
		new Geometry::RandomLine( toString(newrdlname) );
    const StepInterval<int> inlrange = SI().inlRange( OD::UsrWork );
    const StepInterval<int> crlrange = SI().crlRange( OD::UsrWork );
    const BinID start( inlrange.snappedCenter(), crlrange.start );
    const BinID stop( start.inl(), crlrange.stop );
    rl->addNode( start );
    rl->addNode( stop );
    rl->setZRange( SI().zRange(OD::UsrWork) );
    return rl;
}


RandomLineProbe::RandomLineProbe( int rdlid )
    : Probe()
    , rdlid_(rdlid)
{
    if ( rdlid_ < 0 )
    {
	Geometry::RandomLine* newrl = createNewDefaultRDL();
	rdlid_ = newrl->ID();
    }

    mTriggerInstanceCreatedNotifier();
}


RandomLineProbe::RandomLineProbe( const RandomLineProbe& oth )
    : Probe(oth)
    , rdlid_(oth.rdlid_)
{
    copyClassData( oth );

    mTriggerInstanceCreatedNotifier();
}


RandomLineProbe::~RandomLineProbe()
{
    sendDelNotif();
}


mImplMonitorableAssignment( RandomLineProbe, Probe )

void RandomLineProbe::copyClassData( const RandomLineProbe& oth )
{
    rdlid_ = oth.randomeLineID();
}


Monitorable::ChangeType RandomLineProbe::compareClassData(
					const RandomLineProbe& oth ) const
{
    mDeliverYesNoMonitorableCompare( rdlid_ == oth.randomeLineID() );
}


const char* RandomLineProbe::sFactoryKey()
{
    return sKey::RandomLine();
}


uiWord RandomLineProbe::usrType() const
{
    return uiStrings::sRandomLine();
}


uiWord RandomLineProbe::displayName() const
{
    uiWord ret;
    const Geometry::RandomLine* rdl = Geometry::RLM().get( rdlid_ );
    if ( rdl )
	ret = toUiString(rdl->name());
    return ret;
}


void RandomLineProbe::geomUpdated()
{
    mLock4Write();

    for ( int idx=0; idx<layers_.size(); idx++ )
	layers_[idx]->invalidateData();

    mSendChgNotif( cPositionChange(), cUnspecChgID() );
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

    mSendChgNotif( cPositionChange(), cUnspecChgID() );
}


void RandomLineProbe::fillPar( IOPar& par ) const
{
    Probe::fillPar( par );
    par.set( sKeyRandomLineID, rdlid_ );
}

bool RandomLineProbe::usePar( const IOPar& par )
{
    if ( !Probe::usePar(par) )
	return false;

    return par.get( sKeyRandomLineID, rdlid_ );
}


Probe* RandomLineProbe::createFrom( const IOPar& par )
{
    RandomLineProbe* probe = new RandomLineProbe();
    if ( !probe->usePar(par) )
	{ delete probe; return 0; }

    return probe;
}


void RandomLineProbe::initClass()
{
    ProbeFac().addCreateFunc( createFrom, sFactoryKey() );
}
