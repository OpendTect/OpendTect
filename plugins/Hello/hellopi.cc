/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003 / Apr 2011
-*/

static const char* rcsID mUnusedVar = "$Id: hellopi.cc,v 1.14 2012-08-27 13:53:31 cvskris Exp $";

#include "hellomod.h"

#include "odplugin.h"
#include <iostream>

mDefODInitPlugin(Hello)
{
    std::cout << "Hello world" << std::endl;
    return 0; // All OK - no error messages
}
