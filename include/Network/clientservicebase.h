#pragma once
/*+
* ________________________________________________________________________
*
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* Author:	A. Huck
* Date:		Oct 2020
* ________________________________________________________________________
*
* -*/

#include "servicemgrbase.h"

class uiServiceClientMgr;


/*!\
  brief Base class for client applications that register spawn services
  and provide interaction with them
 */


mExpClass(Network) ServiceClientMgr : public ServiceMgrBase
{ mODTextTranslationClass(ServiceClientMgr);
public:

    virtual			~ServiceClientMgr();

    bool			isPresent(const Network::Service::ID) const;
    bool			isAlive(const Network::Service::ID) const;
    BufferString		serviceName(const Network::Service::ID) const;
    Network::Service::SubID	serviceSubID(const Network::Service::ID) const;
    bool			stopService(const Network::Service::ID);

    CNotifier<ServiceClientMgr,Network::Service::ID> serviceAdded;
    CNotifier<ServiceClientMgr,Network::Service::ID> serviceRemoved;

	/* Require an action or request to be done by an external application
	   started by od_main. The return value indicates if the action was
	   send, received and processed successfully by this application */
    uiRetVal		sendAction(const Network::Service::ID,
				   const char*) const;
    uiRetVal		sendRequest(const Network::Service::ID,const char*,
				    const OD::JSON::Object& ) const;

    void		printInfo(const Network::Service::ID,
				  const char* desc=nullptr,
				  od_ostream* = nullptr) const;

    static bool		addApplicationAuthority(OS::MachineCommand&);
    BufferString	getLockFileFP(const Network::Service::ID) const;
protected:
				ServiceClientMgr(const char* servicenm);
				ServiceClientMgr(const char* servicenm,
					    bool assignport,
					    Network::SpecAddr=Network::Any);

    bool			checkService(const Network::Service::ID) const;

    CNotifier<ServiceClientMgr,Network::Service*> serviceToBeAdded;
    CNotifier<ServiceClientMgr,Network::Service*> serviceToBeRemoved;
				/*!< Notifies implementations that a service
				  is about to be registered/doregistered */

	/* Implement an action/request send from an external application, that
	   must be handled by od_main. Can be used to transfer signals
	   emitted by the external application, to make od_main aware of it */

    bool		canParseAction(const char*,uiRetVal&) override;
    bool		canParseRequest(const OD::JSON::Object&,
				       uiRetVal&) override;

    uiRetVal		doHandleAction(const char* action) override;
    uiRetVal		doHandleRequest(const OD::JSON::Object&) override;

    const Network::Service* getService(const Network::Service::ID) const;
    Network::Service* getService(const Network::Service::ID);

    void		doAppClosing(CallBacker*) override;
    void		closeApp() override;

    virtual bool	canClaimService(const Network::Service&) const	= 0;
			/*<! When a service is registered, you may want to claim
			     it for your own manager		  */

    uiRetVal		sendAction(const Network::Service&,const char*) const;
    uiRetVal		sendRequest(const Network::Service&,const char*,
				    const OD::JSON::Object& ) const;

    ObjectSet<Network::Service> services_;

private:

    void		init(bool local);

    uiRetVal		addService(const OD::JSON::Object&);
    void		addService(Network::Service&);
    uiRetVal		removeService(const OD::JSON::Object&);
    bool		removeService(const Network::Service::ID);
    void		cleanupServices();
			//<! Remove all unreachable services
};


/*!\
  brief Base class for sending/receiving signals to/from running Batch programs.
  You must likely only need the manager for sending actions/requests to the
  batch programs.
  It also lists the keys that will be send by the batch program to keep us
  informed on its status.
 */

mExpClass(Network) BatchServiceClientMgr : public ServiceClientMgr
{ mODTextTranslationClass(BatchServiceClientMgr)
public:
    virtual			~BatchServiceClientMgr();

    static BatchServiceClientMgr&	getMgr();
    bool			isOK() const;

    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchStarted;
							//<!You may store the ID
    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchEnded;
							//<!Forget this ID

    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchHasStarted;
    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchPaused;
    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchResumed;
    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchKilled;
    CNotifier<BatchServiceClientMgr,Network::Service::ID>	batchFinished;

    static const char*	sKeyStoreInfo()		{ return "batch-info"; }
    static const char*	sKeyDoWork()		{ return "batch-dowork"; }
    static const char*	sKeyPaused()		{ return "batch-paused"; }
    static const char*	sKeyResumed()		{ return "batch-resumed"; }
    static const char*	sKeyKilled()		{ return "batch-killed"; }
    static const char*	sKeyFinished()		{ return "batch-finished"; }

    static const char*	sKeyBatchRequest()	{ return "batch-request"; }

private:
			BatchServiceClientMgr();
    bool		canParseAction(const char*,uiRetVal&) override;
    bool		canParseRequest(const OD::JSON::Object&,
				       uiRetVal&) override;

    uiRetVal		doHandleAction(const char* action) override;
    uiRetVal		doHandleRequest(const OD::JSON::Object&) override;

    bool		canClaimService(const Network::Service&) const override;

    void		batchServiceToBeAdded(CallBacker*);
    void		batchServiceAdded(CallBacker*);
    void		batchServiceRemoved(CallBacker*);
    void		doAppClosing(CallBacker*) override;

    friend class uiServiceClientMgr;
};
