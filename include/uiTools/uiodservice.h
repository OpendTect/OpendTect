#ifndef uiodservice_h
#define uiodservice_h
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	Wayne Mogg
 * Date:	Oct 2019
 * RCS:		$Id$
 * ________________________________________________________________________
 *
 * -*/

#include "uitoolsmod.h"

#include "networkcommon.h"
#include "odservicebase.h"



mExpClass(uiTools) uiODService : public ODServiceBase
{ mODTextTranslationClass(uiODService)
public:

    typedef int ID;

    virtual		~uiODService();

    bool		isODMainSlave() const;
    bool		isMasterAlive() const;
    void		setBackground();

protected:

			uiODService(bool assignport=false);

    uiRetVal		sendAction(const char* act) const;
    uiRetVal		sendRequest(const char* reqkey,
				    const OD::JSON::Object&) const;
    uiRetVal		doAction(const OD::JSON::Object&) override;
    uiRetVal		close();


private:
			uiODService(const uiODService&) = delete;
			uiODService(uiODService&&) = delete;

    uiODService&	operator=(const uiODService&) = delete;
    uiODService&	operator=(uiODService &&) = delete;

    uiRetVal		doRegister();
    uiRetVal		doDeRegister();
    void		handleMasterCheckTimer(bool start);
    void		doAppClosing(CallBacker*) override;
    void		doPyEnvChange(CallBacker*) override;
    void		masterCheckCB(CallBacker*);
    void		connClosedCB(CallBacker*) override;

    Network::Authority	odauth_;
    ID			servid_ = 0;

    Timer*		mastercheck_ = nullptr;

};

#endif
