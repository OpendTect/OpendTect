/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id: uiodscenemgr.cc,v 1.2 2003-12-24 15:15:50 bert Exp $
________________________________________________________________________

-*/

#include "uiodscenemgr.h"
#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "scene.xpm"

static const int cWSWidth = 600;
static const int cWSHeight = 300;
static const int cMinZoom = 1;
static const int cMaxZoom = 150;

#define mWSMCB(fn) mCB(this,uiODSceneMgr,fn)
#define mDoAllScenes(memb,fn,arg) \
    for ( int idx=0; idx<scenes.size(); idx++ ) \
	scenes[idx]->memb->fn( arg )



uiODSceneMgr::uiODSceneMgr( uiODMain* a )
    	: appl(*a)
    	, wsp(new uiWorkSpace( this, "OpendTect work space" ))
	, vwridx(0)
    	, lasthrot(0), lastvrot(0), lastdval(0)
    	, visserv(a->applMgr().visServer())
    	, tifs(new uiTreeFactorySet)
{
    wsp->setPrefWidth( cWSWidth );
    wsp->setPrefHeight( cWSHeight );

    uiGroup* leftgrp = new uiGroup( &appl, "Left group" );

    uiThumbWheel* dollywheel = new uiThumbWheel( leftgrp, "Dolly", false );
    dollywheel->wheelMoved.notify( mWSMCB(dWheelMoved) );
    dollywheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    dollywheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    uiLabel* dollylbl = new uiLabel( leftgrp, "MOV" );
    dollylbl->attach( centeredBelow, dollywheel );

    uiLabel* dummylbl = new uiLabel( leftgrp, "" );
    dummylbl->attach( centeredBelow, dollylbl );
    dummylbl->setStretch( 0, 2 );
    uiThumbWheel* vwheel = new uiThumbWheel( leftgrp, "vRotate", false );
    vwheel->wheelMoved.notify( mWSMCB(vWheelMoved) );
    vwheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    vwheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    vwheel->attach( centeredBelow, dummylbl );

    uiLabel* rotlbl = new uiLabel( this, "ROT" );
    rotlbl->attach( centeredBelow, leftgrp );

    uiThumbWheel* hwheel = new uiThumbWheel( this, "hRotate", true );
    hwheel->wheelMoved.notify( mWSMCB(hWheelMoved) );
    hwheel->wheelPressed.notify( mWSMCB(anyWheelStart) );
    hwheel->wheelReleased.notify( mWSMCB(anyWheelStop) );
    hwheel->attach( leftAlignedBelow, &appl.workSpace() );

    zoomslider = new uiLabeledSlider( this, "ZOOM" );
    zoomslider->sldr()->valueChanged.notify( mWSMCB(zoomChanged) );
    zoomslider->sldr()->setTickMarks( false );
    zoomslider->sldr()->setMinValue( cMinZoom );
    zoomslider->sldr()->setMaxValue( cMaxZoom );
    zoomslider->setStretch( 0, 0 );
    zoomslider->attach( rightAlignedBelow, &appl.workSpace() );

    leftgrp->attach( leftOf, &appl.workSpace() );
}


uiODSceneMgr::~uiODSceneMgr()
{
    cleanUp( false );
}


void uiODSceneMgr::cleanUp( bool startnew )
{
    while ( scenes.size() )
	scenes[0]->group()->mainObject()->close();
    	// close() cascades callbacks which remove the scene from set

    visserv->deleteAllObjects();
    vwridx = 0;
    if ( startnew ) addScene();
}


void uiODSceneMgr::addScene()
{
    uiSceneGroup* scene = new uiSceneGroup( wsp );
    scene->group()->mainObject()->closed.notify( mWSMCB(removeScene) );
    scenes += scene;
    vwridx++;

    int sceneid = visserv->addScene();
    scene->sovwr->setSceneGraph( sceneid );
    BufferString title( scenestr );
    title += vwridx;
    scene->sovwr->setTitle( title );
    visserv->setObjectName( sceneid, title );
    scene->sovwr->display();
    scene->sovwr->viewAll();
    scene->sovwr->saveHomePos();
    scene->sovwr->viewmodechanged.notify( mWSMCB(viewModeChg) );
    scene->group()->display( true, false, true );
    actMode(0);
    appl.posCtrlMgr()->setZoomValue( scene->sovwr->getCameraZoom() );
    initTree( scene, vwridx );

    if ( scenes.size() > 1 && scenes[0] )
    {
	scene->sovwr->setStereoViewing(
		scenes[0]->sovwr->isStereoViewing() );
	scene->sovwr->setStereoOffset(
		scenes[0]->sovwr->getStereoOffset() );
	scene->sovwr->setQuadBufferStereo( 
		scenes[0]->sovwr->isQuadBufferStereo() );
	
    }
}


void uiODSceneMgr::removeScene( CallBacker* cb )
{
    mDynamicCastGet(uiGroupObj*,grp,cb)
    if ( !grp ) return;
    int idxnr = -1;
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	if ( grp == scenes[idx]->group()->mainObject() )
	{
	    idxnr = idx;
	    break;
	}
    }
    if ( idxnr < 0 ) return;

    visserv->removeScene( scenes[idxnr]->itemmanager->sceneID() );
    appl.removeDockWindow( scenes[idxnr]->treewin );


    scenes[idxnr]->group()->mainObject()->closed.remove( mWSMCB(removeScene) );
    delete scenes[idxnr];
    scenes -= scenes[idxnr];
}


void uiODSceneMgr::getScenePars( IOPar& iopar )
{
    mDoAllScenes(sovwr,fillPar,iopar);
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	IOPar iop;
	scenes[idx]->sovwr->fillPar( iop );
	BufferString key = idx;
	iopar.mergeComp( iop, key );
    }
}


void uiODSceneMgr::mkScenesFrom( ODSession* session )
{
    if ( !session ) return;

    for ( int idx=0; ; idx++ )
    {
	BufferString key = idx;
	PtrMan<IOPar> scenepar = session->scenepars().subselect( key );
	if ( !scenepar || !scenepar->size() )
	{
	    if ( !idx ) continue;
	    break;
	}

	uiSceneGroup* scene = new uiSceneGroup( wsp );
	scene->group()->mainObject()->closed.notify( mWSMCB(removeScene) );
	scenes += scene;
	vwridx++;

  	if ( !scene->sovwr->usePar( *scenepar ) )
	    return false;
    
	int sceneid = scene->sovwr->sceneId();
	BufferString title( scenestr );
	title += vwridx;
  	scene->sovwr->setTitle( title );
	scene->sovwr->display();
	scene->group()->display( true, false );
	setZoomValue( scene->sovwr->getCameraZoom() );
	initTree( scene, vwridx );
    }

    rebuildTrees();
}


void uiODSceneMgr::viewModeChg( CallBacker* cb )
{
    if ( !scenes.size() ) return;

    mDynamicCastGet(uiSoViewer*,sovwr,cb)
    if ( sovwr ) setToViewMode( sovwr->isViewing() );
}


void uiODSceneMgr::setToViewMode( bool yn )
{
    mDoAllScenes(sovwr,setViewing,yn);
    visserv->setViewMode( yn );
    menuMgr()->showViewMode( yn );
}


void uiODSceneMgr::actMode( CallBacker* )
{
    setToViewMode( false );
}


void uiODSceneMgr::viewMode( CallBacker* )
{
    setToViewMode( true );
    appl.statusBar()->message( "", 0 );
    appl.statusBar()->message( "", 1 );
    appl.statusBar()->message( "", 2 );
}


void uiODSceneMgr::setMousePos()
{
    Coord3 xytpos = visserv->getMousePos(true);
    BufferString msg;
    if ( !mIsUndefined( xytpos.x ) )
    {
	BinID bid( SI().transform( Coord(xytpos.x,xytpos.y) ) );
	msg = bid.inl; msg += "/"; msg += bid.crl;
	msg += "   (";
	msg += mNINT(xytpos.x); msg += ",";
	msg += mNINT(xytpos.y); msg += ",";
	msg += SI().zIsTime() ? mNINT(xytpos.z * 1000) : xytpos.z; msg += ")";
    }

    appl.statusBar()->message( msg, 0 );
    msg = "Value = "; msg += visserv->getMousePosVal();
    appl.statusBar()->message( msg, 1 );
}


void uiODSceneMgr::setKeyBindings()
{
    if ( !scenes.size() ) return;

    BufferStringSet keyset;
    scenes[0]->sovwr->getAllKeyBindings( keyset );

    StringListInpSpec inpspec( keyset );
    inpspec.setText( scenes[0]->sovwr->getCurrentKeyBindings() );
    uiGenInputDlg dlg( appman, "Select Mouse Controls", "Select", &inpspec );
    if ( dlg.go() )
	mDoAllScenes(sovwr,setKeyBindings,dlg.text());
}


void uiODSceneMgr::setStereoViewing( bool& stereo, bool& quad )
{
    for ( int ids=0; ids<scenes.size(); ids++ )
    {
	uiSoViewer& sovwr = *scenes[ids]->sovwr;
	sovwr.setStereoViewing( stereo );
	sovwr.setQuadBufferStereo( quad );
	if ( stereo )
	    sovwr.setStereoOffset( stereooffset );

	if ( quad && !scenes[ids]->sovwr->isQuadBufferStereo() )
	{
	    sovwr.setStereoViewing( false );
	    sovwr.QuadBufferStereo( false );
	    stereo = quad = false;
	}
    }
}


void uiODSceneMgr::tile()
{ wsp->tile(); }
void uiODSceneMgr::cascade()
{ wsp->cascade(); }


void uiODSceneMgr::layoutScenes()
{
    const int nrgrps = scenes.size();
    if ( nrgrps == 1 && scenes[0] )
	scenes[0]->grp()->display( true, false, true );
    else if ( scenes[0] )
	tile();
}

void uiODSceneMgr::toHomePos( CallBacker* )
{ mDoAllScenes(sovwr,toHomePos,); }
void uiODSceneMgr::saveHomePos( CallBacker* )
{ mDoAllScenes(sovwr,saveHomePos,); }
void uiODSceneMgr::viewAll( CallBacker* )
{ mDoAllScenes(sovwr,viewAll,); }
void uiODSceneMgr::align( CallBacker* )
{ mDoAllScenes(sovwr,align,); }


void uiODSceneMgr::showRotAxis( CallBacker* )
{
    mDoAllScenes(sovwr,showRotAxis,);
    menuMgr()->showAxisMode( scenes.size() && scenes[0]->sovwr->rotAxisShown());
}


void uiODSceneMgr::setZoomValue( float val )
{
    zoomslider->sldr()->setValue( val );
}


void uiODSceneMgr::zoomChanged( CallBacker* )
{
    const float zmval = zoomslider->getValue();
    mDoAllScenes(sovwr,setCameraZoom,zmval);
}


void uiODSceneMgr::anyWheelStart( CallBacker* )
{ mDoAllScenes(sovwr,anyWheelStart,); }
void uiODSceneMgr::anyWheelStop( CallBacker* )
{ mDoAllScenes(sovwr,anyWheelStop,); }


void uiODSceneMgr::wheelMoved( CallBacker* cb, int wh, float& lastval )
{
    mDynamicCastGet(uiThumbWheel*,wheel,cb)
    if ( !wheel ) { pErrMsg("huh") ; return; }

    const float whlval = wheel->lastMoveVal();
    const float diff = lastval - whlval;

    if ( diff )
    {
	for ( int idx=0; idx<scenes.size(); idx++ )
	{
	    if ( wh == 1 )
		scenes[idx]->sovwr->rotateH( diff );
	    else if ( wh == 2 )
		scenes[idx]->sovwr->rotateV( diff );
	    else
		scenes[idx]->sovwr->dolly( diff );
	}
    }

    lastval = whlval;
}


void uiODSceneMgr::hWheelMoved( CallBacker* cb )
{ wheelMoved(cb,1,lasthrot); }
void uiODSceneMgr::vWheelMoved( CallBacker* cb )
{ wheelMoved(cb,2,lastvrot); }
void uiODSceneMgr::dWheelMoved( CallBacker* cb )
{ wheelMoved(cb,0,lastdval); }


void uiODSceneMgr::getSoViewers( ObjectSet<uiSoViewer>& vwrs )
{
    vwrs.erase();
    for ( int idx=0; idx<scenes.size(); idx++ )
	vwrs += scenes[idx]->sovwr;
}


void uiODSceneMgr::initTree( uiSceneGroup* sg, int vwridx )
{
    BufferString capt( "Tree scene " ); capt += vwridx;
    sg->treewin = new uiDockWin( &appl, capt );
    moveDockWindow( *sg->treewin, uiMainWin::Left );
    sg->treewin->setResizeEnabled( true );

    sg->lv = new uiListView( sg->treewin, "d-Tect Tree" );
    sg->lv->addColumn( "Elements" );
    sg->lv->addColumn( "Position" );
    sg->lv->setColumnWidthMode( 0, uiListView::Manual );
    sg->lv->setColumnWidth( 0, 90 );
    sg->lv->setColumnWidthMode( 1, uiListView::Manual );
    sg->lv->setColumnWidthMode( 1, uiListView::Manual);
    sg->lv->setPrefWidth( 150 );
    sg->lv->setStretch( 2, 2 );

    sg->itemmanager = new uiODTreeTop( sg, this, tifs );

    for ( int idx=0; idx<tifs->nrFactories(); idx++ )
	sg->itemmanager->addChild( tifs->getFactory(idx)->create() );

#ifdef __debug__
    sg->itemmanager->addChild( new FaultStickFactoryTreeItem );
    sg->itemmanager->addChild( new FaultFactoryTreeItem );
#endif
    sg->itemmanager->addChild( new WellFactoryTreeItem );
    sg->itemmanager->addChild( new HorizonFactoryTreeItem );
    sg->itemmanager->addChild( new PickSetFactoryTreeItem );
    sg->itemmanager->addChild( new RandomLineFactoryTreeItem );
    sg->itemmanager->addChild( new VolumeFactoryTreeItem );
    sg->itemmanager->addChild( new TimesliceFactoryTreeItem );
    sg->itemmanager->addChild( new CrosslineFactoryTreeItem );
    sg->itemmanager->addChild( new InlineFactoryTreeItem );
    sg->itemmanager->addChild( new SceneTreeItem(sg->sovwr->getTitle(),
						 sg->sovwr->sceneId() ) );
    sg->lv->display();
    sg->treewin->display();
}


void uiODSceneMgr::updateTrees()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	scene.itemmanager->updateColumnText(0);
	scene.itemmanager->updateColumnText(1);
    }
}


void uiODSceneMgr::rebuildTrees()
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	const int sceneid = scene.sovwr->sceneId();
	TypeSet<int> visids; visserv->getChildIds( sceneid, visids );

	for ( int idy=0; idy<visids.size(); idy++ )
	    DisplayTreeItem::factory(scene.itemmanager,applMgr(),visids[idy]);
    }
    updateSelectedTreeItem();
}


void uiODSceneMgr::setItemInfo( int id )
{
    mDoAllScenes(itemmanager,updateColumnText,1);
    appl.statusBar()->message( "", 0 );
    appl.statusBar()->message( "", 1 );
    appl.statusBar()->message( visserv->getInteractionMsg(id), 2 );
}


void uiODSceneMgr::updateSelectedTreeItem()
{
    const int id = visserv->getSelObjectId();
    const bool ispickset = visserv->isPickSet( id );
    if ( ispickset && !applMgr().attrserv->attrSetEditorActive() )
	actMode( 0 );

    if ( id != -1 )
    {
	setItemInfo( id );
	applMgr().modifyColorTable( id );
    }

    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	scene.itemmanager->updateSelection( id );
	scene.itemmanager->updateColumnText(0);
	scene.itemmanager->updateColumnText(1);
    }
}


int uiODSceneMgr::getIDFromName( const char* str ) const
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	const uiTreeItem* itm = scenes[idx]->itemmanager->findChild( str );
	if ( itm ) return itm->selectionKey();
    }

    return -1;
}


void uiODSceneMgr::disabRightClick( bool yn )
{
    mDoAllScenes(itemmanager,disabRightClick,yn);
}


void uiODSceneMgr::addPickSetItem( const PickSet* ps, int sceneid )
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	if ( sceneid >= 0 && sceneid != scene.sovwr->sceneId() ) continue;

	scene.itemmanager->addChild( new PickSetTreeItem(ps) );
    }
}


void uiODSceneMgr::addHorizonItem( const MultiID& mid, int sceneid )
{
    for ( int idx=0; idx<scenes.size(); idx++ )
    {
	Scene& scene = *scenes[idx];
	if ( sceneid >= 0 && sceneid != scene.sovwr->sceneId() ) continue;

	scene.itemmanager->addChild( new HorizonTreeItem(mid) );
    }
}



uiODSceneMgr::Scene::Scene( uiWorkSpace* wsp )
        : lv(0)
        , treewin(0)
        , sovwr(0)
    	, itemmanager( 0 )
{
    if ( !wsp ) return;

    uiGroup* grp = new uiGroup( wsp );
    grp->setPrefWidth( 200 );
    grp->setPrefHeight( 200 );
    sovwr = new uiSoViewer( grp );
    sovwr->setPrefWidth( 200 );
    sovwr->setPrefHeight( 200 );
    sovwr->setIcon( scene_xpm_data );
}


uiODSceneMgr::Scene::~Scene()
{
    delete sovwr;
    delete lv;
    delete treewin;
}


uiGroup* uiODSceneMgr::Scene::grp()
{
    return sovwr ? (uiGroup*)sovwr->parent() : 0;
}
