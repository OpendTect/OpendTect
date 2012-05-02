/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003 / Apr 2011
-*/

static const char* mUnusedVar rcsID = "$Id: hellopi.cc,v 1.10 2012-05-02 11:52:46 cvskris Exp $";

#include "odplugin.h"
#include <iostream>

mDefODInitPlugin(Hello)
{
    std::cout << "Hello world" << std::endl;
    return 0; // All OK - no error messages
}
