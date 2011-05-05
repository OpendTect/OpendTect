/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodplanedatatreeitem.cc,v 1.46 2011-05-05 07:26:12 cvsnanne Exp $";

#include "uiodplanedatatreeitem.h"

#include "uiattribpartserv.h"
#include "uigridlinesdlg.h"
#include "uilistview.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uishortcutsmgr.h"
#include "uislicesel.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "visplanedatadisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvscene.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "keystrs.h"
#include "linekey.h"
#include "settings.h"
#include "survinfo.h"
#include "zaxistransform.h"


static const int cPositionIdx = 990;
static const int cGridLinesIdx = 980;

static uiODPlaneDataTreeItem::Type getType( int mnuid )
{
    return mnuid == 0 ? uiODPlaneDataTreeItem::Empty
		      : (mnuid==1 ? uiODPlaneDataTreeItem::Default
			          : uiODPlaneDataTreeItem::RGBA);
}


#define mParentShowSubMenu( treeitm ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("&Add"), 0 ); \
    mnu.insertItem( new uiMenuItem("Add &default data"), 1 ); \
    mnu.insertItem( new uiMenuItem("Add &color blended"), 2 ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 || mnuid==1 || mnuid==2 ) \
        addChild( new treeitm(-1,getType(mnuid)), false ); \
    handleStandardItems( mnuid ); \
    return true


uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, Orientation o, Type t )
    : orient_(o)
    , type_(t)
    , positiondlg_(0)
    , positionmnuitem_("P&osition ...",cPositionIdx)
    , gridlinesmnuitem_("&Gridlines ...",cGridLinesIdx)
{
    displayid_ = did;
    positionmnuitem_.iconfnm = "orientation64.png";
}


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( pdd )
    {
	pdd->selection()->remove( mCB(this,uiODPlaneDataTreeItem,selChg) );
	pdd->deSelection()->remove( mCB(this,uiODPlaneDataTreeItem,selChg) );
	pdd->unRef();
    }

    getItem()->keyPressed.remove( mCB(this,uiODPlaneDataTreeItem,keyPressCB) );
    visserv_->getUiSlicePos()->positionChg.remove(
	    		mCB(this,uiODPlaneDataTreeItem,posChange) );

    visserv_->getUiSlicePos()->setDisplay( 0 );
    delete positiondlg_;
}


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid_==-1 )
    {
	RefMan<visSurvey::PlaneDataDisplay> pdd =
			visSurvey::PlaneDataDisplay::create();
	displayid_ = pdd->id();
	if ( type_ == RGBA )
	{
	    pdd->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
	    pdd->addAttrib();
	    pdd->addAttrib();
	    pdd->addAttrib();
	}

	pdd->setOrientation( (visSurvey::PlaneDataDisplay::Orientation)orient_);
	visserv_->addObject( pdd, sceneID(), true );

	BufferString res;
	Settings::common().get( "dTect.Texture2D Resolution", res );
	for ( int idx=0; idx<pdd->nrResolutions(); idx++ )
	{
	    if ( res == pdd->getResolutionName(idx) )
		pdd->setResolution( idx, 0 );
	}

	if ( type_ == Default )
	{
	    uiAttribPartServer* attrserv = applMgr()->attrServer();
	    const char* keystr = SI().pars().find( sKey::DefCube );
	    Attrib::DescID descid = attrserv->getStoredID( keystr, false );
	    const Attrib::DescSet* ads =
		Attrib::DSHolder().getDescSet( false, true );
	    if ( descid.isValid() && ads )
	    {
		Attrib::SelSpec as( 0, descid, false, "" );
		as.setRefFromID( *ads );
		visserv_->setSelSpec( displayid_, 0, as );
		visserv_->calculateAttrib( displayid_, 0, false );
	    }
	    else
	    {
		uiMSG().error( "No or no valid default volume found\n"
				"An empty plane will be added" );
	    }
	}
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return false;

    pdd->ref();
    pdd->selection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );
    pdd->deSelection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );

    getItem()->keyPressed.notify( mCB(this,uiODPlaneDataTreeItem,keyPressCB) );
    visserv_->getUiSlicePos()->positionChg.notify(
	    		mCB(this,uiODPlaneDataTreeItem,posChange) );

    return uiODDisplayTreeItem::init();
}


void uiODPlaneDataTreeItem::posChange( CallBacker* )
{
    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != displayid_ )
	return;

    movePlaneAndCalcAttribs( slicepos->getCubeSampling() );
}


void uiODPlaneDataTreeItem::selChg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    visserv_->getUiSlicePos()->setDisplay( pdd->isSelected() ? pdd : 0 );
}


BufferString uiODPlaneDataTreeItem::createDisplayName() const
{
    BufferString res;
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))
    const CubeSampling cs = pdd->getCubeSampling(true,true);
    const visSurvey::PlaneDataDisplay::Orientation orientation =
						    pdd->getOrientation();

    if ( orientation==visSurvey::PlaneDataDisplay::Inline )
	res = cs.hrg.start.inl;
    else if ( orientation==visSurvey::PlaneDataDisplay::Crossline )
	res = cs.hrg.start.crl;
    else
    {
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene && !scene->getZAxisTransform() )
	{
	    const float zval = cs.zrg.start * SI().zFactor();
	    res = toString( SI().zIsTime() ? (float)(mNINT(zval)) : zval );
	}
	else
	    res = cs.zrg.start;
    }

    return res;
}

void uiODPlaneDataTreeItem::addToToolBarCB( CallBacker* cb )
{
    mDynamicCastGet(uiTreeItemTBHandler*,tb,cb);
    if ( !tb || tb->menuID() != displayID() || !isSelected() )
	return;

    mAddMenuItem( tb, &positionmnuitem_, !visserv_->isLocked(displayid_),
	          false );
    uiODDisplayTreeItem::addToToolBarCB( cb );
}


void uiODPlaneDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    mAddMenuItem( menu, &positionmnuitem_, !visserv_->isLocked(displayid_),
	          false );
    mAddMenuItem( menu, &gridlinesmnuitem_, true, false );
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() || mnuid==-1 )
	return;
    
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;
	delete positiondlg_;
	CubeSampling maxcs = SI().sampling(true);
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	if ( scene && scene->getZAxisTransform() )
	{
	    const Interval<float> zintv =
		scene->getZAxisTransform()->getZInterval( false );
	    maxcs.zrg.start = zintv.start;
	    maxcs.zrg.stop = zintv.stop;
	    maxcs.zrg.step = scene->getZAxisTransform()->getGoodZStep();
	}

	positiondlg_ = new uiSliceSelDlg( getUiParent(),
				pdd->getCubeSampling(true,true), maxcs,
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos), 
				(uiSliceSel::Type)orient_,scene->zDomainInfo());
	positiondlg_->windowClosed.notify( 
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg_->go();
	pdd->getMovementNotifier()->notify(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
	applMgr()->enableMenusAndToolBars( false );
	applMgr()->visServer()->disabToolBars( false );
    }
    else if ( mnuid == gridlinesmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;

	uiGridLinesDlg gldlg( getUiParent(), pdd );
	gldlg.go();
    }
}


void uiODPlaneDataTreeItem::updatePositionDlg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv_->getObject(displayid_))
    const CubeSampling newcs = pdd->getCubeSampling( true, true );
    positiondlg_->setCubeSampling( newcs );
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv_->getObject(displayid_))
    CubeSampling newcs = positiondlg_->getCubeSampling();
    bool samepos = newcs == pdd->getCubeSampling();
    if ( positiondlg_->uiResult() && !samepos )
	movePlaneAndCalcAttribs( newcs );

    applMgr()->enableMenusAndToolBars( true );
    applMgr()->enableSceneManipulation( true );
    pdd->getMovementNotifier()->remove(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv_->getObject(displayid_))
    mDynamicCastGet(uiSliceSel*,slicesel,cb)
    if ( !slicesel ) return;

    movePlaneAndCalcAttribs( slicesel->getCubeSampling() );
}


void uiODPlaneDataTreeItem::movePlaneAndCalcAttribs( const CubeSampling& cs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv_->getObject(displayid_))

    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    for ( int attrib=visserv_->getNrAttribs(displayid_); attrib>=0; attrib--)
	visserv_->calculateAttrib( displayid_, attrib, false );

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPlaneDataTreeItem::keyPressCB( CallBacker* cb )
{
    mCBCapsuleGet(uiKeyDesc,caps,cb)
    if ( !caps ) return;
    const uiShortcutsList& scl = SCMgr().getList( "ODScene" );
    BufferString act( scl.nameOf(caps->data) );
    const bool fwd = act == "Move slice forward";
    const bool bwd = fwd ? false : act == "Move slice backward";
    if ( !fwd && !bwd ) return;

    caps->data.setKey( 0 );
    movePlane( fwd );
}


void uiODPlaneDataTreeItem::movePlane( bool forward )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    CubeSampling cs = pdd->getCubeSampling();
    const int dir = forward ? 1 : -1;

    if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
    {
	cs.hrg.start.inl += cs.hrg.step.inl * dir;
	cs.hrg.stop.inl = cs.hrg.start.inl;
    }
    else if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Crossline )
    {
	cs.hrg.start.crl += cs.hrg.step.crl * dir;
	cs.hrg.stop.crl = cs.hrg.start.crl;
    }
    else if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Zslice )
    {
	cs.zrg.start += cs.zrg.step * dir;
	cs.zrg.stop = cs.zrg.start;
    }
    else
	return;

    movePlaneAndCalcAttribs( cs );
}


uiTreeItem*
    uiODInlineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Inline )
	return 0;

    mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
	    	     pdd->getChannels2RGBA() );

    return new uiODInlineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODTreeItem( "Inline" )
{ }


bool uiODInlineParentTreeItem::showSubMenu()
{
    if ( !SI().crlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( "Flat survey, disabled inline display" );
	return false;
    }
    
    mParentShowSubMenu( uiODInlineTreeItem );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, Inline, tp )
{}


uiTreeItem*
    uiODCrosslineTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Crossline )
	return 0;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
	    	    pdd->getChannels2RGBA());
    return new uiODCrosslineTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODTreeItem( "Crossline" )
{ }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    if ( !SI().inlRange(true).width() ||
	  SI().zRange(true).width() < SI().zStep() * 0.5 )
    {
	uiMSG().warning( "Flat survey, disabled cross-line display" );
	return false;
    }
    
    mParentShowSubMenu( uiODCrosslineTreeItem );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, Crossline, tp )
{}


uiTreeItem*
    uiODZsliceTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));

    if ( !pdd || pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Zslice )
	return 0;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
	    	    pdd->getChannels2RGBA());

    return new uiODZsliceTreeItem( visid,
	rgba ? uiODPlaneDataTreeItem::RGBA : uiODPlaneDataTreeItem::Empty );
}


uiODZsliceParentTreeItem::uiODZsliceParentTreeItem()
    : uiODTreeItem( "Z-slice" )
{}


bool uiODZsliceParentTreeItem::showSubMenu()
{
     if ( !SI().inlRange(true).width() || !SI().crlRange(true).width() )
     {
	 uiMSG().warning( "Flat survey, disabled z display" );
	 return false;
     }
     
    mParentShowSubMenu( uiODZsliceTreeItem );
}


uiODZsliceTreeItem::uiODZsliceTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, ZSlice, tp )
{
}
