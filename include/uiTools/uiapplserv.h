#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "namedobj.h"

class uiApplPartServer;
class uiApplService;
class uiDialog;
class uiMainWin;
class uiParent;

class uiEMPartServer;
class uiPickPartServer;
class uiSeisPartServer;
class uiWellPartServer;
class uiVisPartServer;
class uiAttribPartServer;
class uiVolProcPartServer;
class uiEMAttribPartServer;
class uiWellAttribPartServer;
class uiMPEPartServer;
class uiNLAPartServer;


/*! \brief Base class for an application level manager */

mExpClass(uiTools) uiApplMgr : public CallBacker
{
public:
			~uiApplMgr();

    virtual bool	handleEvent(const uiApplPartServer*,int evid)	= 0;
    virtual void*	deliverObject(const uiApplPartServer*,int evid) = 0;

    uiApplService&	applService()	{ return applservice_; }

    static uiApplMgr*	instance(const char* servicenm=nullptr);

    virtual uiEMPartServer*		EMServer()			= 0;
    virtual uiPickPartServer*		pickServer()			= 0;
    virtual uiSeisPartServer*		seisServer()			= 0;
    virtual uiWellPartServer*		wellServer()			= 0;

    virtual uiVisPartServer*		visServer()	    { return nullptr; }
    virtual uiAttribPartServer*		attrServer()	    { return nullptr; }
    virtual uiVolProcPartServer*	volprocServer()     { return nullptr; }
    virtual uiEMAttribPartServer*	EMAttribServer()    { return nullptr; }
    virtual uiWellAttribPartServer*	wellAttribServer()  { return nullptr; }
    virtual uiMPEPartServer*		mpeServer()	    { return nullptr; }
    virtual uiNLAPartServer*		nlaServer()	    { return nullptr; }

    const uiPickPartServer*		pickServer() const;
    const uiVisPartServer*		visServer() const;
    const uiSeisPartServer*		seisServer() const;
    const uiAttribPartServer*		attrServer() const;
    const uiVolProcPartServer*		volprocServer() const;
    const uiEMPartServer*		EMServer() const;
    const uiEMAttribPartServer*		EMAttribServer() const;
    const uiWellPartServer*		wellServer() const;
    const uiWellAttribPartServer*	wellAttribServer() const;
    const uiMPEPartServer*		mpeServer() const;
    const uiNLAPartServer*		nlaServer() const;

    virtual void			setNlaServer(uiNLAPartServer*)	{}

    void				showInformation(uiParent*);
    void				showProxy(uiParent*);
    void				showSettings(uiParent*);

protected:
			uiApplMgr(uiMainWin&,uiApplService&);
			//!< uiApplService object becomes mine

    void		prepareSurveyChange(CallBacker*);
    void		surveyToBeChanged(CallBacker*);
    void		surveyChanged(CallBacker*);

    virtual void	prepSurveyChange();
    virtual void	survToBeChanged();
    virtual void	survChanged();

    uiApplService&	applservice_;

    uiDialog*		infodlg_    = nullptr;
    uiDialog*		proxydlg_   = nullptr;
};


/*! \brief Services from application level to 'Part servers' */

mExpClass(uiTools) uiApplService : public NamedObject
{
public:
			uiApplService(uiParent*,uiApplMgr&,
				      const char* nm=nullptr);
			//!< The name is the application name
			~uiApplService();

    virtual uiParent*	parent() const;
    virtual bool	eventOccurred(const uiApplPartServer*,int evid);
			//!< The evid will be specific for each partserver
    virtual void*	getObject(const uiApplPartServer*,int);
			//!< The actual type is a protocol with the partserver

protected:
    uiParent*		par_;
    uiApplMgr&		applman_;
};


/*! \brief Makes available certain services that are needed on a higher level.

The idea is that subclasses make available certain services that may be
interesting in an application environment. In such situations, the server may
need feed-back from the application, which can be requested through the
eventOccurred interface. The idea is that the application then - knowing
which of its part servers is calling - proceeds with the right action.

*/

mExpClass(uiTools) uiApplPartServer : public CallBacker
{
public:
			~uiApplPartServer();

    virtual const char*	name() const		= 0;

    uiApplService&	appserv();
    const uiApplService& appserv() const;

    void		setParent(uiParent*);

protected:
			uiApplPartServer(uiApplService&);

    uiParent*		parent() const;

    bool		sendEvent(int evid) const;
    void*		getObject(int objid) const;

private:

    uiApplService&	uias_;
    uiParent*		parent_ = nullptr;

};
