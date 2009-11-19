/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgclient.cc,v 1.6 2009-11-19 12:17:59 cvsbert Exp $";

#include "odusgclient.h"
#include "odusginfo.h"
#include <iostream>


bool Usage::Client::sendUsgInfo() const
{
    usginfo_.id_ = Usage::Info::newID();

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

    return true;
}
