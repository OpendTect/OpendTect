#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "namedobj.h"
class uiParent;
class uiApplPartServer;


/*! \brief Services from application level to 'Part servers' */

mExpClass(uiTools) uiApplService : public NamedObject
{
public:
			uiApplService( const char* nm = 0 );
			//!< The name is the application name
			~uiApplService();

    virtual uiParent*	parent() const					= 0;
    virtual bool	eventOccurred(const uiApplPartServer*,int evid)	= 0;
			//!< The evid will be specific for each partserver
    virtual void*	getObject(const uiApplPartServer*,int)		= 0;
			//!< The actual type is a protocol with the partserver
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

    bool		sendEvent( int evid ) const;
    void*		getObject( int objid ) const;

private:

    uiApplService&	uias_;
    uiParent*		parent_;

};
