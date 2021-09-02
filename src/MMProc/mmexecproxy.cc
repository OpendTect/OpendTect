/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Sep 2006
-*/


#include "mmexecproxy.h"


MMProc::ExecProxy::ExecProxy( const char* pnm, const char* hnm )
    : prognm_(pnm)
    , hostnm_(hnm)
    , msg_(tr("Not started"))
{
}


bool MMProc::ExecProxy::launch( const IOPar& iop )
{
    return false;
}


MMProc::ExecProxy::Status MMProc::ExecProxy::update()
{
    return NotStarted;
}
