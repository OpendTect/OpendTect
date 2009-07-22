/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgclient.cc,v 1.4 2009-07-22 16:01:35 cvsbert Exp $";

#include "odusgclient.h"
#include "odusginfo.h"
#include <iostream>


bool Usage::Client::sendUsageInfo()
{
    static int iret = 1;
#ifdef __debug__
    std::cerr << "Usage::Client::sendUsageInfo: ";
    usginfo_.dump( std::cerr ) << std::endl;
    usginfo_.aux_ = "aux ret";
    iret++;
#endif
    return iret % 2;
}
