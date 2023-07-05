#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "gendefs.h"
#include "networkcommon.h"
#include "odjson.h"
#include "ptrman.h"

namespace Network { class Socket; }


/*!
\brief Remote job executor
*/

mExpClass(MMProc) RemoteJobExec : public CallBacker
{
public:
			RemoteJobExec(const Network::Authority&);
			~RemoteJobExec();

    bool		launchProc() const;
    void		addPar(const OD::JSON::Object&);
    static PortNr_Type	getLocalHandlerPort();
    static PortNr_Type	legacyRemoteHandlerPort();
    static PortNr_Type	stdRemoteHandlerPort();
    mDeprecated("Use getLocalHandlerPort")
    static PortNr_Type	remoteHandlerPort()
					    { return mCast(PortNr_Type,5050); }
    static const char*	remoteHandlerName()	{ return "od_remoteservice"; }

protected:

    void		checkConnection();

    PtrMan<OD::JSON::Object>	par_;
    const Network::Authority	auth_;

};
