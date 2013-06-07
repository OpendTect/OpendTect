/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiodbodydisplaytreeitem.h"

#include "arrayndimpl.h"
#include "empolygonbody.h"
#include "emmarchingcubessurface.h"
#include "emmanager.h"
#include "emrandomposbody.h"
#include "ioman.h"
#include "ioobj.h"
#include "marchingcubes.h"
#include "mousecursor.h"
#include "randcolor.h"

#include "uiempartserv.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "vismarchingcubessurface.h"
#include "vismarchingcubessurfacedisplay.h"
#include "vispolygonbodydisplay.h"
#include "visrandomposbodydisplay.h"


/*test*/
#include "cubesampling.h"
#include "ranges.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "houghtransform.h"
#include "iodir.h"
#include "embodytr.h"
#include "emfault3d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emmarchingcubessurface.h"
#include "emsurfacetr.h"
#include "explfaultsticksurface.h"
#include "explplaneintersection.h"
#include "executor.h"
#include "survinfo.h"



uiODBodyDisplayParentTreeItem::uiODBodyDisplayParentTreeItem()
   : uiODTreeItem( "Body" )
{}


bool uiODBodyDisplayParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().message( "Cannot add Bodies to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("&Add ..."), 0 );
    mnu.insertItem( new uiMenuItem("&New polygon body..."), 1 );
    //mnu.insertItem( new uiMenuItem("&Auto tracking Fault body..."), 2 );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid==0 )
    {
	ObjectSet<EM::EMObject> objs;
	applMgr()->EMServer()->selectBodies( objs );
	MouseCursorChanger uics( MouseCursor::Wait );
	for ( int idx=0; idx<objs.size(); idx++ )
	    addChild( new uiODBodyDisplayTreeItem(objs[idx]->id()), false );
	deepUnRef( objs );
    }
    else if ( mnuid==1 )
    {
	RefMan<EM::EMObject> emplg =
	    EM::EMM().createTempObject( EM::PolygonBody::typeStr() );
	if ( !emplg )
	    return false;

	emplg->setPreferredColor( getRandomColor(false) );
	emplg->setNewName();
	emplg->setFullyLoaded( true );
	addChild( new uiODBodyDisplayTreeItem( emplg->id() ), false );
	
	uiVisPartServer* visserv = applMgr()->visServer();
	visserv->showMPEToolbar();
	visserv->turnSeedPickingOn( false );
    }
    else if ( mnuid==2 )//To delete
    {
	bool usebody = 1;
	CubeSampling cs(true);
	    cs.hrg.step = BinID(2,2);
    	    cs.zrg.set( 1.432, 2.692, 0.004 );
	if ( !usebody) 
	    cs.hrg.set( Interval<int>(1456,1456), Interval<int>(2360,2870) );
	else
	    cs.hrg.set( Interval<int>(1022,1682), Interval<int>(2056,2870) );

	IOM().to( MultiID("100010") );
	const IOObj* ioobj = usebody ? IOM().getLocal("Semblance") :
	    IOM().getLocal("similarity-inl1456");
	    //IOM().getLocal("ridge-inl1456");
	if ( !ioobj ) return false;

	mDeclareAndTryAlloc(Array3DImpl<float>*,arr,Array3DImpl<float>(
		    cs.nrInl(), cs.nrCrl(), cs.nrZ() ));
	if ( !arr ) return false;
	arr->setAll( 1 );

	SeisTrcReader reader(ioobj);
	TypeSet<DataPack::ID> dpids;
	ObjectSet<SeisTrcBuf> tbs;
	for ( int iscrl=0; iscrl<2; iscrl++ ){
	int fisz = iscrl ? cs.nrCrl() : cs.nrInl();
	for ( int iidx=0; iidx<fisz; iidx++ )
	{
	    CubeSampling  inlcs = cs;
	    if ( usebody )
	    {
		if ( iscrl )
		{
		    const int crl = cs.hrg.crlRange().atIndex(iidx);
		    inlcs.hrg.setCrlRange(Interval<int>(crl,crl));
		}
		else 
		{
		    const int inl = cs.hrg.inlRange().atIndex(iidx);
		    inlcs.hrg.setInlRange(Interval<int>(inl,inl));
		}
	    }

	    reader.setSelData( new Seis::RangeSelData(inlcs) );
	    reader.prepareWork();

	    SeisTrcBuf* trcbuf = new SeisTrcBuf( true );
	    tbs += trcbuf;
	    while ( true )
	    {
		SeisTrc* trc = new SeisTrc;
		const int res = reader.get( trc->info() );
		if ( res == -1 ) { delete trc; return false; }
		else if ( res == 2 )  { delete trc; continue; }
		else if ( reader.get(*trc) )
		    trcbuf->add( trc );
		else delete trc;
		
		if ( res == 0 ) break;
	    }

	    SeisTrcBufDataPack dp(trcbuf,Seis::Vol,
		    !iscrl ? SeisTrcInfo::BinIDInl : SeisTrcInfo::BinIDCrl,
		    "Seismics");
	    DPM( DataPackMgr::FlatID() ).addAndObtain( &dp );
	    dpids += dp.id();

	    const int nrows = dp.data().info().getSize(0);
	    const int ncols = dp.data().info().getSize(1);
	    mDeclareAndTryAlloc(Array2DImpl<float>*, trace,
		    Array2DImpl<float>(nrows,ncols));

	    for ( int idy=0; idy<nrows; idy++ )
	    {
		for ( int idz=0; idz<ncols; idz++ )
		    trace->set(idy,idz,dp.data().get(idy,idz));
	    }

	    LineFrom2DSpaceHoughTransform ht( *trace );
	    ht.setThreshold( 0.8, false );
	    //ht.setLineAngleRange(Interval<float>(0,M_PI_4));
	    ht.setLineAngleRange(Interval<float>(M_PI_4,3*M_PI_4));
	    ht.setTopList(10);
	    ht.compute();
	    delete trace;
	    const Array2D<unsigned char>* flag = ht.getResult();
	    if ( usebody )
	    {
		for ( int idy=0; idy<nrows; idy++ )
		{
		    for ( int idz=0; idz<ncols; idz++ )
		    {
			int val = flag->get(idy,idz);
			if ( !iscrl )
			{
			    if ( val )
    				arr->set(iidx,idy,idz,-1);
			}
			else
			{
			    if ( !val )
				arr->set(idy,iidx,idz,1);
			    else if ( arr->get(idy,iidx,idz<0 ))
    				arr->set(idy,iidx,idz,-1);
			}
		    }
		}
	    }
	    else
	    {
		const Array2D<int>* ha = ht.hougharr_;
		const int tsz = ha->info().getSize(0);
		const int rsz = ha->info().getSize(1);
		const float rf = (float)tsz/(float)nrows;
		const float cf = (float)rsz/(float)ncols;
		const IOObj* output = IOM().getLocal("Fault-inl1456-output");
		const IOObj* ho = IOM().getLocal("Hough-array");
		if ( !output ) return false;
		PtrMan<SeisTrcWriter> writer = new SeisTrcWriter( output );
		PtrMan<SeisTrcWriter> hwriter = new SeisTrcWriter( ho );
		SamplingData<double> sd(cs.zrg.start,cs.zrg.step);
		BinID curbid = cs.hrg.start;
		for ( int ridx=0; ridx<nrows; ridx++ )
		{
		    curbid.crl = cs.hrg.crlRange().atIndex(ridx);

		    SeisTrc outtrc(cs.nrZ()), hot(cs.nrZ());
		    hot.info().sampling = outtrc.info().sampling = sd;
		    hot.info().binid = outtrc.info().binid = curbid;
		    hot.info().coord=outtrc.info().coord=SI().transform(curbid);
		    for ( int cidx=0; cidx<ncols; cidx++ )
		    {
			outtrc.set(cidx, flag->get(ridx,cidx),0);
    			hot.set(cidx, ha->get((int)(ridx*rf),(int)(cidx*cf)),0);
		    }
		    writer->put( outtrc );
		    hwriter->put( hot );
		}

		return true;
	    }
	}}

	RefMan<EM::MarchingCubesSurface> emcs =
	    new EM::MarchingCubesSurface(EM::EMM());
	if ( !emcs ) return false;

	emcs->surface().setVolumeData( 0, 0, 0, *arr, 0, 0 );
	emcs->setInlSampling(
		SamplingData<int>(cs.hrg.start.inl,cs.hrg.step.inl));
	emcs->setCrlSampling(
		SamplingData<int>(cs.hrg.start.crl,cs.hrg.step.crl));
	emcs->setZSampling(SamplingData<float>(cs.zrg.start,cs.zrg.step));

	emcs->setPreferredColor( getRandomColor(false) );
	
	static int autofltidx = 0;
	BufferString nm = "New-fault-body-";
	nm += autofltidx++;
	emcs->setName( nm.buf() );
	emcs->setFullyLoaded( true );
	EM::EMM().addObject( emcs );

	PtrMan<Executor> exec = emcs->saver();
	if ( exec )
	{
	    MultiID key = emcs->multiID();
	    PtrMan<IOObj> eioobj = IOM().get( key );
	    if ( !eioobj->pars().find( sKey::Type ) )
	    {
		eioobj->pars().set( sKey::Type, emcs->getTypeStr() );
		IOM().commitChanges( *eioobj );
	    }
	
	    exec->execute();
	}
	    
	addChild( new uiODBodyDisplayTreeItem(emcs->id()), false );

	//deepErase(tbs);	
    	//for ( int idx=0; idx<dpids.size(); idx++ )
    	  //  DPM(DataPackMgr::FlatID()).release( dpids[idx] );
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODBodyDisplayTreeItemFactory::createForVis( int visid,
							  uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PolygonBodyDisplay*,plg,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( plg )
	return new uiODBodyDisplayTreeItem( visid, true );
    
    mDynamicCastGet(visSurvey::MarchingCubesDisplay*,mcd,
	    ODMainWin()->applMgr().visServer()->getObject(visid));
    if ( mcd )
    	return new uiODBodyDisplayTreeItem( visid, true );

    return 0;
}


#define mCommonInit \
    , savemnuitem_("&Save") \
    , saveasmnuitem_("Save &as ...") \
    , displaybodymnuitem_("&Body") \
    , displaypolygonmnuitem_("&Picked polygons") \
    , displayintersectionmnuitem_("&Only at sections") \
    , singlecolormnuitem_("Use single &color") \
    , mcd_(0) \
    , plg_(0) \
    , rpb_(0)

#define mCommonInit2 \
    displaybodymnuitem_.checkable = true; \
    displaypolygonmnuitem_.checkable = true; \
    displayintersectionmnuitem_.checkable = true; \
    singlecolormnuitem_.checkable = true; \
    savemnuitem_.iconfnm = "save.png"; \
    saveasmnuitem_.iconfnm = "saveas.png";


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( const EM::ObjectID& oid )
    : uiODDisplayTreeItem()
    , emid_( oid )
    mCommonInit
{
    mCommonInit2
}


uiODBodyDisplayTreeItem::uiODBodyDisplayTreeItem( int id, bool dummy )
    : uiODDisplayTreeItem()
    , emid_(-1)
    mCommonInit
{
    displayid_ = id;
    mCommonInit2
}


uiODBodyDisplayTreeItem::~uiODBodyDisplayTreeItem()
{
    if ( mcd_ )
    {
	mcd_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	mcd_->unRef();
    }

    if ( plg_ )
    {
	plg_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	plg_->unRef();
    }

    if ( rpb_ ) 
    {
	rpb_->materialChange()->remove(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	rpb_->unRef();
    }
}


bool uiODBodyDisplayTreeItem::init()
{
    if ( displayid_==-1 )
    {
	EM::EMObject* object = EM::EMM().getObject( emid_ );
	mDynamicCastGet( EM::PolygonBody*, emplg, object );
	mDynamicCastGet( EM::MarchingCubesSurface*, emmcs, object );
	mDynamicCastGet( EM::RandomPosBody*, emrpb, object );
	if ( emplg )
	{
	    visSurvey::PolygonBodyDisplay* plg =
		visSurvey::PolygonBodyDisplay::create();
	    displayid_ = plg->id();
	    plg_ = plg;
	    plg_->ref();
	    plg_->setEMID(emid_);
	    visserv_->addObject( plg_, sceneID(), true );
	}
	else if ( emmcs ) 
	{
	    visSurvey::MarchingCubesDisplay* mcd =
		visSurvey::MarchingCubesDisplay::create();
	    displayid_ = mcd->id();
	    mcd_ = mcd;
	    mcd_->ref();
	    uiTaskRunner taskrunner( getUiParent() );
	    mcd_->setEMID( emid_, &taskrunner );
	    visserv_->addObject( mcd_, sceneID(), true );
	}
	else if ( emrpb )
	{
	    visSurvey::RandomPosBodyDisplay* rpb = 
		visSurvey::RandomPosBodyDisplay::create();
	    displayid_ = rpb->id();
	    rpb_ = rpb;
	    rpb_->ref();
	    rpb_->setEMID( emid_ );
	    visserv_->addObject( rpb_, sceneID(), true );
	}
    }
    else
    {
	mDynamicCastGet( visSurvey::PolygonBodyDisplay*, plg,
			 visserv_->getObject(displayid_) );
	mDynamicCastGet( visSurvey::MarchingCubesDisplay*, mcd,
			 visserv_->getObject(displayid_) );
	mDynamicCastGet( visSurvey::RandomPosBodyDisplay*, rpb, 
			 visserv_->getObject(displayid_) );
	if ( plg )
	{
	    plg_ = plg;
	    plg_->ref();
	    emid_ = plg->getEMID();
	}
	else if ( mcd )
	{
	    mcd_ = mcd;
	    mcd_->ref();
	    emid_ = mcd->getEMID();
	}
	else if ( rpb )
	{
	    rpb_ = rpb;
	    rpb_->ref();
	    emid_ = rpb->getEMID();
	}	
    }

    if ( plg_ )
    {
	plg_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }
    
    if ( mcd_ )
    {
	mcd_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }

    if ( rpb_ )
    {
	rpb_->materialChange()->notify(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
    }
    
    return uiODDisplayTreeItem::init();
}


void uiODBodyDisplayTreeItem::colorChCB( CallBacker* )
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


bool uiODBodyDisplayTreeItem::askContinueAndSaveIfNeeded( bool withcancel )
{
    return applMgr()->EMServer()->askUserToSave( emObjectID(), withcancel );
}


void uiODBodyDisplayTreeItem::prepareForShutdown()
{
    if ( mcd_ )
    {
	mcd_->materialChange()->remove(
	    mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	mcd_->unRef();
    }
    mcd_ = 0;

    if ( plg_ )
    {
	plg_->materialChange()->remove(
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	plg_->unRef();
    }
    plg_ = 0;

    if ( rpb_ ) 
    {
	rpb_->materialChange()->remove( 
		mCB(this,uiODBodyDisplayTreeItem,colorChCB) );
	rpb_->unRef();
    }
    rpb_ = 0;
    
    uiODDisplayTreeItem::prepareForShutdown();
}


void uiODBodyDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() || ( !mcd_ && !plg_ && !rpb_) )
	return;
	
    const bool enablesave = applMgr()->EMServer()->isChanged(emid_) &&
			    applMgr()->EMServer()->isFullyLoaded(emid_);
    if ( mcd_ )
    {
	const bool intersectdisplay = mcd_->areIntersectionsDisplayed();
	mAddMenuItem( menu, &displaymnuitem_, true, true );
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
 		!intersectdisplay );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true, 
		intersectdisplay );
	mAddMenuItem( &displaymnuitem_, &singlecolormnuitem_, true, 
		!mcd_->usesTexture() );
    }

    if ( plg_ )
    {
	mAddMenuItem( &displaymnuitem_, &displaybodymnuitem_, true,
		      plg_->isBodyDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displaypolygonmnuitem_, true,
		      plg_->arePolygonsDisplayed() );
	mAddMenuItem( &displaymnuitem_, &displayintersectionmnuitem_, true,
		      plg_->areIntersectionsDisplayed() );
	mAddMenuItem( menu, &displaymnuitem_, true, true );
    }

    mAddMenuItem( menu, &savemnuitem_, enablesave, false );
    mAddMenuItem( menu, &saveasmnuitem_, true, false );
}


void uiODBodyDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;

    if ( mnuid==saveasmnuitem_.id || mnuid==savemnuitem_.id )
    {
	bool saveas = mnuid==saveasmnuitem_.id ||
	    applMgr()->EMServer()->getStorageID(emid_).isEmpty();
	if ( !saveas )
	{
	    PtrMan<IOObj> ioobj =
		IOM().get( applMgr()->EMServer()->getStorageID(emid_) );
	    saveas = !ioobj;
	}
	
	applMgr()->EMServer()->storeObject( emid_, saveas );
	const bool notempty = !applMgr()->EMServer()->getName(emid_).isEmpty();
	if ( saveas && notempty )
	{
	    if ( plg_ )
		plg_->setName( applMgr()->EMServer()->getName(emid_) );

	    if ( rpb_ )
		rpb_->setName( applMgr()->EMServer()->getName(emid_) );

	    if ( mcd_ )
		mcd_->setName( applMgr()->EMServer()->getName(emid_) );

	    updateColumnText( uiODSceneMgr::cNameColumn() );
	}
    }
    else if ( mnuid==displaybodymnuitem_.id )
    {
	const bool bodydisplay = !displaybodymnuitem_.checked;
	if ( plg_ )
	{
    	    const bool polygondisplayed = displaypolygonmnuitem_.checked;
    	    plg_->display( polygondisplayed, bodydisplay );
    	    plg_->displayIntersections( !polygondisplayed && !bodydisplay );
	}
	else if ( mcd_ )
	    mcd_->displayIntersections( !bodydisplay );
    }
    else if ( mnuid==displaypolygonmnuitem_.id )
    {
	const bool polygondisplay = !displaypolygonmnuitem_.checked;
	const bool bodydisplayed = displaybodymnuitem_.checked;
	plg_->display( polygondisplay, bodydisplayed );
	plg_->displayIntersections( !polygondisplay && !bodydisplayed );
    }
    else if ( mnuid==displayintersectionmnuitem_.id )
    {
	const bool intersectdisplay = !displayintersectionmnuitem_.checked;
	if ( plg_ )
	{
    	    plg_->display( false, !intersectdisplay );
    	    plg_->displayIntersections( intersectdisplay );
	}
	else if ( mcd_ )
	    mcd_->displayIntersections( intersectdisplay );
    }
    else if ( mnuid==singlecolormnuitem_.id )
    {
	mcd_->useTexture( !mcd_->usesTexture() );
    }
    else
	return;
	
    menu->setIsHandled(true);
}
