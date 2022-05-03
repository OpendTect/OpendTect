#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          August 2010
________________________________________________________________________

-*/

#include "mmprocmod.h"

#include "gendefs.h"
#include "networkcommon.h"
#include "odjson.h"

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
    static PortNr_Type	remoteHandlerPort()
					    { return mCast(PortNr_Type,5050); }
    static const char*	remoteHandlerName()	{ return "od_remoteservice"; }

protected:

    void		checkConnection();

    OD::JSON::Object	par_;
    const Network::Authority	auth_;

};


