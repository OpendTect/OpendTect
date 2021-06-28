/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
 * FUNCTION :
-*/


#include "applicationdata.h"
#include "systeminfo.h"
#include "timer.h"

#include "testprog.h"


class TestClass : public CallBacker
{
public:
    TestClass()
        : timer_( "starter" )
    {
        mAttachCB( timer_.tick, TestClass::timerTick );
        timer_.start( 0, true );
    }

    ~TestClass()
    {
        detachAllNotifiers();
        CallBack::removeFromThreadCalls( this );
    }

    void timerTick( CallBacker* )
    {
	retval_ = testSystemInfo() ? 0 : 1;
	CallBack::addToMainThread( mCB(this,TestClass,closeTesterCB) );
    }

    void closeTesterCB( CallBacker* )
    {
	ApplicationData::exit( retval_ );
    }

    bool testSystemInfo()
    {
	//Dummy test in a sense, as we cannot check the result
	mRunStandardTest( System::macAddressHash(), "macAddressHash" );

	const BufferString localaddress = System::localAddress();
	mRunStandardTest( !localaddress.isEmpty(), "Local address" );

	BufferString hostaddress = System::hostAddress( "localhost", true );
	mRunStandardTest( hostaddress=="127.0.0.1", "localhost address ipv4" );

	hostaddress = System::hostAddress( "localhost", false );
	mRunStandardTest( hostaddress=="::1", "localhost address ipv6" );

	hostaddress = System::hostAddress( "dgb29", true );
	mRunStandardTest( hostaddress=="192.168.0.29", "dgb29 ipv4" );

	hostaddress = System::hostAddress( "dgb29", false );
	mRunStandardTest( !hostaddress.isEmpty(), "dgb29 ipv6" );

	const BufferString dgb29hostname =
		__iswin__ ? "dgb29.DGBES.local" : "dgb29.enschede.dgbes.com";
	hostaddress = System::hostAddress( dgb29hostname );
	mRunStandardTest( hostaddress=="192.168.0.29", "dgb29.domain ipv4" );

	hostaddress = System::hostAddress( dgb29hostname, false );
	mRunStandardTest( !hostaddress.isEmpty(), "dgb29.domain ipv6" );

	return true;
    }

    Timer	timer_;
    int		retval_ = 0;

};




int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    ApplicationData app;
    TestClass tester;

    return app.exec();
}
