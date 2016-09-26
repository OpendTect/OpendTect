/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2016
-*/


#include "testprog.h"
#include "pickset.h"
#include "statrand.h"
#include "thread.h"

static RefMan<Pick::Set> ps_;
static Pick::Set::LocID locid1_;
static Pick::Set::LocID locid2_;
static Threads::Atomic<int> thrnr_( 0 );
static Threads::Atomic<int> nrthrdsfinished_( 0 );

static void runThread( CallBacker* )
{
    const int thrnr = thrnr_++;
    const bool hastext = thrnr % 2 != 0;
    const int nradds = Stats::randGen().getIndex( 10 );

    for ( int idx=0; idx<nradds; idx++ )
    {
	const int addnr = nradds + idx * 25;
	Pick::Location pl (100001.+addnr, 200001.+addnr, 1001.+addnr );
	if ( hastext )
	    pl.setText( BufferString(toString(idx),thrnr," aap") );
	ps_->add( pl );
    }

    for ( int idx=0; idx<10000; idx++ )
    {
	float newz = 1000.f;
	if ( idx % 2 )
	{
	    MonitorLock ml( *ps_ );
	    newz = ps_->getByIndex( 1 ).z();
	}
	ps_->setZ( locid1_, newz );
    }

    nrthrdsfinished_++;
}

static void startThread( int nr )
{
    const BufferString thrdnm( "PickSet Test Thread ", nr );
    new Threads::Thread( mSCB(runThread), thrdnm );
    tstStream( false ) << "Started thread " << thrdnm << od_endl;
}


int testMain( int argc, char** argv )
{
    mInitTestProg();

    ps_ = new Pick::Set;
    locid1_ = ps_->add( Pick::Location(100000.,200000.,1000.) );
    locid2_ = ps_->add( Pick::Location(100001.,200001.,1001.) );

    startThread( 1 );
    startThread( 2 );
    startThread( 3 );

    while ( nrthrdsfinished_ < 3 )
	Threads::sleep( 1 );

    tstStream( false ) << "All threads finished." << od_endl;
    return 0;
}
