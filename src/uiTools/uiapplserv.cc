/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Jul 2006
________________________________________________________________________

-*/

#include "uiapplserv.h"


uiApplService::uiApplService( const char* nm )
    : NamedObject(nm)
{}


uiApplPartServer::uiApplPartServer( uiApplService& a )
    : uias_(a)
    , parent_(nullptr)
{}


uiApplService& uiApplPartServer::appserv()
{ return uias_; }


const uiApplService& uiApplPartServer::appserv() const
{ return uias_; }


void uiApplPartServer::setParent( uiParent* p )
{ parent_ = p; }


uiParent* uiApplPartServer::parent() const
{ return parent_ ? parent_ : uias_.parent(); }


bool uiApplPartServer::sendEvent( int evid ) const
{ return const_cast<uiApplService&>(appserv()).eventOccurred(this,evid); }


void* uiApplPartServer::getObject( int objid ) const
{ return const_cast<uiApplService&>(appserv()).getObject(this,objid); }
