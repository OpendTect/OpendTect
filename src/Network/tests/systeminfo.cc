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
        CallBack::addToMainThread( mCB( this, TestClass, closeTesterCB ) );
    }

    void closeTesterCB( CallBacker* )
    {
        ApplicationData::exit( retval_ );
    }

    bool testSystemInfo()
    {
        //Dummy test in a sense, as we cannot check the result
        mRunStandardTest( System::macAddressHash(),
            "macAddressHash" );
        const BufferString localaddress = System::localAddress();
        mRunStandardTest( !localaddress.isEmpty(),
            "Local address" );

        return true;
    }

    Timer   timer_;
    int     retval_ = 0;

};




int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    ApplicationData app;
    TestClass tester;

    return app.exec();
}
