/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2003 / Apr 2011
-*/


#include "hellomod.h"

#include "odplugin.h"
#include "od_ostream.h"

mDefODInitPlugin(Hello)
{
    od_cout() << "Hello world" << od_endl;
    return 0; // All OK - no error messages
}
