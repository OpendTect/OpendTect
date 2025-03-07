/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiapplserv.h"

#include "ioman.h"
#include "uihostiddlg.h"
#include "uiproxydlg.h"
#include "uisettings.h"

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
    mAttachCB( IOM().prepareSurveyChange, uiApplMgr::prepareSurveyChange );
    mAttachCB( IOM().surveyToBeChanged, uiApplMgr::surveyToBeChanged );
    mAttachCB( IOM().surveyChanged, uiApplMgr::surveyChanged );
}


uiApplMgr::~uiApplMgr()
{
    detachAllNotifiers();
    OD::applmgrs() -= this;
    delete infodlg_;
    delete proxydlg_;
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

const uiPickPartServer* uiApplMgr::pickServer() const
{
    return getNonConst(*this).pickServer();
}


const uiVisPartServer* uiApplMgr::visServer() const
{
    return getNonConst(*this).visServer();
}


const uiSeisPartServer* uiApplMgr::seisServer() const
{
    return getNonConst(*this).seisServer();
}


const uiAttribPartServer* uiApplMgr::attrServer() const
{
    return getNonConst(*this).attrServer();
}


const uiVolProcPartServer* uiApplMgr::volprocServer() const
{
    return getNonConst(*this).volprocServer();
}


const uiEMPartServer* uiApplMgr::EMServer() const
{
    return getNonConst(*this).EMServer();
}


const uiEMAttribPartServer* uiApplMgr::EMAttribServer() const
{
    return getNonConst(*this).EMAttribServer();
}


const uiWellPartServer* uiApplMgr::wellServer() const
{
    return getNonConst(*this).wellServer();
}


const uiWellAttribPartServer* uiApplMgr::wellAttribServer() const
{
    return getNonConst(*this).wellAttribServer();
}


const uiMPEPartServer* uiApplMgr::mpeServer() const
{
    return getNonConst(*this).mpeServer();
}


const uiNLAPartServer* uiApplMgr::nlaServer() const
{
    return getNonConst(*this).nlaServer();
}


void uiApplMgr::prepareSurveyChange(CallBacker*)
{
    prepSurveyChange();
}


void uiApplMgr::surveyToBeChanged(CallBacker*)
{
    survToBeChanged();
}


void uiApplMgr::surveyChanged( CallBacker* )
{
    survChanged();
}


void uiApplMgr::prepSurveyChange()
{
}


void uiApplMgr::survToBeChanged()
{
    closeAndNullPtr( infodlg_ );
    closeAndNullPtr( proxydlg_ );
}


void uiApplMgr::survChanged()
{
}


void uiApplMgr::showInformation( uiParent* p )
{
    if ( infodlg_ && p != infodlg_->parent() )
	closeAndNullPtr( infodlg_ );

    if ( !infodlg_ )
    {
	infodlg_ = new uiInformationDlg( p );
	infodlg_->setModal( false );
    }

    infodlg_->show();
}


void uiApplMgr::showProxy( uiParent* p )
{
    if ( proxydlg_ && p != proxydlg_->parent() )
	closeAndNullPtr( proxydlg_ );

    if ( !proxydlg_ )
    {
	proxydlg_ = new uiProxyDlg( p );
	proxydlg_->setModal( false );
    }

    proxydlg_->show();
}


void uiApplMgr::showSettings( uiParent* p )
{
    uiSettingsDlg dlg( p );
    dlg.go();
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
