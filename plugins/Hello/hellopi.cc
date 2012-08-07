/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003 / Apr 2011
-*/

static const char* rcsID mUnusedVar = "$Id: hellopi.cc,v 1.12 2012-08-07 04:23:04 cvsmahant Exp $";

#include "odplugin.h"
#include <iostream>

mDefODInitPlugin(Hello)
{
    std::cout << "Hello world! Speed 1 THz" << std::endl;
    return 0; // All OK - no error messages
}
