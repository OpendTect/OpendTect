#ifndef uiapplserv_h
#define uiapplserv_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Feb 2002
 RCS:		$Id: uiapplserv.h,v 1.8 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/


#include "uidobj.h"
class uiParent;
class uiApplPartServer;


/*! \brief Services from application level to 'Part servers' */

class uiApplService : public UserIDObject
{
public:
			uiApplService( const char* nm = 0 )
			: UserIDObject(nm)				{}
			//!< The name is the application name

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

class uiApplPartServer : public CallBacker
{
public:

    			uiApplPartServer( uiApplService& a )
			: uias_(a)		{}
    virtual const char*	name() const		= 0;

    uiApplService&	appserv()		{ return uias_; }
    const uiApplService& appserv() const	{ return uias_; }

protected:

    bool		sendEvent( int evid ) const
    			{ return const_cast<uiApplService&>(appserv())
				    .eventOccurred(this,evid); }
    void*		getObject( int objid ) const
			{ return const_cast<uiApplService&>(appserv())
				    .getObject(this,objid); }

private:

    uiApplService&	uias_;

};


#endif
