#pragma once
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	  Wayne Mogg
 * Date:	  Apr 2022
 * ________________________________________________________________________
 *
 * -*/


#include "mmprocmod.h"

#include "filepath.h"
#include "namedobj.h"
#include "netservice.h"
#include "odplatform.h"

namespace Network { class Authority; }

mExpClass(MMProc) MMPServerClient : public NamedCallBacker
{ mODTextTranslationClass(MMPServerClient)
public:
			MMPServerClient(PortNr_Type portid,
					const char* hostnm=nullptr,
					int timeout=4000);
			MMPServerClient(const Network::Authority&,
					int timeout=4000);
			~MMPServerClient();
			MMPServerClient(const MMPServerClient&) = delete;
    MMPServerClient	operator=(const MMPServerClient&) = delete;

    uiRetVal		errMsg() const		{ return errmsg_; }
    bool		isOK() const;
    bool		refresh();
    void		setTimeout(int timeout) { timeout_ = timeout; }

    OD::JSON::Object	sendRequest(const char* reqkey,
				    const OD::JSON::Object& reqobj );

    bool		getServerDataRoot();
    bool		setServerDataRoot(const char*);
    bool		validServerDataRoot(const char*);

    BufferString	serverDataRoot() const	{ return svr_drfp_.fullPath(); }
    BufferString	serverODVer() const	{ return svr_odver_; }
    OD::Platform	serverPlatform() const	{ return svr_platform_; }
    const Network::Service&	serverService() const	{ return mmpservice_; }
    void		stopServer(bool removelog=false);

    Notifier<MMPServerClient>	logFileChg;
    Notifier<MMPServerClient>	errorNotice;

protected:
    Network::Service&	mmpservice_;
    BufferString	svr_odver_;
    FilePath		svr_drfp_;
    OD::Platform	svr_platform_;
    int			timeout_;
    uiRetVal		errmsg_;


    bool		checkServer( const Network::Authority& auth );

    OD::JSON::Object	sendRequest(const Network::Authority&,
				    const char* reqkey,
				    const OD::JSON::Object& reqobj );
};
