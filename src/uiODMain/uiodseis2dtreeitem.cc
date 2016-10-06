/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/

#include "uiodseis2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiattr2dsel.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiodeditattribcolordlg.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "visseis2ddisplay.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "emmanager.h"
#include "externalattrib.h"
#include "dbman.h"
#include "posinfo2d.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "survgeom2d.h"


static TypeSet<int> selcomps;

const char* uiODLine2DParentTreeItem::sKeyRightClick()
{ return "<right-click>"; }
const char* uiODLine2DParentTreeItem::sKeyUnselected()
{ return "<Unselected>"; }
static const char* sKeySelecting()
{ return "<Selecting>"; }


#define cAdd		1000
#define cGridFrom3D	999
#define cFrom3D		998
#define cTo3D		997

#define cAddAttr	996
#define cRemoveAttr	993
#define cReplaceAttr	992
#define cDispAttr	991
#define cHideAttr	990
#define cEditColorSetts 989

#define cDisplayAll	988
#define cHideAll	987


uiODLine2DParentTreeItem::uiODLine2DParentTreeItem()
    : uiODSceneTreeItem( tr("2D Line") )
    , visserv_(ODMainWin()->applMgr().visServer())
    , additm_(m3Dots(uiStrings::sAdd()),cAdd)
    , create2dgridfrom3ditm_(m3Dots(tr("Create 2D Grid from 3D")),cGridFrom3D)
    , extractfrom3ditm_(m3Dots(tr("Extract from 3D")),cFrom3D)
    , generate3dcubeitm_(m3Dots(tr("Generate 3D Cube")),cTo3D)
    , addattritm_(tr("Add Attribute"),cAddAttr)
    , removeattritm_(tr("Remove Attribute"),cRemoveAttr)
    , replaceattritm_(tr("Replace Attribute"),cReplaceAttr)
    , dispattritm_(tr("Display Attribute"),cDispAttr)
    , hideattritm_(tr("Hide Attribute"),cHideAttr)
    , editcoltabitm_(tr("Edit Color Settings"),cEditColorSetts)
    , displayallitm_(tr("Display All"),cDisplayAll)
    , hideallitm_(tr("Hide All"),cHideAll)
{
}


uiODLine2DParentTreeItem::~uiODLine2DParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODLine2DParentTreeItem::iconName() const
{ return "tree-geom2d"; }


bool uiODLine2DParentTreeItem::init()
{
    if ( !uiODTreeItem::init() )
	return false;

    MenuHandler* menu = visserv_->getMenuHandler();
    mAttachCB( menu->initnotifier, uiODLine2DParentTreeItem::createMenuCB );
    mAttachCB( menu->handlenotifier, uiODLine2DParentTreeItem::handleMenuCB );
    return true;
}


bool uiODLine2DParentTreeItem::showSubMenu()
{
    return visserv_->showMenu( selectionKey(), uiMenuHandler::fromTree() );
}


int uiODLine2DParentTreeItem::selectionKey() const
{
    if ( children_.size() < 1 )
	return -1;

    mDynamicCastGet(const uiODDisplayTreeItem*,itm,children_[0]);
    return itm ? 100000+itm->displayID() : -1;
}


void uiODLine2DParentTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu || menu->menuID()!=selectionKey() )
	return;

    createMenu( menu, false );
}


#define mAddAttrBasedItem( attritm ) \
    mAddMenuItem( menu, &attritm, true, false ); \
    attritm.removeItems(); \
    for ( int idx=0; idx<displayedattribs.size(); idx++ ) \
    attritm.addItem( \
	 new MenuItem(mToUiStringTodo(displayedattribs.get(idx)),varmenuid++), \
		      true );

#define mAddDispHideAllItems( itm ) \
    mAddMenuItem( menu, &itm, true, false ); \
    itm.removeItems(); \
    itm.addItem( new MenuItem(uiStrings::sLineName(mPlural),varmenuid++),true);\
    itm.addItem( new MenuItem(uiStrings::s2DPlane(mPlural),varmenuid++),true); \
    itm.addItem( new MenuItem(uiStrings::sLineGeometry(),varmenuid++), true );


void uiODLine2DParentTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    mAddMenuItem( menu, &additm_, true, false );
    if ( SI().has3D() )
    {
	mAddMenuItem( menu, &create2dgridfrom3ditm_, true, false );
	mAddMenuItem( menu, &extractfrom3ditm_, true, false );
    }

#ifdef __debug__
    mAddMenuItem( menu, &generate3dcubeitm_, true, false );
#endif

    BufferStringSet displayedattribs;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	const int id = ((uiOD2DLineTreeItem*)children_[idx])->displayID();
	const int nrattribs = applMgr()->visServer()->getNrAttribs( id );
	for ( int adx=0; adx<nrattribs; adx++ )
	{
	    const Attrib::SelSpec* ds =
		applMgr()->visServer()->getSelSpec( id, adx );
	    if ( ds && ds->id() == Attrib::SelSpec::cOtherAttrib() )
		continue;

	    BufferString attribname = sKeyUnselected();
	    if ( ds && ds->userRef() && *ds->userRef() )
		attribname = ds->userRef();

	    if ( ds && ds->isNLA() )
	    {
		attribname = ds->objectRef();
		const BufferString nodenm = DBM().nameFor( ds->userRef() );
		attribname += " ("; attribname += nodenm; attribname += ")";
	    }

	    displayedattribs.addIfNew( attribname );
	}
    }

    int varmenuid = 500;
    if ( !children_.isEmpty() )
    {
	mAddMenuItem( menu, &addattritm_, true, false );
	if ( !displayedattribs.isEmpty() )
	{
	    mAddAttrBasedItem( replaceattritm_ );
	    if ( displayedattribs.size()>1 )
		{ mAddAttrBasedItem( removeattritm_ ); }

	    const int emptyidx = displayedattribs.indexOf( sKeyUnselected() );
	    if ( emptyidx >= 0 ) displayedattribs.removeSingle( emptyidx );
	    if ( displayedattribs.size() )
	    {
		mAddAttrBasedItem( dispattritm_ );
		mAddAttrBasedItem( hideattritm_ );
		mAddAttrBasedItem( editcoltabitm_ );
	    }
	}

	mAddDispHideAllItems( displayallitm_ );
	mAddDispHideAllItems( hideallitm_ );
    }

    addStandardItems( menu );
}


void uiODLine2DParentTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || menu->isHandled() || menu->menuID()!=selectionKey() ||
	    menuid==-1 )
	return;

    TypeSet<Pos::GeomID> displayedgeomids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx]);
	mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
			visserv_->getObject(itm->displayID()));
	if ( !s2d ) continue;

	displayedgeomids += s2d->getGeomID();
    }

    if ( menuid == additm_.id )
    {
	int action = 0;
	TypeSet<Pos::GeomID> geomids;
	applMgr()->seisServer()->select2DLines( geomids, action );
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	for ( int idx=geomids.size()-1; idx>=0; idx-- )
	{
	    setMoreObjectsToDoHint( idx>0 );
	    addChild( new uiOD2DLineTreeItem(geomids[idx]), false );
	}
	cursorchgr.restore();

	if ( action==0 || geomids.isEmpty() )
	    return;
	else if ( action == 1 )
	    loadDefaultData();
	else if ( action == 2 )
	    selectLoadAttribute( geomids );
    }
    else if ( menuid == create2dgridfrom3ditm_.id )
	ODMainWin()->applMgr().create2DGrid();
    else if ( menuid == extractfrom3ditm_.id )
	ODMainWin()->applMgr().create2DFrom3D();
    else if ( menuid == addattritm_.id )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx]);
	    const FixedString topattrnm = itm->nrChildren()<=0 ? "" :
		itm->getChild(itm->nrChildren()-1)->name().getOriginalString();
	    if ( topattrnm != sKeyRightClick() )
		itm->addAttribItem();
	}

	setTopAttribName( sKeySelecting() );
	if ( !selectLoadAttribute(displayedgeomids,sKeySelecting()) )
	    setTopAttribName( sKeyRightClick() );
    }
    else if ( menuid == generate3dcubeitm_.id )
	ODMainWin()->applMgr().create3DFrom2D();
    else if ( replaceattritm_.findItem(menuid) )
    {
	const MenuItem* itm = replaceattritm_.findItem( menuid );
	FixedString attrnm = itm->text.getOriginalString();
	if ( attrnm == sKeyUnselected() ) attrnm = sKeyRightClick();
	selectLoadAttribute( displayedgeomids, attrnm );
    }
    else if ( removeattritm_.findItem(menuid) )
    {
	const MenuItem* itm = removeattritm_.findItem( menuid );
	FixedString attrnm = itm->text.getOriginalString();
	if ( attrnm == sKeyUnselected() ) attrnm = sKeyRightClick();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitm,children_[idx]);
	    if ( lineitm ) lineitm->removeAttrib( attrnm );
	}
    }
    else if ( dispattritm_.findItem(menuid) || hideattritm_.findItem(menuid) )
    {
	const MenuItem* itm = dispattritm_.findItem( menuid );
	const bool disp = itm;
	if ( !itm ) itm = hideattritm_.findItem( menuid );
	const FixedString attrnm = itm->text.getOriginalString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	for ( int idx=0; idx<set.size(); idx++ )
	    set[idx]->setChecked( disp, true );
    }
    else if ( editcoltabitm_.findItem(menuid) )
    {
	const MenuItem* itm = editcoltabitm_.findItem( menuid );
	const FixedString attrnm = itm->text.getOriginalString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	if ( set.size() )
	{
	    uiODEditAttribColorDlg dlg( ODMainWin(), set, attrnm );
	    dlg.go();
	}
    }
    else if ( displayallitm_.findItem(menuid) || hideallitm_.findItem(menuid) )
    {
	const MenuItem* itm = displayallitm_.findItem( menuid );
	const bool disp = itm;
	if ( !itm ) itm = hideallitm_.findItem( menuid );
	const int itemidx = disp ? displayallitm_.itemIndex(menuid)
				 : hideallitm_.itemIndex(menuid);
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,treeitm,children_[idx]);
	    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
			    visserv_->getObject(treeitm->displayID()));
	    if ( !s2d ) continue;

	    switch ( itemidx )
	    {
		case 0: s2d->showLineName( disp ); break;
		case 1: s2d->showPanel( disp ); break;
		case 2: s2d->showPolyLine( disp ); break;
	    }
	}
    }
    else
	handleStandardMenuCB( cb );
}


void uiODLine2DParentTreeItem::setTopAttribName( const char* nm )
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx]);
	if ( itm->nrChildren() > 0 )
	    itm->getChild(itm->nrChildren()-1)->setName( toUiString(nm) );
    }
}


bool uiODLine2DParentTreeItem::loadDefaultData()
{
    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid,true) )
	return false;

    const Attrib::DescSet* ads = Attrib::eDSHolder().getDescSet( true, false );
    if ( !ads )
	return false;

    const Attrib::Desc* desc = ads->getDesc( descid );
    if ( !desc )
    {
	ads = Attrib::eDSHolder().getDescSet( true, true );
	if ( !ads )
	    return false;

	desc = ads->getDesc( descid );
	if ( !desc )
	    return false;
    }

    const char* attrnm = desc->userRef();
    uiTaskRunner uitr( ODMainWin() );
    ObjectSet<uiTreeItem> set;
    findChildren( sKeyRightClick(), set );
    {
	for ( int idx=0; idx<set.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineSetAttribItem*,item,set[idx])
	    if ( item ) item->displayStoredData( attrnm, 0, uitr );
	}
    }

    return true;
}


bool uiODLine2DParentTreeItem::selectLoadAttribute(
	const TypeSet<Pos::GeomID>& geomids, const char* curattrnm )
{
    const Attrib::DescSet* ds =
	applMgr()->attrServer()->curDescSet( true );
    const NLAModel* nla = applMgr()->attrServer()->getNLAModel( true );
    uiAttr2DSelDlg dlg( ODMainWin(), ds, geomids, nla, curattrnm );
    if ( !dlg.go() ) return false;

    uiTaskRunner uitr( ODMainWin() );
    ObjectSet<uiTreeItem> set;
    findChildren( curattrnm, set );
    const int attrtype = dlg.getSelType();
    if ( attrtype == 0 || attrtype == 1 )
    {
	const char* newattrnm = dlg.getStoredAttrName();
	for ( int idx=0; idx<set.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineSetAttribItem*,item,set[idx])
	    if ( item ) item->displayStoredData(
			    newattrnm, dlg.getComponent(), uitr );
	}
    }
    else if ( attrtype == 2 || attrtype == 3 )
    {
	Attrib::SelSpec as;
	if ( attrtype == 2 )
	{
	    const Attrib::Desc* desc = ds->getDesc(dlg.getSelDescID());
	    if ( !desc )
	    {
		uiMSG().error(tr("Selected attribute is not available"));
		return true;
	    }

	    as.set( *desc );
	}
	else if ( nla )
	{
	    as.set( 0, dlg.getSelDescID(), attrtype == 2, "" );
	    as.setObjectRef( applMgr()->nlaServer()->modelName() );
	    as.setRefFromID( *nla );
	}

	as.set2DFlag( true );
	for ( int idx=0; idx<set.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineSetAttribItem*,item,set[idx])
	    item->setAttrib( as, uitr );
	}
    }

    return true;
}


// Line2DTreeItemFactory
uiTreeItem*
    Line2DTreeItemFactory::createForVis( int visid, uiTreeItem* treeitem ) const
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    ODMainWin()->applMgr().visServer()->getObject(visid))
    if ( !s2d || !treeitem ) return 0;

    uiOD2DLineTreeItem* newsubitm =
	new uiOD2DLineTreeItem( s2d->getGeomID(), visid );

    if ( newsubitm )
       treeitem->addChild( newsubitm,true );

    return 0;
}


uiOD2DLineTreeItem::uiOD2DLineTreeItem( Pos::GeomID geomid, int displayid )
    : linenmitm_(tr("Show Linename"))
    , panelitm_(tr("Show 2D Plane"))
    , polylineitm_(tr("Show Line Geometry"))
    , positionitm_(m3Dots(tr("Position")))
    , geomid_(geomid)
{
    name_ = toUiString(Survey::GM().getName( geomid ));
    displayid_ = displayid;

    positionitm_.iconfnm = "orientation64";
    linenmitm_.checkable = true;
    panelitm_.checkable = true;
    polylineitm_.checkable = true;
}


uiOD2DLineTreeItem::~uiOD2DLineTreeItem()
{
    applMgr()->getOtherFormatData.remove(
	    mCB(this,uiOD2DLineTreeItem,getNewData) );
}


const char* uiOD2DLineTreeItem::parentType() const
{ return typeid(uiODLine2DParentTreeItem).name(); }


bool uiOD2DLineTreeItem::init()
{
    bool newdisplay = false;
    if ( displayid_==-1 )
    {
	mDynamicCastGet(uiODLine2DParentTreeItem*,parentitm,parent_)
	if ( !parentitm ) return false;

	visSurvey::Seis2DDisplay* s2d = new visSurvey::Seis2DDisplay;
	visserv_->addObject( s2d, sceneID(), true );
	displayid_ = s2d->id();

	s2d->turnOn( true );
	newdisplay = true;
    }

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return false;

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom);
    if ( !geom2d )
	return false;

    s2d->setGeomID( geomid_ );
    s2d->setName( toUiString(geom2d->getName()) );
    //If restore, we use the old display range after set the geometry.
    const Interval<int> oldtrcnrrg = s2d->getTraceNrRange();
    const Interval<float> oldzrg = s2d->getZRange( true );
    if ( newdisplay && (geom2d->data().positions().size() > 300000000 ||
			geom2d->data().zRange().nrSteps() > 299999999) )
    {
       uiString msg = tr("Either trace size or z size is beyond max display "
			 "size of 3 X 10 e8. You can right click the line name "
			 "to change position range to view part of the data.");
       uiMSG().warning( msg );
    }

    s2d->setGeometry( geom2d->data() );
    if ( !newdisplay )
    {
	if ( !oldtrcnrrg.isUdf() )
	    s2d->setTraceNrRange( oldtrcnrrg );

	if ( !oldzrg.isUdf() )
	    s2d->setZRange( oldzrg );
    }
    else
    {
	const bool hasworkzrg = SI().zRange(true) != SI().zRange(false);
	if ( hasworkzrg )
	{
	    StepInterval<float> newzrg = geom2d->data().zRange();
	    newzrg.limitTo( SI().zRange(true) );
	    s2d->setZRange( newzrg );
	}
    }

    if ( applMgr() )
	applMgr()->getOtherFormatData.notify(
	    mCB(this,uiOD2DLineTreeItem,getNewData) );

    return uiODDisplayTreeItem::init();
}


uiString uiOD2DLineTreeItem::createDisplayName() const
{
    return visserv_->getObjectName(displayid_);
}


uiODDataTreeItem* uiOD2DLineTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiOD2DLineSetAttribItem::factory().create(0,*as,parenttype,false) : 0;

    if ( !res ) res = new uiOD2DLineSetAttribItem( parenttype );
    return res;
}


void uiOD2DLineTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !menu || menu->menuID() != displayID() || !s2d ) return;

    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &linenmitm_,
		      true, s2d->isLineNameShown() );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &panelitm_,
		      true, s2d->isPanelShown() );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &polylineitm_,
		      true, s2d->isPolyLineShown() );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionitm_, true, false);
}


void uiOD2DLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || menu->isHandled() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_));
    if ( !s2d || menu->menuID() != displayID() )
	return;

    if ( mnuid==linenmitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showLineName( !s2d->isLineNameShown() );
    }
    else if ( mnuid==panelitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showPanel( !s2d->isPanelShown() );
    }
    else if ( mnuid==polylineitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showPolyLine( !s2d->isPolyLineShown() );
    }
    else if ( mnuid==positionitm_.id )
    {
	menu->setIsHandled(true);

	TrcKeyZSampling maxcs;
	assign( maxcs.zsamp_, s2d->getMaxZRange(true)  );
	maxcs.hsamp_.start_.crl() = s2d->getMaxTraceNrRange().start;
	maxcs.hsamp_.stop_.crl() = s2d->getMaxTraceNrRange().stop;

	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	CallBack dummy;
	uiSliceSelDlg positiondlg( getUiParent(), s2d->getTrcKeyZSampling(true),
				   maxcs, dummy, uiSliceSel::TwoD,
				   scene->zDomainInfo() );
	if ( !positiondlg.go() ) return;
	const TrcKeyZSampling newcs = positiondlg.getTrcKeyZSampling();

	const Interval<float> newzrg( newcs.zsamp_.start, newcs.zsamp_.stop );
	if ( !newzrg.isEqual(s2d->getZRange(true),mDefEps) )
	{
	    s2d->annotateNextUpdateStage( true );
	    s2d->setZRange( newzrg );
	}

	const Interval<int> ntrcnrrg(
	    newcs.hsamp_.start_.crl(), newcs.hsamp_.stop_.crl() );
	if ( ntrcnrrg != s2d->getTraceNrRange() )
	{
	    if ( !s2d->getUpdateStageNr() )
		s2d->annotateNextUpdateStage( true );

	    s2d->setTraceNrRange( ntrcnrrg );
	}

	if ( s2d->getUpdateStageNr() )
	{
	    s2d->annotateNextUpdateStage( true );
	    for ( int idx=0; idx<s2d->nrAttribs(); idx++ )
	    {
		if ( s2d->getSelSpec(idx)
		  && s2d->getSelSpec(idx)->id().isValid() )
		    visserv_->calculateAttrib( displayid_, idx, false );
	    }
	    s2d->annotateNextUpdateStage( false );
	}

	updateColumnText(0);
    }
}


bool uiOD2DLineTreeItem::addStoredData( const char* nm, int component,
					  uiTaskRunner& uitr )
{
    addAttribItem();
    const int lastattridx = children_.size() - 1;
    if ( lastattridx < 0 ) return false;

    mDynamicCastGet( uiOD2DLineSetAttribItem*, lsai, children_[lastattridx] );
    if ( !lsai ) return false;

    return lsai->displayStoredData( nm, component, uitr );
}


void uiOD2DLineTreeItem::addAttrib( const Attrib::SelSpec& myas,
				      uiTaskRunner& uitr )
{
    addAttribItem();
    const int lastattridx = children_.size() - 1;
    if ( lastattridx < 0 ) return;

    mDynamicCastGet( uiOD2DLineSetAttribItem*, lsai, children_[lastattridx] );
    if ( !lsai ) return;

    lsai->setAttrib( myas, uitr );
}


void uiOD2DLineTreeItem::getNewData( CallBacker* cb )
{
    const int visid = applMgr()->otherFormatVisID();
    if ( visid != displayid_ ) return;

    const int attribnr = applMgr()->otherFormatAttrib();

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return;

    const TrcKeyZSampling tkzs = s2d->getTrcKeyZSampling( false );
    const TypeSet<Attrib::SelSpec>& as = *s2d->getSelSpecs( attribnr );

    RefMan<DataPack> dp = 0;
    if ( as[0].id().asInt() == Attrib::SelSpec::cOtherAttrib().asInt() )
    {
	PtrMan<Attrib::ExtAttribCalc> calc =
	    Attrib::ExtAttrFact().create( 0, as[0], false );
	if ( !calc )
	{
	    uiMSG().error( tr("Attribute cannot be created") );
	    return;
	}

	uiTaskRunner uitr( ODMainWin() );
	dp = calc->createAttrib( tkzs, DataPack::ID::get(0), &uitr );
    }
    else
    {
	applMgr()->attrServer()->setTargetSelSpecs( as );
	const DataPackMgr& dpm = DPM(DataPackMgr::SeisID());
	ConstRefMan<RegularSeisDataPack> regsdp =
			dpm.get( s2d->getDataPackID(attribnr) );
	dp = applMgr()->attrServer()->createOutput( tkzs, regsdp );
    }

    if ( !dp )
	return;

    s2d->setDataPackID( attribnr, dp->id(), 0 );
    s2d->showPanel( true );
}


void uiOD2DLineTreeItem::showLineName( bool yn )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( s2d ) s2d->showLineName( yn );
}


void uiOD2DLineTreeItem::setZRange( const Interval<float> newzrg )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return;

    s2d->annotateNextUpdateStage( true );
    s2d->setZRange( newzrg );
    s2d->annotateNextUpdateStage( true );
    for ( int idx=0; idx<s2d->nrAttribs(); idx++ )
    {
	if ( s2d->getSelSpec(idx) && s2d->getSelSpec(idx)->id().isValid() )
	    visserv_->calculateAttrib( displayid_, idx, false );
    }
    s2d->annotateNextUpdateStage( false );
}


void uiOD2DLineTreeItem::removeAttrib( const char* attribnm )
{
    BufferString itemnm = attribnm;
    if ( itemnm == uiODLine2DParentTreeItem::sKeyUnselected() )
	itemnm = uiODLine2DParentTreeItem::sKeyRightClick();

    int nrattribitms = 0;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineSetAttribItem*,item,children_[idx]);
	if ( item ) nrattribitms++;
    }

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,dataitem,children_[idx]);
	mDynamicCastGet(uiOD2DLineSetAttribItem*,attribitem,children_[idx]);
	if ( !dataitem || itemnm!=dataitem->name().getFullString() ) continue;

	if ( attribitem && nrattribitms<=1 )
	{
	    attribitem->clearAttrib();
	    return;
	}

	applMgr()->visServer()->removeAttrib( displayID(),
					      dataitem->attribNr() );
	dataitem->prepareForShutdown();
	removeChild( dataitem );
	idx--;
    }
}


uiOD2DLineSetAttribItem::uiOD2DLineSetAttribItem( const char* pt )
    : uiODAttribTreeItem( pt )
    , attrnoneitm_(uiStrings::sNone())
    , storeditm_(tr("Stored 2D Data"))
    , steeringitm_(tr("Steering 2D Data"))
    , zattritm_(tr("ZDomain Attrib 2D Data"))
{}


void uiOD2DLineSetAttribItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject( displayID() ))
    if ( !menu || !s2d || istb ) return;

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    Attrib::SelSpec as = *visserv_->getSelSpec( displayID(), attribNr() );
    as.set2DFlag();
    BufferString objnm =
	mFromUiStringTodo(visserv_->getObjectName( displayID() ));

    BufferStringSet datasets;
    seisserv->get2DStoredAttribs( objnm, datasets, 0 );
    const Attrib::DescSet* ads = attrserv->curDescSet(true);
    const Attrib::Desc* desc = ads->getDesc( as.id() );
    const bool isstored = desc && desc->isStored();

    selattrmnuitem_.removeItems();

    bool docheckparent = false;
    storeditm_.removeItems();
    for ( int idx=0; idx<datasets.size(); idx++ )
    {
	FixedString nm = datasets.get(idx).buf();
	MenuItem* item = new MenuItem(toUiString(nm));
	const bool docheck = isstored && nm==as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &storeditm_,item,true,docheck);
    }

    mAddMenuItem( &selattrmnuitem_, &storeditm_, true, storeditm_.checked );

    MenuItem* attrmenu = attrserv->calcAttribMenuItem( as, true, false );
    attrserv->filter2DMenuItems( *attrmenu, as, s2d->getGeomID(), false, 2 );
    mAddMenuItem( &selattrmnuitem_, attrmenu, attrmenu->nrItems(),
		  attrmenu->checked );

    MenuItem* nla = attrserv->nlaAttribMenuItem( as, true, false );
    if ( nla && nla->nrItems() )
	mAddMenuItem( &selattrmnuitem_, nla, true, false );
    // TODO attrserv->filter2DMenuItems( *nla, as, s2d->getGeomID(), false, 0 );

    BufferStringSet steerdatanames;
    seisserv->get2DStoredAttribs( objnm, steerdatanames, 1 );
    docheckparent = false;
    steeringitm_.removeItems();
    for ( int idx=0; idx<steerdatanames.size(); idx++ )
    {
	FixedString nm = steerdatanames.get(idx).buf();
	MenuItem* item = new MenuItem(toUiString(nm));
	const bool docheck = isstored && nm==as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &steeringitm_,item,true,docheck);
    }

    mAddMenuItem( &selattrmnuitem_, &steeringitm_, true, docheckparent );

    zattritm_.removeItems();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))

    if ( scene->getZAxisTransform() )
    {
	zattritm_.enabled = false;
	BufferStringSet zattribnms;
	seisserv->get2DZdomainAttribs( objnm, scene->zDomainKey(), zattribnms );
	if ( zattribnms.size() )
	{
	    mAddMenuItem( &selattrmnuitem_, &zattritm_, true, false );
	    for ( int idx=0; idx<zattribnms.size(); idx++ )
	    {
		FixedString nm = zattribnms.get(idx).buf();
		MenuItem* item = new MenuItem(toUiString(nm));
		const bool docheck = isstored && nm==as.userRef();
		if ( docheck ) docheckparent=true;
		mAddManagedMenuItem( &zattritm_,item,true,docheck);
	    }

	    zattritm_.enabled = true;
	}
    }

    mAddMenuItem( &selattrmnuitem_, &attrnoneitm_, true, false );
}


void uiOD2DLineSetAttribItem::handleMenuCB( CallBacker* cb )
{
    selcomps.erase();
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject( displayID() ));
    if ( !s2d )
	return;

    uiTaskRunner uitr( ODMainWin() );
    Attrib::SelSpec myas;
    bool usemcomp = false;
    BufferString attrnm;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled(true);
	attrnm = storeditm_.findItem(mnuid)->text.getFullString();
	displayStoredData( attrnm, -1, uitr );
    }
    else if ( steeringitm_.itemIndex(mnuid)!=-1 )
    {
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	menu->setIsHandled(true);
	attrnm = steeringitm_.findItem(mnuid)->text.getFullString();
	displayStoredData( attrnm, 1, uitr );
    }
    else if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,myas,usemcomp))
    {
	menu->setIsHandled(true);
	setAttrib( myas, uitr );
    }
    else if ( zattritm_.itemIndex(mnuid)!=-1 )
    {
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	menu->setIsHandled(true);
	attrnm = zattritm_.findItem(mnuid)->text.getFullString();
	displayStoredData( attrnm, -1, uitr );
    }
    else if ( mnuid==attrnoneitm_.id )
    {
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	menu->setIsHandled(true);
	clearAttrib();
    }
}


bool uiOD2DLineSetAttribItem::displayStoredData( const char* attribnm,
						 int component,
						 uiTaskRunner& taskrunner )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject( displayID() ))
    if ( !s2d ) return false;

    BufferString linename( Survey::GM().getName(s2d->getGeomID()) );
    BufferStringSet lnms;
    const SeisIOObjInfo objinfo( attribnm, Seis::Line );
    SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
    objinfo.getLineNames( lnms, opts2d );
    if ( !lnms.isPresent(linename) || !objinfo.ioObj() )
	return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const DBKey dbkey = objinfo.ioObj()->key();
    //First time to ensure all components are available
    Attrib::DescID attribid = attrserv->getStoredID( dbkey, true );

    BufferStringSet complist;
    SeisIOObjInfo::getCompNames( objinfo.ioObj()->key(), complist );
    if ( complist.size()>1 && component<0 )
    {
	if ( ( !selcomps.size() &&
	       !attrserv->handleMultiComp( dbkey, true, false, complist,
					   attribid, selcomps ) )
	     || ( selcomps.size() && !attrserv->prepMultCompSpecs(
		     selcomps, dbkey, true, false ) ) )
	    return false;

	if ( selcomps.size()>1 )
	{
	    const bool needsetattrid = visserv->getSelAttribNr() != attribNr();
	    Attrib::SelSpec mtas( "Multi-Textures",
				Attrib::SelSpec::cOtherAttrib() );
	    if ( needsetattrid )
		visserv->setSelObjectId( displayID(), attribNr() );

	    const bool rescalc = applMgr()->calcMultipleAttribs( mtas );

	    if ( needsetattrid )
		visserv->setSelObjectId( displayID(), -1 );

	    updateColumnText(0);
	    return rescalc;
	}
    }
    else
	attribid = attrserv->getStoredID( dbkey, true, component );

    if ( !attribid.isValid() ) return false;

    TypeSet<Attrib::SelSpec> as = *visserv->getSelSpecs( displayID(), 0 );
    for ( int idx=0; idx<as.size(); idx++ )
    {
	as[idx].set( attribnm, attribid, false, 0 );
	as[idx].set2DFlag();
	const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true, true );
	if ( !ds ) return false;
	as[idx].setRefFromID( *ds );
	const Attrib::Desc* targetdesc = ds->getDesc( attribid );
	if ( !targetdesc ) return false;

	BufferString defstring;
	targetdesc->getDefStr( defstring );
	as[idx].setDefString( defstring );
    }

    attrserv->setTargetSelSpecs( as );
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
    const FixedString zdomainkey = as[0].zDomainKey();
    const bool alreadytransformed = scene && zdomainkey == scene->zDomainKey();
    const DataPack::ID dpid = attrserv->createOutput(
			s2d->getTrcKeyZSampling(alreadytransformed),
			DataPack::ID::get( 0 ) );
    if ( dpid.isInvalid() )
	return false;

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    s2d->setSelSpecs( attribNr(), as );
    applMgr()->useDefColTab( displayID(), attribNr() );
    s2d->setDataPackID( attribNr(), dpid, &taskrunner );
    s2d->showPanel( true );

    updateColumnText(0);
    setChecked( s2d->isOn() );

    return true;
}


void uiOD2DLineSetAttribItem::setAttrib( const Attrib::SelSpec& myas,
					 uiTaskRunner& uitr )
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayID()));
    if ( !s2d ) return;

    applMgr()->attrServer()->setTargetSelSpec( myas );
    const DataPack::ID dpid = applMgr()->attrServer()->createOutput(
					s2d->getTrcKeyZSampling(false),
					DataPack::ID::get(0) );
    if ( dpid.isInvalid() )
	return;

    s2d->setSelSpecs( attribNr(), TypeSet<Attrib::SelSpec>(1,myas) );
    s2d->setDataPackID( attribNr(), dpid, 0 );
    s2d->showPanel( true );

    updateColumnText(0);
    setChecked( s2d->isOn() );
    applMgr()->updateColorTable( displayID(), attribNr() );
}


void uiOD2DLineSetAttribItem::clearAttrib()
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    applMgr()->visServer()->getObject( displayID() ));
    if ( !s2d )
	return;

    s2d->clearTexture( attribNr() );
    updateColumnText(0);
    applMgr()->updateColorTable( displayID(), attribNr() );
}
