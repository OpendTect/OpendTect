/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiodseis2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiattr2dsel.h"
#include "mousecursor.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiodeditattribcolordlg.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uitaskrunner.h"
#include "visseis2ddisplay.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "emmanager.h"
#include "externalattrib.h"
#include "ioman.h"
#include "ioobj.h"
#include "linekey.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "seisioobjinfo.h"
#include "seis2ddata.h"
#include "survinfo.h"
#include "survgeom2d.h"


static const char* sKeyUnselected = "<Unselected>";
static const char* sKeyRightClick = "<right-click>";
static TypeSet<int> selcomps;

uiODLine2DParentTreeItem::uiODLine2DParentTreeItem()
    : uiODTreeItem("2D Line" )
{
}


#define mAdd		0
#define mGridFrom3D	1
#define mFrom3D		2
#define mTo3D		3

#define mDispNames	10
#define mDispPanels	11
#define mDispPolyLines	12
#define mHideNames	13
#define mHidePanels	14
#define mHidePolyLines	15


#define mInsertItm( menu, name, id, enable ) \
{ \
    uiAction* itm = new uiAction( name ); \
    menu->insertItem( itm, id ); \
    itm->setEnabled( enable ); \
}

bool uiODLine2DParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiAction(uiStrings::sAdd(false)), mAdd );
    if ( SI().has3D() )
    {
	mnu.insertItem( new uiAction(tr("&Create 2D Grid From 3D ...")),
			mGridFrom3D );
	mnu.insertItem( new uiAction(tr("&Extract From 3D ...")), mFrom3D );
    }

    mnu.insertItem( new uiAction(tr("&Generate 3D cube ...")), mTo3D );

    if ( !children_.isEmpty() )
    {
	mnu.insertSeparator();
	uiMenu* dispmnu = new uiMenu( getUiParent(), tr("&Display all") );
	mInsertItm( dispmnu, tr("Line names"), mDispNames, true );
	mInsertItm( dispmnu, tr("2D planes"), mDispPanels, true );
	mInsertItm( dispmnu, tr("Line geometry"), mDispPolyLines, true );
	mnu.insertItem( dispmnu );

	uiMenu* hidemnu = new uiMenu( getUiParent(), tr("&Hide all") );
	mInsertItm( hidemnu, tr("Line names"), mHideNames, true );
	mInsertItm( hidemnu, tr("2D planes"), mHidePanels, true );
	mInsertItm( hidemnu, tr("Line geometry"), mHidePolyLines, true );
	mnu.insertItem( hidemnu );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    return mnuid<0 ? false : handleSubMenu( mnuid );
}


bool uiODLine2DParentTreeItem::handleSubMenu( int mnuid )
{
    if ( mnuid == mAdd )
    {
	BufferStringSet linenames;
	TypeSet<Pos::GeomID> geomids;
	applMgr()->seisServer()->select2DLines( linenames, geomids );
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	for ( int idx=linenames.size()-1; idx>=0; idx-- )
	    addChild( new uiOD2DLineTreeItem(linenames.get(idx),geomids[idx]),
		      false );
	cursorchgr.restore();

	if ( linenames.isEmpty() ) return true;

	const bool res = uiMSG().askGoOn(
		"Do you want to select an attribute for these lines?" );
	if ( !res ) return true;

	const Attrib::DescSet* ds = applMgr()->attrServer()->curDescSet(
								    true );
	const NLAModel* nla = applMgr()->attrServer()->getNLAModel( true );
	Pos::GeomID geomid = Survey::GM().getGeomID( linenames.get(0) );
	uiAttr2DSelDlg dlg( ODMainWin(), ds, geomid, nla, sKeyRightClick );
	if ( !dlg.go() ) return false;

	uiTaskRunner uitr( ODMainWin() );
	ObjectSet<uiTreeItem> set;
	findChildren( sKeyRightClick, set );
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
	    if ( attrtype == 1 )
	    {
		const Attrib::Desc* desc =  ds->getDesc(dlg.getSelDescID());
		if ( !desc )
		{
		    uiMSG().error("Selected attribute is not available");
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
    }
    else if ( mnuid == mGridFrom3D )
	ODMainWin()->applMgr().create2DGrid();
    else if ( mnuid == mFrom3D )
	ODMainWin()->applMgr().create2Dfrom3D();
    else if ( mnuid == mTo3D )
	ODMainWin()->applMgr().create3Dfrom2D();
    else if ( mnuid >= mDispNames && mnuid <= mHidePolyLines )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx]);
	    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		ODMainWin()->applMgr().visServer()->getObject(itm->displayID()))
	    if ( !s2d ) continue;

	    switch ( mnuid )
	    {
		case mDispNames: s2d->showLineName( true ); break;
		case mDispPanels: s2d->showPanel( true ); break;
		case mDispPolyLines: s2d->showPolyLine( true ); break;
		case mHideNames: s2d->showLineName( false ); break;
		case mHidePanels: s2d->showPanel( false ); break;
		case mHidePolyLines: s2d->showPolyLine( false ); break;
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem*
    Line2DTreeItemFactory::createForVis( int visid, uiTreeItem* treeitem ) const
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    ODMainWin()->applMgr().visServer()->getObject(visid))
    if ( !s2d || !treeitem ) return 0;

    uiOD2DLineTreeItem* newsubitm =
	new uiOD2DLineTreeItem( s2d->name(), s2d->getGeomID(), visid );

    if ( newsubitm )
       treeitem->addChild( newsubitm,true );

    return 0;
}

/*
uiOD2DLineSetTreeItem::uiOD2DLineSetTreeItem( const MultiID& mid )
    : uiODTreeItem("")
    , setid_( mid )
    , menuhandler_(0)
    , addlinesitm_("&Add line(s) ...")
    , zrgitm_("Set &Z-Range ...")
    , addattritm_("Add A&ttribute")
    , removeattritm_("Re&move Attribute")
    , editattritm_("&Edit Attribute")
    , editcoltabitm_("Edit &Color Settings")
    , showattritm_("Sho&w Attribute")
    , hideattritm_("&Hide Attribute")
    , showitm_("&Show all")
    , hideitm_("&Hide all")
    , showlineitm_("&Lines")
    , hidelineitm_("&Lines")
    , showlblitm_("Line&names")
    , hidelblitm_("Line&names")
    , removeitm_("&Remove")
    , storeditm_("Stored &2D data")
    , steeringitm_("Steer&ing 2D data")
    , expanditm_("Expand all")
    , collapseitm_("Collapse all")
    , curzrg_( Interval<float>().setFrom(SI().zRange(true)) )
{
    storeditm_.checkable = true;
    steeringitm_.checkable = true;
}


uiOD2DLineSetTreeItem::~uiOD2DLineSetTreeItem()
{
    if ( menuhandler_ )
    {
	menuhandler_->createnotifier.remove(
		mCB(this,uiOD2DLineSetTreeItem,createMenuCB) );
	menuhandler_->handlenotifier.remove(
		mCB(this,uiOD2DLineSetTreeItem,handleMenuCB) );
	menuhandler_->unRef();
    }
}


int uiOD2DLineSetTreeItem::uiTreeViewItemType() const
{ return uiTreeViewItem::CheckBox; }


void uiOD2DLineSetTreeItem::checkCB( CallBacker* )
{
    const bool checked = isChecked();
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	const int id = ((uiOD2DLineTreeItem*)children_[idx])->displayID();
	visserv->turnOn( id, checked ? children_[idx]->isChecked() : false );
    }
}


bool uiOD2DLineSetTreeItem::showSubMenu()
{
    if ( !menuhandler_ )
    {
	mDynamicCast(uiMenuHandler*,menuhandler_,
		     ODMainWin()->applMgr().visServer()->getMenuHandler())
	menuhandler_->createnotifier.notify(
		mCB(this,uiOD2DLineSetTreeItem,createMenuCB) );
	menuhandler_->handlenotifier.notify(
		mCB(this,uiOD2DLineSetTreeItem,handleMenuCB) );
	menuhandler_->ref();
    }

    menuhandler_->setMenuID( selectionKey() );
    menuhandler_->executeMenu( uiMenuHandler::fromTree() );
    return true;
}


int uiOD2DLineSetTreeItem::selectionKey() const
{
    if ( children_.size() < 1 )
	return -1;

    mDynamicCastGet(const uiODDisplayTreeItem*,itm,children_[0])
    return itm ? 100000 + itm->displayID() : -1;
}


void uiOD2DLineSetTreeItem::selectAddLines()
{
    BufferStringSet linenames;
    applMgr()->seisServer()->select2DLines( setid_, linenames );

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    for ( int idx=linenames.size()-1; idx>=0; idx-- )
	addChild( new uiOD2DLineTreeItem(linenames.get(idx)), false );
    cursorchgr.restore();

    if ( !linenames.isEmpty() )
	selectNewAttribute( sKeyRightClick );
}


void uiOD2DLineSetTreeItem::createAttrMenu( MenuHandler* menu )
{
    Attrib::SelSpec as;
    applMgr()->attrServer()->resetMenuItems();
    MenuItem* dummy =
	applMgr()->attrServer()->storedAttribMenuItem( as, true, false );
    dummy->removeItems();

    BufferStringSet allstored;
    Attrib::SelInfo::getAttrNames( setid_, allstored );
    allstored.sort();
    storeditm_.createItems( allstored );
    mAddMenuItem( &addattritm_, &storeditm_, storeditm_.nrItems(), false );

    MenuItem* attrmenu = applMgr()->attrServer()->
			    calcAttribMenuItem( as, true, false );
    mAddMenuItem( &addattritm_, attrmenu, attrmenu->nrItems(), false );

    MenuItem* nla = applMgr()->attrServer()->
			    nlaAttribMenuItem( as, true, false );
    if ( nla && nla->nrItems() )
	mAddMenuItem( &addattritm_, nla, true, false )

    BufferStringSet allsteering;
    Attrib::SelInfo::getAttrNames( setid_, allsteering, true );
    allsteering.sort();
    steeringitm_.createItems( allsteering );
    mAddMenuItem( &addattritm_, &steeringitm_, steeringitm_.nrItems(), false );

    mAddMenuItem( menu, &addattritm_, true, false );

    BufferStringSet displayedattribs;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	const int id = ((uiOD2DLineTreeItem*)children_[idx])->displayID();
	const int nrattribs = applMgr()->visServer()->getNrAttribs( id );
	for ( int adx=0; adx<nrattribs; adx++ )
	{
	    const Attrib::SelSpec* ds =
			    applMgr()->visServer()->getSelSpec( id, adx );
	    BufferString attribname = sKeyUnselected;
	    if ( ds && ds->userRef() && *ds->userRef() )
		attribname = ds->userRef();

	    if ( ds && ds->isNLA() )
	    {
		attribname = ds->objectRef();
		const char* nodenm = ds->userRef();
		if ( IOObj::isKey(ds->userRef()) )
		    nodenm = IOM().nameOf( ds->userRef() );
		attribname += " ("; attribname += nodenm; attribname += ")";
	    }

	    displayedattribs.addIfNew( attribname );
	}
    }

    if ( displayedattribs.size() )
    {
	editattritm_.createItems( displayedattribs );
	mAddMenuItem( menu, &editattritm_, true, false );
	showattritm_.createItems( displayedattribs );
	mAddMenuItem( menu, &showattritm_, true, false );
	hideattritm_.createItems( displayedattribs );
	mAddMenuItem( menu, &hideattritm_, true, false );

	const int emptyidx = displayedattribs.indexOf( sKeyUnselected );
	if ( emptyidx<0 || displayedattribs.size()>1 )
	{
	    removeattritm_.createItems( displayedattribs );
	    mAddMenuItem( menu, &removeattritm_, true, false );
	}
	else
	    mResetMenuItem( &removeattritm_ )

	if ( emptyidx >= 0 ) displayedattribs.removeSingle( emptyidx );

	if ( displayedattribs.size() )
	{
	    editcoltabitm_.createItems( displayedattribs );
	    mAddMenuItem( menu, &editcoltabitm_, true, false );
	}
	else
	    mResetMenuItem( &editcoltabitm_ )
    }
    else
    {
	mResetMenuItem( &editattritm_ );
	mResetMenuItem( &showattritm_ );
	mResetMenuItem( &hideattritm_ );
	mResetMenuItem( &removeattritm_ );
	mResetMenuItem( &editcoltabitm_ );
    }
}


void uiOD2DLineSetTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( !menu || menu->menuID() != selectionKey() )
	return;

    mAddMenuItem( menu, &addlinesitm_, true, false );
    if ( children_.size() > 1 )
    {
	mAddMenuItem( menu, &zrgitm_, true, false );

	createAttrMenu( menu );
	mAddMenuItem( menu, &showitm_, true, false );
	mAddMenuItem( &showitm_, &showlineitm_, true, false );
	mAddMenuItem( &showitm_, &showlblitm_, true, false );

	mAddMenuItem( menu, &hideitm_, true, false );
	mAddMenuItem( &hideitm_, &hidelineitm_, true, false );
	mAddMenuItem( &hideitm_, &hidelblitm_, true, false );

	mAddMenuItem( menu, &expanditm_, true, false );
	mAddMenuItem( menu, &collapseitm_, true, false );
    }
    else
    {
	mResetMenuItem( &zrgitm_ );
	mResetMenuItem( &addattritm_ );
	mResetMenuItem( &removeattritm_ );
	mResetMenuItem( &showattritm_ );
	mResetMenuItem( &hideattritm_ );
	mResetMenuItem( &showitm_ );
	mResetMenuItem( &showlineitm_ );
	mResetMenuItem( &showlblitm_ );
	mResetMenuItem( &hideitm_ );
	mResetMenuItem( &hidelineitm_ );
	mResetMenuItem( &hidelblitm_ );
	mResetMenuItem( &expanditm_ );
	mResetMenuItem( &collapseitm_ );
    }

    mAddMenuItem( menu, &removeitm_, true, false );
}


#define mForAllKidsWithBurstCtrl( action ) \
    int lastidx = children_.size()-1; \
    for ( int idx=0; idx<=lastidx; idx++ ) \
    { \
	if ( !idx || idx==lastidx ) \
	    EM::EMM().burstAlertToAll( !idx ); \
	((uiOD2DLineTreeItem*) children_[idx])->action; \
    }

void uiOD2DLineSetTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() || menu->menuID() != selectionKey() )
	return;

    if ( mnuid==addlinesitm_.id )
    {
	menu->setIsHandled(true);
	selectAddLines();
	return;
    }

    uiTaskRunner uitr( ODMainWin() );
    Attrib::SelSpec as;
    bool usemcomp = false;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	selcomps.erase();
	menu->setIsHandled( true );
	const int itmidx = storeditm_.itemIndex( mnuid );
	const BufferString attribnm =
		storeditm_.getItem(itmidx)->text.getFullString();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitem,children_[idx]);
	    lineitem->addStoredData( attribnm, -1, uitr );
	}
    }
    else if ( steeringitm_.itemIndex(mnuid)!=-1 )
    {
	selcomps.erase();
	menu->setIsHandled( true );
	const int itmidx = steeringitm_.itemIndex( mnuid );
	const BufferString attribnm =
		steeringitm_.getItem(itmidx)->text.getFullString();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitem,children_[idx]);
	    lineitem->addStoredData( attribnm, 1, uitr );
	}
    }
    else if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,as,usemcomp) )
    {
	menu->setIsHandled( true );
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitem,children_[idx]);
	    lineitem->addAttrib( as, uitr );
	}
    }
    else if ( editcoltabitm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	const int itmidx = editcoltabitm_.itemIndex( mnuid );
	BufferString attrnm =
		editcoltabitm_.getItem(itmidx)->text.getFullString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	if ( set.size() )
	{
	    uiODEditAttribColorDlg dlg( ODMainWin(), set, attrnm );
	    dlg.go();
	}
    }
    else if ( editattritm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	const int itmidx = editattritm_.itemIndex( mnuid );
	BufferString curnm = editattritm_.getItem(itmidx)->text.getFullString();
	const char* attribnm = curnm==sKeyUnselected ? sKeyRightClick
						     : curnm.buf();
	selectNewAttribute( attribnm );
    }
    else if ( removeattritm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	const int itmidx = removeattritm_.itemIndex( mnuid );
	const BufferString attribnm =
		removeattritm_.getItem(itmidx)->text.getFullString();
	for ( int idx=0; idx<children_.size(); idx++ )
	    ((uiOD2DLineTreeItem*)children_[idx])->removeAttrib( attribnm );
    }
    else if ( showattritm_.itemIndex(mnuid)!=-1 ||
	      hideattritm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	const bool show = showattritm_.itemIndex(mnuid)!=-1;
	MenuItem& attritm = show ? showattritm_ : hideattritm_;
	const int itmidx = attritm.itemIndex( mnuid );
	BufferString curnm = attritm.getItem(itmidx)->text.getFullString();
	const char* attribnm = curnm==sKeyUnselected ? sKeyRightClick
						     : curnm.buf();
	ObjectSet<uiTreeItem> itms; findChildren( attribnm, itms );
	for ( int idx=0; idx<itms.size(); idx++ )
	    itms[idx]->setChecked( show, true );
    }
    else if ( mnuid==removeitm_.id )
    {
	menu->setIsHandled( true );
	if ( children_.size()>0 &&
	     !uiMSG().askRemove("All lines will be removed from the tree."
			      "\nDo you want to continue?") )
	    return;

	prepareForShutdown();
	while ( children_.size() )
	{
	    uiOD2DLineTreeItem* itm = (uiOD2DLineTreeItem*)children_[0];
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
	parent_->removeChild( this );
    }
    else if ( mnuid==showlineitm_.id || mnuid==hidelineitm_.id )
    {
	menu->setIsHandled( true );
	const bool turnon = mnuid==showlineitm_.id;
	mForAllKidsWithBurstCtrl( setChecked(turnon,true) );
    }
    else if ( mnuid==showlblitm_.id || mnuid==hidelblitm_.id )
    {
	menu->setIsHandled( true );
	const bool turnon = mnuid==showlblitm_.id;
	for ( int idx=0; idx<children_.size(); idx++ )
	    ((uiOD2DLineTreeItem*)children_[idx])->showLineName( turnon );
    }
    else if ( mnuid == zrgitm_.id )
    {
	menu->setIsHandled( true );

	BufferString lbl( "Z-Range " ); lbl += SI().getZUnitString();
	Interval<int> intzrg(mNINT32(curzrg_.start*SI().zDomain().userFactor()),
			    mNINT32(curzrg_.stop*SI().zDomain().userFactor()));
	uiGenInputDlg dlg( getUiParent(), "Specify 2D line Z-Range", lbl,
			   new IntInpIntervalSpec(intzrg) );
	if ( !dlg.go() ) return;

	intzrg = dlg.getFld()->getIInterval();
	curzrg_.start = float(intzrg.start) / SI().zDomain().userFactor();
	curzrg_.stop = float(intzrg.stop) / SI().zDomain().userFactor();
	mForAllKidsWithBurstCtrl( setZRange(curzrg_) );
    }
    else if ( mnuid == expanditm_.id )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	    children_[idx]->expand();
    }
    else if ( mnuid == collapseitm_.id )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	    children_[idx]->collapse();
    }
}


void uiOD2DLineSetTreeItem::selectNewAttribute( const char* attribnm )
{
    const Attrib::DescSet* ds = applMgr()->attrServer()->curDescSet( true );
    const NLAModel* nla = applMgr()->attrServer()->getNLAModel( true );
    uiAttr2DSelDlg dlg( ODMainWin(), ds, setid_, nla, attribnm );
    if ( !dlg.go() ) return;

    uiTaskRunner uitr( ODMainWin() );
    ObjectSet<uiTreeItem> set;
    findChildren( attribnm, set );
    const int attrtype = dlg.getSelType();
    if ( attrtype == 0 )
    {
	const char* newattrnm = dlg.getStoredAttrName();
	for ( int idx=0; idx<set.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineSetAttribItem*,item,set[idx])
	    if ( item ) item->displayStoredData( newattrnm, -1, uitr );
	}
    }
    else if ( attrtype == 1 || attrtype == 2 )
    {
	Attrib::SelSpec as;
	if ( attrtype == 1 )
	{
	    const Attrib::Desc* desc =  ds->getDesc( dlg.getSelDescID() );
	    if ( !desc )
	    {
		uiMSG().error("Selected attribute is not available");
		return;
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
}


const char* uiOD2DLineSetTreeItem::parentType() const
{ return typeid(uiODLine2DParentTreeItem).name(); }


bool uiOD2DLineSetTreeItem::init()
{
    applMgr()->seisServer()->get2DLineSetName( setid_, name_ );
    checkStatusChange()->notify( mCB(this,uiOD2DLineSetTreeItem,checkCB) );
    return true;
}
*/

uiOD2DLineTreeItem::uiOD2DLineTreeItem( const char* nm, Pos::GeomID geomid,
					int displayid )
    : linenmitm_("Show linename")
    , panelitm_("Show 2D plane")
    , polylineitm_("Show line geometry")
    , positionitm_("Position ...")
    , geomid_(geomid)
{
    name_ = nm;
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
    s2d->setName( geom2d->getName() );
    //If restore, we use the old display range after set the geometry.
    const Interval<int> oldtrcnrrg = s2d->getTraceNrRange();
    const Interval<float> oldzrg = s2d->getZRange( true );
    if ( newdisplay && (geom2d->data().positions().size() > 300000000 ||
			geom2d->data().zRange().nrSteps() > 299999999) )
    {
	BufferString msg = "Either trace size or z size is beyond max display";
	msg += " size of 3 X 10 e8. You can right click the line name to ";
	msg += "change position range to view part of the data.";
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


BufferString uiOD2DLineTreeItem::createDisplayName() const
{
    return BufferString( visserv_->getObjectName(displayid_) );
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
    if ( !menu || menu->menuID() != displayID() || !s2d || istb ) return;

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

	CubeSampling maxcs;
	assign( maxcs.zrg, s2d->getMaxZRange(true)  );
	maxcs.hrg.start.crl() = s2d->getMaxTraceNrRange().start;
	maxcs.hrg.stop.crl() = s2d->getMaxTraceNrRange().stop;

	CubeSampling curcs;
	curcs.zrg.setFrom( s2d->getZRange(true) );
	curcs.hrg.start.crl() = s2d->getTraceNrRange().start;
	curcs.hrg.stop.crl() = s2d->getTraceNrRange().stop;

	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	CallBack dummy;
	uiSliceSelDlg positiondlg( getUiParent(), curcs,
				   maxcs, dummy, uiSliceSel::TwoD,
				   scene->zDomainInfo() );
	if ( !positiondlg.go() ) return;
	const CubeSampling newcs = positiondlg.getCubeSampling();

	const Interval<float> newzrg( newcs.zrg.start, newcs.zrg.stop );
	if ( !newzrg.isEqual(s2d->getZRange(true),mDefEps) )
	{
	    s2d->annotateNextUpdateStage( true );
	    s2d->setZRange( newzrg );
	}

	const Interval<int> ntrcnrrg(
	    newcs.hrg.start.crl(), newcs.hrg.stop.crl() );
	if ( ntrcnrrg != s2d->getTraceNrRange() )
	{
	    if ( !s2d->getUpdateStageNr() )
		s2d->annotateNextUpdateStage( true );

	    s2d->setTraceNrRange( ntrcnrrg );
	}

	if ( s2d->getUpdateStageNr() )
	{
	    s2d->annotateNextUpdateStage( true );
	    for ( int idx=s2d->nrAttribs()-1; idx>=0; idx-- )
	    {
		if ( s2d->getSelSpec(idx)
		  && s2d->getSelSpec(idx)->id().isValid() )
		    visserv_->calculateAttrib( displayid_, idx, false );
	    }
	    s2d->annotateNextUpdateStage( false );
	}
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

    CubeSampling cs;
    cs.hrg.start.inl() = cs.hrg.stop.inl() = 0;
    cs.hrg.step.inl() = 1;
    cs.hrg.start.crl() = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl() = s2d->getTraceNrRange().stop;
    cs.hrg.step.crl() = 1;
    cs.zrg.setFrom( s2d->getZRange(false) );

    Attrib::SelSpec as = *s2d->getSelSpec( attribnr );
    as.set2DFlag();

    uiTaskRunner uitr( ODMainWin() );
    DataPack::ID dpid = -1;
    LineKey lk( s2d->name() );
    if ( as.id().asInt() == Attrib::SelSpec::cOtherAttrib().asInt() )
    {
	PtrMan<Attrib::ExtAttribCalc> calc =
	    Attrib::ExtAttrFact().create( 0, as, false );
	if ( !calc )
	{
	    uiMSG().error( "Attribute cannot be created" );
	    return;
	}

	dpid = calc->createAttrib( cs, lk, &uitr );
    }
    else
    {
	const char* ptr = firstOcc( as.userRef(), '|' );
	if ( ptr ) // only to set correct userref in selspec
	{
	    LineKey lkusrref( as.userRef() );
	    as.setUserRef( lkusrref.attrName() );
	    s2d->setSelSpec( attribnr, as );
	}

	if ( lk.attrName() == LineKey::sKeyDefAttrib() &&
	     !FixedString(as.userRef()).isEmpty() )
	    lk.setAttrName( as.userRef() );

	applMgr()->attrServer()->setTargetSelSpec( as );
	dpid = applMgr()->attrServer()->create2DOutput( cs, lk, uitr );
    }

    if ( dpid < 0 )
	return;
    s2d->setDataPackID( attribnr, dpid, 0 );
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
    for ( int idx=s2d->nrAttribs()-1; idx>=0; idx-- )
    {
	if ( s2d->getSelSpec(idx) && s2d->getSelSpec(idx)->id().isValid() )
	    visserv_->calculateAttrib( displayid_, idx, false );
    }
    s2d->annotateNextUpdateStage( false );
}


void uiOD2DLineTreeItem::removeAttrib( const char* attribnm )
{
    BufferString itemnm = attribnm;
    if ( itemnm == sKeyUnselected )
	itemnm = sKeyRightClick;

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
	if ( !dataitem || itemnm!=dataitem->name() ) continue;

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
    , attrnoneitm_("&None")
    , storeditm_("Stored &2D data")
    , steeringitm_("Steer&ing 2D data")
    , zattritm_("ZDomain Atrrib 2D data")
{}


void uiOD2DLineSetAttribItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );
    const uiVisPartServer* visserv_ = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject( displayID() ))
    if ( !menu || !s2d || istb ) return;

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    Attrib::SelSpec as = *visserv_->getSelSpec( displayID(), attribNr() );
    as.set2DFlag();
    const char* objnm = visserv_->getObjectName( displayID() );

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
	MenuItem* item = new MenuItem(nm);
	const bool docheck = isstored && nm==as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &storeditm_,item,true,docheck);
    }

    mAddMenuItem( &selattrmnuitem_, &storeditm_, true, docheckparent );

    MenuItem* attrmenu = attrserv->calcAttribMenuItem( as, true, false );
    mAddMenuItem( &selattrmnuitem_, attrmenu, attrmenu->nrItems(), false );

    MenuItem* nla = attrserv->nlaAttribMenuItem( as, true, false );
    if ( nla && nla->nrItems() )
	mAddMenuItem( &selattrmnuitem_, nla, true, false );

    BufferStringSet steerdatanames;
    seisserv->get2DStoredAttribs( objnm, steerdatanames, 1 );
    docheckparent = false;
    steeringitm_.removeItems();
    for ( int idx=0; idx<steerdatanames.size(); idx++ )
    {
	FixedString nm = steerdatanames.get(idx).buf();
	MenuItem* item = new MenuItem(nm);
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
		MenuItem* item = new MenuItem(nm);
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

    const uiVisPartServer* visserv_ = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject( displayID() ));
    if ( !s2d )
	return;

    uiTaskRunner uitr( ODMainWin() );
    Attrib::SelSpec myas;
    bool usemcomp = false;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled(true);
	displayStoredData(
		storeditm_.findItem(mnuid)->text.getFullString(), -1, uitr );
    }
    else if ( steeringitm_.itemIndex(mnuid)!=-1 )
    {
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	menu->setIsHandled(true);
	displayStoredData(
	    steeringitm_.findItem(mnuid)->text.getFullString(), 1, uitr );
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
	displayStoredData(
	    zattritm_.findItem(mnuid)->text.getFullString(), -1, uitr );
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
    SeisIOObjInfo objinfo( attribnm );
    SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
    objinfo.getLineNames( lnms, opts2d );
    if ( !lnms.isPresent(linename) || !objinfo.ioObj() )
	return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    LineKey lk( objinfo.ioObj()->key() );
    //First time to ensure all components are available
    Attrib::DescID attribid = attrserv->getStoredID( lk, true );

    BufferStringSet complist;
    SeisIOObjInfo::getCompNames( objinfo.ioObj()->key(), complist );
    if ( complist.size()>1 && component<0 )
    {
	if ( ( !selcomps.size() &&
	       !attrserv->handleMultiComp( lk, true, false, complist,
					   attribid, selcomps ) )
	     || ( selcomps.size() &&
		  !attrserv->prepMultCompSpecs( selcomps, lk, true, false ) ) )
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
	attribid = attrserv->getStoredID( lk, true, component );

    if ( !attribid.isValid() ) return false;

    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), 0 );
    Attrib::SelSpec myas( *as );
    LineKey linekey( s2d->name() );
    myas.set( attribnm, attribid, false, 0 );
    myas.set2DFlag();
    const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true, true );
    if ( !ds ) return false;
    myas.setRefFromID( *ds );
    myas.setUserRef( attribnm ); // Why is this necessary?
    const Attrib::Desc* targetdesc = ds->getDesc( attribid );
    if ( !targetdesc ) return false;

    BufferString defstring;
    targetdesc->getDefStr( defstring );
    myas.setDefString( defstring );
    attrserv->setTargetSelSpec( myas );

    CubeSampling cs;
    cs.hrg.start.crl() = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl() = s2d->getTraceNrRange().stop;

    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
    const FixedString zdomainkey = myas.zDomainKey();
    const bool alreadytransformed = scene && zdomainkey == scene->zDomainKey();
    cs.zrg.setFrom( s2d->getZRange(alreadytransformed) );

    const DataPack::ID dpid =
	applMgr()->attrServer()->create2DOutput( cs, linekey, taskrunner );
    if ( dpid < 0 )
	return false;

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    s2d->setSelSpec( attribNr(), myas );
    applMgr()->useDefColTab( displayID(), attribNr() );
    s2d->setDataPackID( attribNr(), dpid, 0 );
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
		    visserv->getObject(displayID()))

    CubeSampling cs;
    cs.hrg.start.crl() = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl() = s2d->getTraceNrRange().stop;
    cs.zrg.setFrom( s2d->getZRange(false) );

    LineKey lk( s2d->name() );
    applMgr()->attrServer()->setTargetSelSpec( myas );

    const DataPack::ID dpid =
	applMgr()->attrServer()->create2DOutput( cs, lk, uitr );
    if ( dpid < 0 )
	return;

    s2d->setSelSpec( attribNr(), myas );
    s2d->setDataPackID( attribNr(), dpid, 0 );

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
