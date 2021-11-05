#pragma once
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	Wayne Mogg
 * Date:	Oct 2019
 * ________________________________________________________________________
 *
 * -*/

#include "uiodmainmod.h"

#include "uiserviceservermgr.h"

#include "clientservicebase.h"


/*!\brief The OpendTect service manager
 Manage communication between ODMain and external services/apps so that
 the external app can show behaviour/state consistent with ODMain. Items
 communicated by ODMain are:
 - ODMain is closing
 - ODMain mainwindow has been raised or hidden
 - User has changed the Python environment settings
 - User has changed the Survey

 The external app can send messages to the ODMain server:
 - App/Service registration packet containing information about the service,
   normally sent when the service starts up
 - App/Service exit message
 - User has changed the Python environment settings from the App/Service

 Communications take the form of network packets containing JSon objects
 Typical usage:
 - App/Service is provided with the ODMain server hostname and port when it
   starts
 - App/Service connects to the ODMain server and sends a registration packet
 - ODMain server uses the registration packet to register the service
 - ODMain and the App/Service exchange messages and respond as required
 - App/Service sends an exit message, ODMain deregisters the service

 */

class uiMainWin;


mExpClass(uiODMain) uiServiceClientMgr : public ServiceClientMgr
{ mODTextTranslationClass(uiServiceClientMgr)
public:

    virtual		~uiServiceClientMgr();

    static void		setFor(uiMainWin&);
    static uiServiceClientMgr& getMgr();

    bool		isOK() const;

    void		raise(const Network::Service::ID) const;

private:
			uiServiceClientMgr();

    bool		canClaimService(const Network::Service&) const override;

	/* Implement an action/request send from an external application, that
	   must be handled by od_main. Can be used to transfer signals
	   emitted by the external application, to make od_main aware of it */

    bool		canParseAction(const char*,uiRetVal&) override;
    bool		canParseRequest(const OD::JSON::Object&,
				       uiRetVal&) override;

    uiRetVal		doHandleAction(const char*) override;
    uiRetVal		doHandleRequest(const OD::JSON::Object&) override;
    uiRetVal		startWorkflow(const OD::JSON::Object&);
    uiRetVal		logsChanged(const OD::JSON::Object&);

    void		doPyEnvChange(CallBacker*) override final;
    void		doSurveyChanged(CallBacker*) override final;
    void		doAppClosing(CallBacker*) override final;
    void		closeApp() override final;

};
