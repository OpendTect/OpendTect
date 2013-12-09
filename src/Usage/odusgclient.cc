/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "odusgclient.h"
#include "odusginfo.h"
#include "callback.h"
#include "genc.h"
#include "ptrman.h"
#include "od_ostream.h"


namespace Usage
{

class ProcExit : public CallBacker
{
public:

ProcExit()
{
    NotifyExitProgram( &atEnd );
}

static void atEnd()
{
    Usage::Client ucl( "*" );
    ucl.prepUsgEnd( "*" );
    ucl.sendUsgInfo();
}


};

}


bool Usage::Client::sendUsgInfo() const
{
    usginfo_.prepareForSend();

#ifdef __debug__
    od_cout() <<  "UsageInfo: ";
    usginfo_.dump( od_cout() );

    if ( usginfo_.withreply_ )
    {
	mDefineStaticLocalObject( int, iret, = 1 );
	usginfo_.withreply_ = false;
	usginfo_.aux_ = "OK ";
	usginfo_.aux_ += iret++;
	od_cout() << "\t\tREPLY '" << usginfo_.aux_ << "'\n";
    }

    od_cout() << od_endl;
#endif

    // only go here if send succeeded
    mDefineStaticLocalObject( PtrMan<ProcExit>, thepe, = new ProcExit );
    return true;
}
