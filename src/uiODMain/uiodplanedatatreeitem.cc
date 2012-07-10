/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author: 	K. Tingdahl
 Date: 		Jul 2003
___________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiodplanedatatreeitem.cc,v 1.69 2012-07-10 08:05:36 cvskris Exp $";

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
#include "uiwellpartserv.h"
#include "visplanedatadisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "vissurvscene.h"

#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "settings.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellman.h"
#include "zaxistransform.h"


static const int cPositionIdx = 990;
static const int cGridLinesIdx = 980;

static uiODPlaneDataTreeItem::Type getType( int mnuid )
{
    switch ( mnuid )
    {
	case 0: return uiODPlaneDataTreeItem::Empty; break;
	case 1: return uiODPlaneDataTreeItem::Default; break;
	case 2: return uiODPlaneDataTreeItem::RGBA; break;
	case 3: return uiODPlaneDataTreeItem::FromWell; break;
	default: return uiODPlaneDataTreeItem::Empty;
    }
}


#define mParentShowSubMenu( treeitm, fromwell ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("&Add"), 0 ); \
    mnu.insertItem( new uiMenuItem("Add &default data"), 1 ); \
    mnu.insertItem( new uiMenuItem("Add &color blended"), 2 ); \
    if ( fromwell ) \
	mnu.insertItem( new uiMenuItem("Add at Well location..."), 3 ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 || mnuid==1 || mnuid==2 || mnuid==3 ) \
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
    positionmnuitem_.iconfnm = "orientation64";
    gridlinesmnuitem_.iconfnm = "gridlines";
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

//    getItem()->keyPressed.remove( mCB(this,uiODPlaneDataTreeItem,keyPressCB) );
    visserv_->getUiSlicePos()->positionChg.remove(
	    		mCB(this,uiODPlaneDataTreeItem,posChange) );

    visserv_->getUiSlicePos()->setDisplay( -1 );
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

	if ( type_ == FromWell )
	{
	    ObjectSet<MultiID> wellids;
	    if ( applMgr()->wellServer()->selectWells(wellids) )
	    {
		Well::Data* wd = Well::MGR().get( *wellids[0] );
		if ( wd )
		{
		    const Coord surfacecoord = wd->info().surfacecoord;
		    const BinID bid = SI().transform( surfacecoord );
		    CubeSampling cs = pdd->getCubeSampling();
		    if ( orient_ == Inline )
			cs.hrg.setInlRange( Interval<int>(bid.inl,bid.inl) );
		    else
			cs.hrg.setCrlRange( Interval<int>(bid.crl,bid.crl) );

		    pdd->setCubeSampling( cs );
		    displayDefaultData();
		}
	    }
	}

	if ( type_ == Default )
	    displayDefaultData();
    }

    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_));
    if ( !pdd ) return false;

    pdd->ref();
    pdd->selection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );
    pdd->deSelection()->notify( mCB(this,uiODPlaneDataTreeItem,selChg) );

//    getItem()->keyPressed.notify( mCB(this,uiODPlaneDataTreeItem,keyPressCB) );
    visserv_->getUiSlicePos()->positionChg.notify(
	    		mCB(this,uiODPlaneDataTreeItem,posChange) );

    return uiODDisplayTreeItem::init();
}


bool uiODPlaneDataTreeItem::getDefaultDescID( Attrib::DescID& descid )
{
    BufferString keystr( SI().pars().find(sKey::DefCube()) );
    if ( keystr.isEmpty() )
    {
	const IODir* iodir = IOM().dirPtr();
	ObjectSet<IOObj> ioobjs = iodir->getObjs();
	int nrod3d = 0;
	int def3didx = 0;
	for ( int idx=0; idx<ioobjs.size(); idx++ )
	{
	    SeisIOObjInfo seisinfo( ioobjs[idx] );
	    if ( seisinfo.isOK() && !seisinfo.is2D() )
	    {
		nrod3d++;
		def3didx = idx;
	    }
	}

	if ( nrod3d == 1 )
	    keystr = ioobjs[def3didx]->key();
    }

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    descid = attrserv->getStoredID( keystr.buf(),false );
    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, true );
    if ( descid.isValid() && ads )
	return true;

    BufferString msg( "No or no valid default volume found."
	"You can set a default volume in the 'Manage Seismics' "
	"window. Do you want to go there now? "
	"On 'No' an empty plane will be added" );
    const bool tomanage = uiMSG().askGoOn( msg );
    if ( tomanage )
    {
	applMgr()->seisServer()->manageSeismics( false );
	return getDefaultDescID( descid );
    }

    return false;
}


bool uiODPlaneDataTreeItem::displayDefaultData()
{
    Attrib::DescID descid;
    if ( !getDefaultDescID(descid) )
	return false;

    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( false, true );
    Attrib::SelSpec as( 0, descid, false, "" );
    as.setRefFromID( *ads );
    visserv_->setSelSpec( displayid_, 0, as );
    return visserv_->calculateAttrib( displayid_, 0, false );
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

    if ( pdd->isSelected() )
	visserv_->getUiSlicePos()->setDisplay( displayid_ );
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
	if ( !scene )
	    res = cs.zrg.start;
	else
	{
	    const ZDomain::Def& zdef = scene->zDomainInfo().def_;
	    const float zval = cs.zrg.start * zdef.userFactor();
	    res = toString( zdef.isTime() || zdef.userFactor()==1000
		    ? (float)(mNINT32(zval)) : zval );
	}
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


void uiODPlaneDataTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID() != displayID() )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionmnuitem_,
		      !islocked, false );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &gridlinesmnuitem_,
		      true, false );
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
	    maxcs = scene->getCubeSampling();

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

    int step = scl.valueOf(caps->data);
    caps->data.setKey( 0 );
    movePlane( fwd, step );
}


void uiODPlaneDataTreeItem::movePlane( bool forward, int step )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv_->getObject(displayid_))

    CubeSampling cs = pdd->getCubeSampling();
    const int dir = forward ? step : -step;

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
    
    mParentShowSubMenu( uiODInlineTreeItem, true );
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
    
    mParentShowSubMenu( uiODCrosslineTreeItem, true );
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
     
    mParentShowSubMenu( uiODZsliceTreeItem, false );
}


uiODZsliceTreeItem::uiODZsliceTreeItem( int id, Type tp )
    : uiODPlaneDataTreeItem( id, ZSlice, tp )
{
}
