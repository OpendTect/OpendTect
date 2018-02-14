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
static Threads::Atomic<int> thrnr_( 1 );
static Threads::Atomic<int> nrthrdsfinished_( 0 );

static void runThread( CallBacker* )
{
    const int thrnr = thrnr_++;

    for ( int idx=0; idx<10000; idx++ )
    {
	const float newz = (float)(999.75 + 0.1 * thrnr);
	ps_->setZ( locid1_, newz );
	const float newerz1 = ps_->get( locid1_ ).z();
	ps_->setZ( locid2_, newz );
	const float newerz2 = ps_->get( locid2_ ).z();
	if ( newz != newerz1 || newz != newerz2 )
	{
	    if ( newz > 1001 || newz < 999 || newerz1 < 999 || newerz1 > 1001
		|| newerz2 < 999 || newerz2 > 1001 )
		tstStream( false ) << "Corruption in " << thrnr << od_endl;
	    else
	    {
		Threads::sleep( 0.001 );
		tstStream( false ) << thrnr;
	    }
	}
    }

    nrthrdsfinished_++;
}

static void startThread( int nr )
{
    const BufferString thrdnm( "PointSet Test Thread ", nr );
    new Threads::Thread( mSCB(runThread), thrdnm );
    tstStream( false ) << "Started thread " << thrdnm << od_endl;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    ps_ = new Pick::Set;
    locid1_ = ps_->add( Pick::Location(100000.,200000.,1000.) );
    locid2_ = ps_->add( Pick::Location(100001.,200001.,1000.1) );

    startThread( 1 );
    startThread( 2 );
    startThread( 3 );
    startThread( 4 );
    startThread( 5 );

    while ( nrthrdsfinished_ < 5 )
	Threads::sleep( 0.1 );

    tstStream( false ) << "\n\nAll threads finished." << od_endl;
    return 0;
}
