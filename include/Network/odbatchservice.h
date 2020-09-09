#pragma once
/*+
* ________________________________________________________________________
*
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* Author:	Prajjaval Singh
* Date:		July 2020
* RCS:		$Id$
* ________________________________________________________________________
*
* -*/

#include "networkmod.h"

#include "networkcommon.h"
#include "odservicebase.h"

class Timer;
// repimplement closeApp
// input out manager

mExpClass(Network) ODBatchService : public ODServiceBase
{ mODTextTranslationClass(ODBatchService)
public:

    typedef int ID;

    virtual			~ODBatchService();

    static ODBatchService&	getMgr();
    static ODBatchService&	getMgr(bool islocal);

    bool			isODMainSlave() const;
    bool			isMasterAlive() const;

    void			processingComplete();

protected:

			ODBatchService(bool assignport=false);
			ODBatchService(const char* hostname,
			    bool assignport);
			explicit ODBatchService(bool islocal,
			    const char* servernm,bool assignport);

    uiRetVal			sendAction(const char* act) const;
    uiRetVal			sendRequest(const char* reqkey,
						const OD::JSON::Object&) const;
    bool			doParseAction(const char*,uiRetVal&) override;
    uiRetVal			close();

    uiRetVal			doRegister();
    uiRetVal			doDeRegister();

private:
				ODBatchService(const ODBatchService&) = delete;
				ODBatchService(ODBatchService&&) = delete;

    ODBatchService&		operator=(const ODBatchService&) = delete;
    ODBatchService&		operator=(ODBatchService &&) = delete;

    void			init(bool islocal);

    void			handleMasterCheckTimer(bool start);
    void			doAppClosing(CallBacker*) override;

    Network::Authority		odauth_;
    ID				servid_ = 0;

    Timer*			mastercheck_ = nullptr;

};
