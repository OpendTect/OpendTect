#ifndef uiodservicemgr_h
#define uiodservicemgr_h
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	Wayne Mogg
 * Date:		Oct 2019
 * RCS:		$Id$
 * ________________________________________________________________________
 *
 * -*/

#include "uiodmainmod.h"

#include "netservice.h"
#include "uiodservice.h"



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


mExpClass(uiODMain) uiODServiceMgr : public uiODService
{ mODTextTranslationClass(uiODServiceMgr)
public:

    static void		setFor(uiMainWin&);
    static uiODServiceMgr& getMgr();

    bool		isPresent(const Network::Service::ID) const;
    bool		isAlive(const Network::Service::ID) const;
    BufferString	serviceName(const Network::Service::ID) const;
    void		raise(const Network::Service::ID) const;
    void		removeService(const Network::Service::ID);

    CNotifier<uiODServiceMgr,Network::Service::ID>	serviceAdded;
    CNotifier<uiODServiceMgr,Network::Service::ID>	serviceRemoved;

protected:

				uiODServiceMgr();
				uiODServiceMgr(const uiODServiceMgr&) = delete;
				uiODServiceMgr(uiODServiceMgr&&) = delete;
				~uiODServiceMgr();

    uiRetVal		sendAction(const Network::Service::ID,
	const char*) const;
    uiRetVal		sendAction(const Network::Service&,const char*) const;
    uiRetVal		sendRequest(const Network::Service&,const char*,
	const OD::JSON::Object&) const;

    virtual bool	doParseAction(const char*,uiRetVal&);
    virtual bool	doParseRequest(const OD::JSON::Object&,
	uiRetVal&);

private:

    uiODServiceMgr&	operator=(const uiODServiceMgr&) = delete;
    uiODServiceMgr&	operator=(uiODServiceMgr &&) = delete;

    ObjectSet<Network::Service> services_;

    void		doSurveyChanged(CallBacker*) override;
    void		doPyEnvChange(CallBacker*) override final;
    void		doAppClosing(CallBacker*) override final;
    void		closeApp() override final;

    uiRetVal		addService(const OD::JSON::Object*);
    uiRetVal		removeService(const OD::JSON::Object*);
    uiRetVal		startApp(const OD::JSON::Object*);
    const Network::Service*	getService(const Network::Service::ID) const;
    Network::Service*	getService(const Network::Service::ID);

    friend class uiODMain;

};
#endif
