/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiapplserv.h"

namespace OD
{

ObjectSet<uiApplMgr>& applmgrs()
{
    static ObjectSet<uiApplMgr> mgrs;
    return mgrs;
}

} // namespace OD

// uiApplMgr

uiApplMgr::uiApplMgr( uiMainWin& uimw, uiApplService& applserv )
    : applservice_(applserv)
{
    OD::applmgrs().add( this );
}


uiApplMgr::~uiApplMgr()
{
    OD::applmgrs() -= this;
    delete &applservice_;
}


uiApplMgr* uiApplMgr::instance( const char* servicenm )
{
    ObjectSet<uiApplMgr>& mgrs = OD::applmgrs();
    if ( !StringView(servicenm).isEmpty() )
    {
	for ( auto* mgr : mgrs )
	{
	    if ( mgr->applService().name() == servicenm )
		return mgr;
	}
    }

    return mgrs.isEmpty() ? nullptr : mgrs.first();
}


// uiApplService

uiApplService::uiApplService( uiParent* p, uiApplMgr& applmgr, const char* nm )
    : NamedObject(nm)
    , par_(p)
    , applman_(applmgr)
{}


uiApplService::~uiApplService()
{}


uiParent* uiApplService::parent() const
{
    return par_;
}


bool uiApplService::eventOccurred( const uiApplPartServer* ps, int evid )
{
    return applman_.handleEvent( ps, evid );
}


void* uiApplService::getObject( const uiApplPartServer* ps, int evid )
{
    return applman_.deliverObject( ps, evid );
}


// uiApplPartServer

uiApplPartServer::uiApplPartServer( uiApplService& a )
    : uias_(a)
{}


uiApplPartServer::~uiApplPartServer()
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
{ return getNonConst( appserv() ).eventOccurred(this,evid); }


void* uiApplPartServer::getObject( int objid ) const
{ return getNonConst( appserv() ).getObject(this,objid); }
