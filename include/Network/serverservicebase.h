#pragma once
/*+
* ________________________________________________________________________
*
* (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
* Author:	A. Huck
* Date:		Oct 2020
* RCS:		$Id$
* ________________________________________________________________________
*
* -*/

#include "servicemgrbase.h"

class Timer;

/*!\
  brief Base class for OpendTect external services/apps launched from
  another 'main' OpendTect application
 */

mExpClass(Network) ServiceServerMgr : public ServiceMgrBase
{ mODTextTranslationClass(ServiceServerMgr)
public:

    enum RegState	{ Unregistered, Requested, Confirmed, Unrequested };

    virtual		~ServiceServerMgr();

    bool		isStandAloneApp() const;
    bool		canReceiveRequests() const;
			/* Only a registered server can expect to receive
			   incoming requests			   */

	/* Use sendAction/sendRequest to ask another listening application
	   to do something on your behalf. By default action/requests are
	   send to the reporting application with the current service name.
	   You should always get a reponse to see if the message reached
	   its target application, and has been processed successfully.
	 */

    virtual uiRetVal	sendAction(const char* act,
				   const Network::Authority* =nullptr,
				   const char* servicenm=nullptr) const;
			/*<! This action will have no information on from
			     the sender. Ensure it can be applied without. */
    virtual uiRetVal	sendActionRequest(const char* reqkey,const char* act,
				   const OD::JSON::Object* morereq=nullptr,
				   const Network::Authority* =nullptr,
				   const char* servicenm=nullptr) const;
			/*<! This will send an action-based request, as
			     the sender's service ID will be added.   */
    virtual uiRetVal	sendRequest(const char* reqkey,const OD::JSON::Object&,
				    const Network::Authority* =nullptr,
				    const char* servicenm=nullptr) const;

protected:
				ServiceServerMgr(Network::Service::ServType,
					      const char* servicenm);
				/*<! Instantiates an application that will
				  have a local listening service as primary
				  server				 */
				ServiceServerMgr(Network::Service::ServType,
					      const char* servicenm,
					      bool assignport,
					      Network::SpecAddr=Network::Any);
				/*<! Instantiate an application that will
				  have a TCP/IP listening service as primary
				  server				 */

		/* Implement an action/request send from an external application, that
		must be handled by od_main. Can be used to transfer signals
		emitted by the external application, to make od_main aware of it */

	bool		canParseAction(const char*,uiRetVal&) override;
	bool		canParseRequest(const OD::JSON::Object&,uiRetVal&) override;

	uiRetVal		doHandleAction(const char* action) override;
	uiRetVal		doHandleRequest(const OD::JSON::Object&) override;

    bool		isRegistered() const;

    bool		isDependentApp() const;
			/*<! returns false when the app is launched as
			  a stand-alone application.
			  Then it never communicates back.		  */

    virtual bool	reportingAppIsAlive() const;
			/*<! If the app was launched from another app,
			    check if this later is still alive	  */

    virtual void	checkOnReportToApplication(bool startyn,
						   int eachms=5000);

    void		doAppClosing(CallBacker*) override;

    const Network::Service*	thisService() const	{ return thisservice_; }

private:

    void		init(Network::Service::ServType,bool local);

    uiRetVal		doRegister();
			//<! On startup, report to the main App (if any)
    uiRetVal		doDeRegister();
			//<! On application closing, inform the main App
    uiRetVal		doRegister_(const char*,bool doreg);

    void		doPyEnvChange(CallBacker*) override;
    virtual void	reportToCheckCB(CallBacker*);

    Network::Service*	thisservice_ = nullptr;
    Network::Authority* reportto_ = nullptr;
    RegState		registerstatus_ = Unregistered;

    Timer*		reporttocheck_ = nullptr;

};
