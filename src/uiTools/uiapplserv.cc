/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiapplserv.cc,v 1.1 2012-08-30 11:39:33 cvskris Exp $";

#include "uiapplserv.h"


uiApplService::uiApplService( const char* nm )
    : NamedObject(nm)				{}


uiApplPartServer::uiApplPartServer( uiApplService& a )
    : uias_(a)		{}


uiApplService& uiApplPartServer::appserv()
{ return uias_; }


const uiApplService& uiApplPartServer::appserv() const
{ return uias_; }


uiParent* uiApplPartServer::parent() const
{ return uias_.parent(); }


bool uiApplPartServer::sendEvent( int evid ) const
{ return const_cast<uiApplService&>(appserv()).eventOccurred(this,evid); }


void* uiApplPartServer::getObject( int objid ) const
{ return const_cast<uiApplService&>(appserv()).getObject(this,objid); }
