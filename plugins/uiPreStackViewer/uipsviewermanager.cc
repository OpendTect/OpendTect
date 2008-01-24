/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Yuancheng Liu
 Date:		5-11-2007
 RCS:		$Id: uipsviewermanager.cc,v 1.9 2008-01-24 16:00:07 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uipsviewermanager.h"

#include "bufstringset.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackgather.h"
#include "seispsread.h"
#include "seispsioprov.h"
#include "survinfo.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodmain.h"
#include "uipsviewersetting.h"
#include "uiseispartserv.h"
#include "uivispartserv.h"
#include "visplanedatadisplay.h"
#include "visprestackviewer.h"


namespace PreStackView
{

uiPSViewerMgr::uiPSViewerMgr()
    : selectpsdatamenuitem_( "Display PS Gather" )
    , proptymenuitem_( "Properties" )				 
    , removemenuitem_( "Remove" ) 
    , viewermenuitem_( "View in 2D Panel" )
    , visserv_( ODMainWin()->applMgr().visServer() )  	      
{
    visserv_->removeAllNotifier().notify( mCB(this,uiPSViewerMgr,removeAllCB) );
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler();

    IOM().surveyToBeChanged.notify(mCB(this,uiPSViewerMgr,surveyToBeChangedCB));
    ODMainWin()->sessionSave.notify( mCB(this,uiPSViewerMgr,sessionSaveCB) );
    ODMainWin()->sessionRestore.notify(
	    mCB(this,uiPSViewerMgr,sessionRestoreCB) );
       
    menuhandler->createnotifier.notify( mCB(this,uiPSViewerMgr,createMenuCB) );
    menuhandler->handlenotifier.notify( mCB(this,uiPSViewerMgr,handleMenuCB) );
}


uiPSViewerMgr::~uiPSViewerMgr()
{
    visserv_->removeAllNotifier().remove( mCB(this,uiPSViewerMgr,removeAllCB) );
    RefMan<MenuHandler> menuhandler = visserv_->getMenuHandler(); 

    IOM().surveyToBeChanged.remove(mCB(this,uiPSViewerMgr,surveyToBeChangedCB));
    ODMainWin()->sessionSave.remove( mCB(this,uiPSViewerMgr,sessionSaveCB) );
    ODMainWin()->sessionRestore.remove( 
	    mCB(this,uiPSViewerMgr,sessionRestoreCB) );
    menuhandler->createnotifier.remove( mCB(this,uiPSViewerMgr,createMenuCB) );
    menuhandler->handlenotifier.remove( mCB(this,uiPSViewerMgr,handleMenuCB) );

    removeAllCB( 0 );
    deepErase( viewwindows_ );
}    


void uiPSViewerMgr::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    
    RefMan<visBase::DataObject> dataobj = visserv_->getObject( menu->menuID() );

    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );
    if ( pdd && pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Timeslice ) 
    {
	uiSeisPartServer* seisserv = ODMainWin()->applMgr().seisServer();
	BufferStringSet gnms; seisserv->getStoredGathersList(true,gnms);
	if ( gnms.size() )
	{
	    selectpsdatamenuitem_.removeItems();
	    selectpsdatamenuitem_.createItems(gnms);
	    mAddMenuItem( menu, &selectpsdatamenuitem_, true, false );
	}
	else
	    mResetMenuItem( &selectpsdatamenuitem_ );
    }
    else
	mResetMenuItem( &selectpsdatamenuitem_ );

    mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
    if ( psv )
    { 
	mAddMenuItem( menu, &proptymenuitem_, true, false );
    	mAddMenuItem( menu, &removemenuitem_, true, false ); 
    	mAddMenuItem( menu, &viewermenuitem_, true, false ); 
    }
    else
    {
	mResetMenuItem( &proptymenuitem_ );
	mResetMenuItem( &removemenuitem_ );
	mResetMenuItem( &viewermenuitem_ );
    }
}


void uiPSViewerMgr::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*,menu,caller );

    if ( menu->isHandled() )
	return;

    int sceneid = -1;
    TypeSet<int> sceneids;
    visserv_->getChildIds( -1, sceneids );

    for ( int idx=0; idx<sceneids.size(); idx++ )
    {
	TypeSet<int> scenechildren;
	visserv_->getChildIds( sceneids[idx], scenechildren );
	if ( scenechildren.indexOf( menu->menuID() ) )
	{
	    sceneid = sceneids[idx];
	    break;
	}
    }

    if ( sceneid==-1 )
	return;

    const int mnuidx = selectpsdatamenuitem_.itemIndex( mnuid );
    if ( mnuidx>=0 )
    {
	PtrMan<IOObj> ioobj = IOM().getLocal(
		selectpsdatamenuitem_.getItem(mnuidx)->text );
	if ( !ioobj )
	    return;

	menu->setIsHandled( true );

	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, dataobj.ptr() );

	PreStackViewer* viewer = PreStackViewer::create();
	viewer->ref();
	viewer->setMultiID( ioobj->key() );
	viewer->setSectionDisplay( pdd ); 
	
	visserv_->addObject( viewer, sceneid, false );
	viewers_ += viewer;
	
	BinID bid;
	if (  menu->getMenuType()==uiMenuHandler::fromScene )
	{
	    const Coord3 pickedpos = menu->getPickedPos();
    	    if ( !pickedpos.isDefined() )
    		return;

	    bid = SI().transform( pickedpos );
	}
	else 
	{
	    if ( !pdd ) return;
	    
	    CubeSampling cs = pdd->getCubeSampling();
  	    cs.snapToSurvey();
  	    bid = BinID( (cs.hrg.stop.inl + cs.hrg.start.inl + 1)/2,
		         (cs.hrg.stop.crl + cs.hrg.start.crl + 1)/2 );
	}

	if ( !viewer->setPosition( bid ) )
	    return;
	
	if ( viewer->getScene() )
	    viewer->getScene()->change.notifyIfNotNotified( mCB( this, 
			uiPSViewerMgr,sceneChangeCB ) );
    }
    else if ( mnuid==removemenuitem_.id )
    {
 	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
	if ( !psv ) return;

	menu->setIsHandled( true );
	
	visserv_->removeObject( psv, sceneid );
	viewers_ -= psv;
	psv->unRef();
    }
    else if ( mnuid==proptymenuitem_.id )
    {
	menu->setIsHandled( true );
	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
	if ( !psv ) return;

	uiPSViewerSetting dlg( menu->getParent(), *psv, *this );
	dlg.go(); 
	if ( !dlg.acceptOK() )
	    return;
    }
    else if ( mnuid==viewermenuitem_.id )
    {
	menu->setIsHandled( true );
 	RefMan<visBase::DataObject> dataobj = visserv_->
	    getObject( menu->menuID() );
	mDynamicCastGet( PreStackView::PreStackViewer*, psv, dataobj.ptr() );
	if ( !psv ) return;

	uiFlatViewWin* viewwin = create2DViewer( psv->getMultiID(),
						 psv->getBinID() );
	if ( viewwin )
	{
    	    viewwindows_ += viewwin;
    	    viewwin->start();
	}
    }
}


#define mErrRes(msg) { uiMSG().error(msg); return 0; }


uiFlatViewWin* uiPSViewerMgr::create2DViewer( const MultiID& mid, 
					      const BinID& bid )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) 
	return 0;

    BufferString title( "Gather from [" );
    title += ioobj->name();
    title += "] at "; 
    title += bid.inl; 
    title += "/"; 
    title += bid.crl;
    uiFlatViewWin* viewwin = new uiFlatViewMainWin( 
	    ODMainWin()->applMgr().seisServer()->appserv().parent(), 
	    uiFlatViewMainWin::Setup(title) );
    
    viewwin->setWinTitle( title );
    viewwin->setDarkBG( false );
    
    uiFlatViewer& vwr = viewwin->viewer();
    vwr.appearance().annot_.setAxesAnnot( true );
    vwr.appearance().setGeoDefaults( true );
    vwr.appearance().ddpars_.show( true, false );
    vwr.appearance().ddpars_.wva_.overlap_ = 1;

    const bool haddata = vwr.pack( false );
    PreStack::Gather* gather = new PreStack::Gather;
    if ( !gather->readFrom( mid, bid ) )
    {
	delete gather;
	if ( haddata )
	    vwr.setPack( false, DataPack::cNoID, false );
	else
	{
	    uiMSG().error("There is no data available");
	    return 0;
	}
    }
    else
    {
	DPM(DataPackMgr::FlatID).add( gather );
	vwr.setPack( false, gather->id(), false, !haddata );
    }
    
    PtrMan<SeisPSReader> rdr = SPSIOPF().get3DReader( *ioobj, bid.inl );
    if ( !rdr )
	mErrRes( "This Pre-Stack data store cannot be handled" )

    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    if ( !rdr->getGather(bid,*tbuf) )
        mErrRes( rdr->errMsg() )
    
    const int tbufsz = tbuf->size();
    if ( tbufsz == 0 )
        mErrRes( "Gather is empty" )

    int pw = 200 + 10 * tbuf->size();
    if ( pw < 400 ) pw = 400; if ( pw > 800 ) pw = 800;
    vwr.setInitialSize( uiSize(pw,500) );
    viewwin->addControl( new uiFlatViewStdControl( vwr,
			 uiFlatViewStdControl::Setup().withstates(false) ) );
    return viewwin;
}


void uiPSViewerMgr::sceneChangeCB( CallBacker* )
{ 
    for ( int idx = 0; idx<viewers_.size(); idx++ )
    {
	PreStackView::PreStackViewer* psv = viewers_[idx];

	visBase::Scene* scene = psv->getScene();
	if ( !scene || scene->getFirstIdx( psv->getSectionDisplay() )==-1 )
	{
	    viewers_.remove( idx );
	    if ( scene ) visserv_->removeObject( psv, scene->id() );
	    psv->unRef();
	    idx--;
	}
    }
}


void uiPSViewerMgr::sessionRestoreCB( CallBacker* )
{
    deepErase( viewwindows_ );

    TypeSet<int> vispsviewids;
    visserv_->findObject( typeid(PreStackView::PreStackViewer), vispsviewids );

    for ( int idx=0; idx<vispsviewids.size(); idx++ )
    {
	mDynamicCastGet( PreStackView::PreStackViewer*, psv,
			 visserv_->getObject(vispsviewids[idx]) );
	viewers_ += psv;
	psv->ref();
    }
    
    PtrMan<IOPar> allwindowspar = ODMainWin()->sessionPars().subselect(
	    			  sKey2DViewers() );
    int nrwindows;
    if ( !allwindowspar || !allwindowspar->get(sKeyNrWindows(), nrwindows) )
	return;

    for ( int idx=0; idx<nrwindows; idx++ )
    {
	BufferString key = sKeyViewerPrefix();
	key += idx;
	PtrMan<IOPar> viewerpar = allwindowspar->subselect( key.buf() );
	if ( !viewerpar )
	    continue;

	MultiID mid;
	BinID bid;
	if ( !viewerpar->get( sKeyMultiID(), mid ) ||
	     !viewerpar->get( sKeyBinID(), bid ) )
	    continue;

	uiFlatViewWin* viewwin = create2DViewer( mid, bid );

	if ( !viewwin )
	    continue;

	viewwindows_ += viewwin;
	viewwin->start();
    }
}


void uiPSViewerMgr::sessionSaveCB( CallBacker* ) 
{
    IOPar allwindowpar;
    int nrsaved = 0;
    for ( int idx=0; idx<viewwindows_.size(); idx++ )
    {
	const FlatDataPack* dp = viewwindows_[idx]->viewer().pack( false );
	mDynamicCastGet( const PreStack::Gather*, gather, dp );
	if ( !gather )
	    continue;

	IOPar viewerpar;
	viewwindows_[idx]->viewer().fillPar( viewerpar );
	viewerpar.set( sKeyMultiID(), gather->getStorageID() );
	viewerpar.set( sKeyBinID(), gather->getBinID() );

	BufferString key = sKeyViewerPrefix();
	key += nrsaved;
	nrsaved++;

	allwindowpar.mergeComp( viewerpar, key );
    }

    allwindowpar.set( sKeyNrWindows(), nrsaved );
    ODMainWin()->sessionPars().mergeComp( allwindowpar, sKey2DViewers() );
}


void  uiPSViewerMgr::removeAllCB( CallBacker* )
{
    deepUnRef( viewers_ );
}    


void uiPSViewerMgr::surveyToBeChangedCB( CallBacker* )
{
    deepErase( viewwindows_ );
}

}; // Namespace
