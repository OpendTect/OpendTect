/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          April 2016
________________________________________________________________________

-*/

#include "webstreamsource.h"

#include "webfile.h"
#include "odnetworkaccess.h"
#include "atomic.h"
#include "moddepmgr.h"


mDefModInitFn(Network)
{
    mIfNotFirstTime( return );

    WebStreamSource::initClass();

    File::setWebHandlers( Network::exists, Network::getFileSize,
			  Network::getContent );
}
