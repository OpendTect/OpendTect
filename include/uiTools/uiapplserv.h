#ifndef uiapplserv_h
#define uiapplserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Feb 2002
 RCS:		$Id: uiapplserv.h,v 1.15 2012-08-30 11:39:32 cvskris Exp $
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "namedobj.h"
class uiParent;
class uiApplPartServer;


/*! \brief Services from application level to 'Part servers' */

mClass(uiTools) uiApplService : public NamedObject
{
public:
			uiApplService( const char* nm = 0 );
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

mClass(uiTools) uiApplPartServer : public CallBacker
{
public:

    			uiApplPartServer( uiApplService& a );
    virtual const char*	name() const		= 0;

    uiApplService&	appserv();
    const uiApplService& appserv() const;

protected:

    uiParent*		parent() const;

    bool		sendEvent( int evid ) const;
    void*		getObject( int objid ) const;

private:

    uiApplService&	uias_;

};


#endif

