#ifndef uiapplserv_h
#define uiapplserv_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		Feb 2002
 RCS:		$Id: uiapplserv.h,v 1.1 2002-02-08 22:00:56 bert Exp $
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
			: UserIDObject(nm)			{}
			//!< The name is the application name

    virtual uiParent*	parent() const				= 0;
    virtual void	eventOccurred(uiApplPartServer*,int)	= 0;

};


/*! \brief Makes available certain services that are needed on a higher level.

The idea is that subclasses make available certain services that may be
interesting in an application environment. In such situations, the server may
need feed-back from the application, which can be requested through the
eventOccurred interface. The idea is that the application then - knowing
which of its part servers is calling - proceeds with the right action.
 
*/

class uiApplPartServer
{
public:

    			uiApplPartServer( uiApplService& a )
			: uias_(a)		{}
    virtual const char*	name()			= 0;

    uiApplService&	appserv()		{ return uias_; }
    const uiApplService& appserv() const	{ return uias_; }

    void		sendEvent( int ev )
    			{ appserv().eventOccurred(this,ev); }

protected:

    uiApplService&	uias_;

};


#endif
