#pragma once
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
#include "uiodservice.h"
#include "uistring.h"
#include "netreqconnection.h"
#include "netservice.h"
#include "manobjectset.h"


/*!\brief The OpendTect service manager
 M a*nagers communication between ODMain and external services/apps so that
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

namespace OD {
    namespace JSON {
	class Object;
    };
};

mExpClass(uiODMain) uiODServiceMgr : public uiODServiceBase
{ mODTextTranslationClass(uiODServiceMgr)
public:
    friend class uiODMain;
    static uiODServiceMgr& getMgr();

    uiODServiceMgr(uiODServiceMgr const&) = delete;
    uiODServiceMgr(uiODServiceMgr&&) = delete;
    uiODServiceMgr& operator=(uiODServiceMgr const&) = delete;
    uiODServiceMgr& operator=(uiODServiceMgr &&) = delete;

    void	raise( const Network::Service& );

    uiRetVal	sendAction( const Network::Service&, const char*,
					    OD::JSON::Object* pobj=nullptr );
    uiRetVal	sendAction( int idx, const char* action,
					    OD::JSON::Object* pobj=nullptr );


    int		indexOfService( const Network::Service& ) const;
    bool	isServicePresent( const Network::Service& service ) const
    { return indexOfService( service ) >=0; }

    BufferString	address() const;

protected:
    uiODServiceMgr();
    ~uiODServiceMgr();

    ManagedObjectSet<Network::Service>	services_;

    void		packetArrivedCB(CallBacker*);
    void		newConnectionCB(CallBacker*);
    void		connClosedCB(CallBacker*);

    void surveyChangedCB(CallBacker*);
    void appClosingCB(CallBacker*);
    void pyenvChangeCB(CallBacker*);

    uiRetVal		addService( const OD::JSON::Object* );
    uiRetVal		removeService( const OD::JSON::Object* );
    void		removeService( const Network::Service& );

    uiRetVal		doAction(const OD::JSON::Object*);

};
