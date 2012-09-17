/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgclient.cc,v 1.7 2009/11/25 16:09:21 cvsbert Exp $";

#include "odusgclient.h"
#include "odusginfo.h"
#include "callback.h"
#include "genc.h"
#include <iostream>


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
    std::cerr << "UsageInfo: ";
    usginfo_.dump( std::cerr );

    if ( usginfo_.withreply_ )
    {
	static int iret = 1;
	usginfo_.withreply_ = false;
	usginfo_.aux_ = "OK ";
	usginfo_.aux_ += iret++;
	std::cerr << "\t\tREPLY '" << usginfo_.aux_ << "'\n";
    }

    std::cerr << std::endl;
#endif

    // only go here if send succeeded
    static ProcExit* thepe = 0;
    if ( !thepe )
	thepe = new ProcExit;

    return true;
}
