/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmexecproxy.h"


MMProc::ExecProxy::ExecProxy( const char* pnm, const char* hnm )
    : prognm_(pnm)
    , hostnm_(hnm)
    , msg_(tr("Not started"))
{
}


MMProc::ExecProxy::~ExecProxy()
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
