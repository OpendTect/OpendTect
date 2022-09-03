/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "envvars.h"
#include "oscommand.h"
#include "testprog.h"
#include "commandlaunchmgr.h"

#include <iostream>

using namespace Threads;

class TestClass : public CallBacker
{
public:
    TestClass(const char* expected_ouput)
	: expout_(expected_ouput)
    {}

    ~TestClass()
    {}

    void finishedCB( CallBacker* cb )
    {
	const auto* cmdtask = CommandLaunchMgr::getMgr().getCommandTask( cb );
	finished_ = true;
	if ( cmdtask )
	{
	    output_ = cmdtask->getStdOutput();
	    error_ = cmdtask->getStdError();
	}
    }

    void wait4Finish()
    {
	while ( !finished_ )
	    Threads::sleep( 1 );
    }

    bool isOK( bool isstderr=false )
    {
	return finished_ && (isstderr ? error_==expout_ : output_==expout_);
    }

    BufferString output_;
    BufferString error_;
    bool finished_ = false;

    BufferString expout_;
};

static bool testCmdWithCallback()
{
    BufferString hello( "Hello Test with Callback" );
    const OS::MachineCommand machcomm1( "echo", hello.buf() );
    const OS::MachineCommand machcomm2( "echo", hello, ">&2" );
    if ( __iswin__ )
	hello.quote( '\"' );

    CommandLaunchMgr& mgr = CommandLaunchMgr::getMgr();
    {
	TestClass test( hello );
	CallBack cb( mCB(&test,TestClass,finishedCB) );
	mgr.execute( machcomm1, true, true, &cb );
	test.wait4Finish();
	mRunStandardTest( test.isOK(), "CommandLaunchMgr::read stdout" );
    }
    {
	if ( __iswin__ )
	    hello.addSpace();

	TestClass test( hello );
	CallBack cb( mCB(&test,TestClass,finishedCB) );
	mgr.execute( machcomm2, true, true, &cb );
	test.wait4Finish();
	mRunStandardTest( test.isOK(true), "CommandLaunchMgr::read stderr" );
    }

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    // Debugging output screws up the command output, needs to be disabled:
    UnsetOSEnvVar( "DTECT_DEBUG" );

    const bool result = testCmdWithCallback();

    return result ? 0 : 1;
}
