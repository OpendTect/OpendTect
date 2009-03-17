/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Mar 2009
-*/

static const char* rcsID = "$Id: odusgclient.cc,v 1.1 2009-03-17 12:53:18 cvsbert Exp $";

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
