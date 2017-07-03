/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2016
________________________________________________________________________

-*/

#include "networkmod.h"
#include "moddepmgr.h"

void NetworkHttpFileSystemAccessinitClass();


mDefModInitFn(Network)
{
    mIfNotFirstTime( return );

    NetworkHttpFileSystemAccessinitClass();
}
