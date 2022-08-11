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
#include "uistrings.h"
#include "uitreeview.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "visrgbatexturechannel2rgba.h"
#include "visseis2ddisplay.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescid.h"
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
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "seisparallelreader.h"
#include "seispreload.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "survinfo.h"
#include "survgeom2d.h"

static TypeSet<int> selcomps;

const char* uiODLine2DParentTreeItem::sKeyRightClick()
{ return "<right-click>"; }
const char* uiODLine2DParentTreeItem::sKeyUnselected()
{ return "<Unselected>"; }
static const char* sKeySelecting()
{ return "<Selecting>"; }


CNotifier<uiODLine2DParentTreeItem,uiMenu*>&
	uiODLine2DParentTreeItem::showMenuNotifier()
{
    static CNotifier<uiODLine2DParentTreeItem,uiMenu*> notif( nullptr );
    return notif;
}


uiODLine2DParentTreeItem::uiODLine2DParentTreeItem()
    : uiODParentTreeItem( tr("2D Line") )
    , replaceattritm_(nullptr)
    , removeattritm_(nullptr)
    , dispattritm_(nullptr)
    , hideattritm_(nullptr)
    , editcoltabitm_(nullptr)
{
}


uiODLine2DParentTreeItem::~uiODLine2DParentTreeItem()
{}


const char* uiODLine2DParentTreeItem::iconName() const
{ return "tree-geom2d"; }


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

#define mAddAttr	20


#define mInsertItm( menu, name, id, enable ) \
{ \
    uiAction* itm = new uiAction( name ); \
    menu->insertAction( itm, id ); \
    itm->setEnabled( enable ); \
}

#define mInsertAttrBasedItem( attritm, txt ) \
    attritm = new uiMenu( getUiParent(), tr(txt) ); \
    mnu.addMenu( attritm ); \
    for ( int idx=0; idx<displayedattribs.size(); idx++ ) \
    attritm->insertAction( \
	    new uiAction(mToUiStringTodo(displayedattribs.get(idx))), \
			 varmenuid++ );

bool uiODLine2DParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), mAdd );
    if ( SI().has3D() )
    {
	mnu.insertAction( new uiAction(m3Dots(tr("Create 2D Grid from 3D"))),
			mGridFrom3D );
	mnu.insertAction( new uiAction(m3Dots(tr("Extract from 3D"))), mFrom3D);
    }

#ifdef __debug__
    mnu.insertAction( new uiAction(m3Dots(tr("Generate 3D Cube"))), mTo3D );
#endif

    BufferStringSet displayedattribs;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	const VisID id = ((uiOD2DLineTreeItem*)children_[idx])->displayID();
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
		const char* nodenm = ds->userRef();
		if ( IOObj::isKey(ds->userRef()) )
		    nodenm = IOM().nameOf( ds->userRef() );
		attribname += " ("; attribname += nodenm; attribname += ")";
	    }

	    displayedattribs.addIfNew( attribname );
	}
    }

    int varmenuid = 1000;
    if ( !children_.isEmpty() )
    {
	mnu.insertSeparator();
	mnu.insertAction( new uiAction(tr("Add Attribute")), mAddAttr );
	if ( !displayedattribs.isEmpty() )
	{
	    mInsertAttrBasedItem( replaceattritm_, "Replace Attribute" )
	    if ( displayedattribs.size()>1 )
	    { mInsertAttrBasedItem( removeattritm_, "Remove Attribute" ) }

	    const int emptyidx = displayedattribs.indexOf( sKeyUnselected() );
	    if ( emptyidx >= 0 ) displayedattribs.removeSingle( emptyidx );
	    if ( displayedattribs.size() )
	    {
		mInsertAttrBasedItem( dispattritm_, "Display Attribute" )
		mInsertAttrBasedItem( hideattritm_, "Hide Attribute" )
		mInsertAttrBasedItem( editcoltabitm_, "Edit Color Settings" )
	    }
	}

	mnu.insertSeparator();
	uiMenu* dispmnu = new uiMenu( getUiParent(), tr("Display All") );
	mInsertItm( dispmnu, uiStrings::sLineName(mPlural), mDispNames, true )
	mInsertItm( dispmnu, tr("2D Planes"), mDispPanels, true )
	mInsertItm( dispmnu, tr("Line Geometry"), mDispPolyLines, true )
	mnu.addMenu( dispmnu );

	uiMenu* hidemnu = new uiMenu( getUiParent(), tr("Hide All") );
	mInsertItm( hidemnu, uiStrings::sLineName(mPlural), mHideNames, true )
	mInsertItm( hidemnu, tr("2D Planes"), mHidePanels, true )
	mInsertItm( hidemnu, tr("Line Geometry"), mHidePolyLines, true )
	mnu.addMenu( hidemnu );
    }

    showMenuNotifier().trigger( &mnu, this );

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    const bool ret = mnuid<0 ? false : handleSubMenu( mnuid );
    replaceattritm_ = removeattritm_ = dispattritm_ = hideattritm_
		    = editcoltabitm_ = nullptr;
    return ret;
}


void uiODLine2DParentTreeItem::setTopAttribName( const char* nm )
{
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx])
	if ( itm->nrChildren() > 0 )
	    itm->getChild(itm->nrChildren()-1)->setName( toUiString(nm) );
    }
}


bool uiODLine2DParentTreeItem::handleSubMenu( int mnuid )
{
    TypeSet<Pos::GeomID> displayedgeomids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx])
	mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    ODMainWin()->applMgr().visServer()->getObject(itm->displayID()))
	if ( !s2d ) continue;

	displayedgeomids += s2d->getGeomID();
    }

    if ( mnuid == mAdd )
    {
	int action = 0;
	TypeSet<Pos::GeomID> geomids;
	applMgr()->seisServer()->select2DLines( geomids, action );
	const bool rgba = action==3;

	for ( int idx=geomids.size()-1; idx>=0; idx-- )
	{
	    setMoreObjectsToDoHint( idx>0 );
	    addChild( new uiOD2DLineTreeItem(geomids[idx],VisID::udf(),rgba),
		      false );
	}

	if ( action==0 || geomids.isEmpty() ) return true;

	if ( action==1 )
	    loadDefaultData();
	else if ( action==2 )
	    selectLoadAttribute( geomids );
    }
    else if ( mnuid == mGridFrom3D )
	ODMainWin()->applMgr().create2DGrid();
    else if ( mnuid == mFrom3D )
	ODMainWin()->applMgr().create2DFrom3D();
    else if ( mnuid == mTo3D )
	ODMainWin()->applMgr().create3DFrom2D();
    else if ( mnuid == mAddAttr )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx])
	    const StringView topattrnm = itm->nrChildren()<=0 ? "" :
		itm->getChild(itm->nrChildren()-1)->name().getOriginalString();
	    if ( topattrnm != sKeyRightClick() )
		itm->addAttribItem();
	}

	setTopAttribName( sKeySelecting() );
	if ( !selectLoadAttribute(displayedgeomids,sKeySelecting()) )
	    setTopAttribName( sKeyRightClick() );
    }
    else if ( replaceattritm_ && replaceattritm_->findAction(mnuid) )
    {
	const uiAction* itm = replaceattritm_->findAction( mnuid );
	StringView attrnm = itm->text().getOriginalString();
	if ( attrnm == sKeyUnselected() ) attrnm = sKeyRightClick();
	selectLoadAttribute( displayedgeomids, attrnm );
    }
    else if ( removeattritm_ && removeattritm_->findAction(mnuid) )
    {
	const uiAction* itm = removeattritm_->findAction( mnuid );
	StringView attrnm = itm->text().getOriginalString();
	if ( attrnm == sKeyUnselected() ) attrnm = sKeyRightClick();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitm,children_[idx])
	    if ( lineitm ) lineitm->removeAttrib( attrnm );
	}
    }
    else if ( ( dispattritm_ && dispattritm_->findAction(mnuid) ) ||
	      ( hideattritm_ && hideattritm_->findAction(mnuid) ) )
    {
	const uiAction* itm = dispattritm_->findAction( mnuid );
	const bool disp = itm;
	if ( !itm ) itm = hideattritm_->findAction( mnuid );
	const StringView attrnm = itm->text().getOriginalString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	for ( int idx=0; idx<set.size(); idx++ )
	    set[idx]->setChecked( disp, true );
    }
    else if ( editcoltabitm_ && editcoltabitm_->findAction(mnuid) )
    {
	const uiAction* itm = editcoltabitm_->findAction( mnuid );
	const StringView attrnm = itm->text().getOriginalString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	if ( set.size() )
	{
	    uiODEditAttribColorDlg dlg( ODMainWin(), set, attrnm );
	    dlg.go();
	}
    }
    else if ( mnuid >= mDispNames && mnuid <= mHidePolyLines )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx])
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
	selcomps.erase();
	for ( int idx=0; idx<set.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineSetAttribItem*,item,set[idx])
	    if ( item ) item->displayStoredData( attrnm, 0, uitr );
	}
    }

    return true;
}


bool uiODLine2DParentTreeItem::selectLoadAttribute(
    const TypeSet<Pos::GeomID>& geomids, const char* curattrnm, int attridx )
{
    const Attrib::DescSet* ds =
	applMgr()->attrServer()->curDescSet( true );
    const NLAModel* nla = applMgr()->attrServer()->getNLAModel( true );
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
    ZDomain::Info info = scene->zDomainInfo();

    uiAttr2DSelDlg dlg( ODMainWin(), ds, geomids, nla, info, curattrnm );

    uiTaskRunner uitr( ODMainWin() );
    ObjectSet<uiTreeItem> set;
    findChildren( curattrnm, set );

    if ( attridx >= 0 )
    {
	const uiODDataTreeItem* dataitm0 = nullptr;
	for ( int idx=set.size()-1; idx>=0; idx-- )
	{
	    mDynamicCastGet(const uiODDataTreeItem*,itm,set[idx])
	    if ( itm && attridx==itm->attribNr() )
		dataitm0 = itm;
	    else
		set.removeSingle( idx );
	}

	if ( dataitm0 )
	{
	    uiString attribposname;
	    applMgr()->visServer()->getAttribPosName( dataitm0->displayID(),
						      attridx, attribposname );
	    dlg.setCaption( uiStrings::phrSelect(attribposname) );
	}
    }

    if ( !dlg.go() ) return false;

    const int attrtype = dlg.getSelType();
    if ( attrtype == 0 || attrtype == 1 )
    {
	selcomps.erase();
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
	    as.set( nullptr, Attrib::DescID(dlg.getOutputNr(), false),
		    true, "" );
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
uiTreeItem* Line2DTreeItemFactory::createForVis( VisID visid,
						 uiTreeItem* treeitem ) const
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    ODMainWin()->applMgr().visServer()->getObject(visid))
    if ( !s2d || !treeitem )
	    return nullptr;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
		    s2d->getChannels2RGBA())

    uiOD2DLineTreeItem* newsubitm =
	new uiOD2DLineTreeItem( s2d->getGeomID(), visid, rgba );

    if ( newsubitm )
       treeitem->addChild( newsubitm,true );

    return nullptr;
}


uiOD2DLineTreeItem::uiOD2DLineTreeItem( Pos::GeomID geomid, VisID displayid,
					bool rgba )
    : geomid_(geomid)
    , linenmitm_(tr("Show Linename"))
    , panelitm_(tr("Show 2D Plane"))
    , polylineitm_(tr("Show Line Geometry"))
    , positionitm_(m3Dots(tr("Position")))
    , rgba_( rgba )
{
    name_ = mToUiStringTodo(Survey::GM().getName( geomid ));
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
    if ( !displayid_.isValid() )
    {
	mDynamicCastGet(uiODLine2DParentTreeItem*,parentitm,parent_)
	if ( !parentitm ) return false;

	visSurvey::Seis2DDisplay* s2d = new visSurvey::Seis2DDisplay;
	visserv_->addObject( s2d, sceneID(), true );
	displayid_ = s2d->id();

	s2d->turnOn( true );
	newdisplay = true;

	if ( rgba_ )
	{
	    s2d->setChannels2RGBA( visBase::RGBATextureChannel2RGBA::create() );
	    s2d->addAttrib();
	    s2d->addAttrib();
	    s2d->addAttrib();
	}
    }

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return false;

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
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
	const bool hastransform = s2d->getZAxisTransform();
	if ( hasworkzrg && !hastransform )
	{
	    StepInterval<float> newzrg = geom2d->data().zRange();
	    newzrg.limitTo( SI().zRange(true) );
	    s2d->setZRange( newzrg );
	}
    }

    if ( applMgr() )
	applMgr()->getOtherFormatData.notify(
	    mCB(this,uiOD2DLineTreeItem,getNewData) );

    if ( rgba_ )
	selectRGBA( geomid_ );

    return uiODDisplayTreeItem::init();
}


uiString uiOD2DLineTreeItem::createDisplayName() const
{
    return visserv_->getUiObjectName(displayid_);
}


uiODDataTreeItem* uiOD2DLineTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* prnttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiOD2DLineSetAttribItem::factory().create(nullptr,*as,prnttype,false)
	: nullptr;

    if ( !res )
	res = new uiOD2DLineSetAttribItem( prnttype );
    return res;
}


void uiOD2DLineTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !menu || isDisplayID(menu->menuID()) || !s2d ) return;

    mAddMenuOrTBItem( istb, nullptr, &displaymnuitem_, &linenmitm_,
		      true, s2d->isLineNameShown() )
    mAddMenuOrTBItem( istb, nullptr, &displaymnuitem_, &panelitm_,
		      true, s2d->isPanelShown() )
    mAddMenuOrTBItem( istb, nullptr, &displaymnuitem_, &polylineitm_,
		      true, s2d->isPolyLineShown() )
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionitm_, true, false)
}


void uiOD2DLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( !menu || menu->isHandled() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d || !isDisplayID(menu->menuID()) )
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
	if ( !newzrg.isEqual(s2d->getZRange(true),mDefEpsF) )
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


bool uiOD2DLineTreeItem::displayDefaultData()
{
    if ( children_.isEmpty() )
	return false;

    mDynamicCastGet(uiOD2DLineSetAttribItem*,item,children_.first())
    if ( !item )
	return false;

    Attrib::DescID descid;
    if ( !applMgr()->getDefaultDescID(descid,true) )
	return false;

    const Attrib::DescSet* ads =
	Attrib::DSHolder().getDescSet( true, true );
    const Attrib::Desc* desc = ads->getDesc( descid );
    if ( !desc )
	return false;

    selcomps.erase();
    const char* attrnm = desc->userRef();
    uiTaskRunner uitr( ODMainWin() );
    return item->displayStoredData( attrnm, 0, uitr );
}


bool uiOD2DLineTreeItem::addStoredData( const char* nm, int component,
					  uiTaskRunner& uitr )
{
    addAttribItem();
    const int lastattridx = children_.size() - 1;
    if ( lastattridx < 0 )
	return false;

    mDynamicCastGet(uiOD2DLineSetAttribItem*,lsai,children_[lastattridx])
    if ( !lsai )
	return false;

    selcomps.erase();
    return lsai->displayStoredData( nm, component, uitr );
}


void uiOD2DLineTreeItem::addAttrib( const Attrib::SelSpec& myas,
				      uiTaskRunner& uitr )
{
    addAttribItem();
    const int lastattridx = children_.size() - 1;
    if ( lastattridx < 0 )
	return;

    mDynamicCastGet(uiOD2DLineSetAttribItem*,lsai,children_[lastattridx])
    if ( !lsai )
	return;

    lsai->setAttrib( myas, uitr );
}


void uiOD2DLineTreeItem::getNewData( CallBacker* )
{
    const VisID visid = applMgr()->otherFormatVisID();
    if ( visid != displayid_ )
	return;

    const int attribnr = applMgr()->otherFormatAttrib();

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return;

    const TrcKeyZSampling tkzs = s2d->getTrcKeyZSampling( false );
    const TypeSet<Attrib::SelSpec>& as = *s2d->getSelSpecs( attribnr );

    DataPack::ID dpid = DataPack::cNoID();
    if ( as[0].id().asInt() == Attrib::SelSpec::cOtherAttrib().asInt() )
    {
	PtrMan<Attrib::ExtAttribCalc> calc =
	    Attrib::ExtAttrFact().create( nullptr, as[0], false );
	if ( !calc )
	{
	    uiMSG().error( tr("Attribute cannot be created") );
	    return;
	}

	uiTaskRunner uitr( ODMainWin() );
	const LineKey lk( s2d->name() );
	dpid = calc->createAttrib( tkzs, lk, &uitr );
    }
    else
    {
	applMgr()->attrServer()->setTargetSelSpecs( as );
	dpid = applMgr()->attrServer()->createOutput( tkzs, DataPack::cNoID() );
    }

    if ( dpid == DataPack::cNoID() )
	return;

    s2d->setDataPackID( attribnr, dpid, nullptr );
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
	mDynamicCastGet(uiOD2DLineSetAttribItem*,item,children_[idx])
	if ( item ) nrattribitms++;
    }

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,dataitem,children_[idx])
	mDynamicCastGet(uiOD2DLineSetAttribItem*,attribitem,children_[idx])
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
    , storeditm_(tr("Stored 2D Data"))
    , steeringitm_(tr("Steering 2D Data"))
    , zattritm_(tr("ZDomain Attrib 2D Data"))
    , attrnoneitm_(uiStrings::sNone())
{}


uiOD2DLineSetAttribItem::~uiOD2DLineSetAttribItem()
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
    const uiString uiobjnm = visserv_->getUiObjectName( displayID() );
    const BufferString objnm = uiobjnm.getFullString();

    BufferStringSet datasets;
    seisserv->get2DStoredAttribs( objnm, datasets, 0 );
    const Attrib::DescSet* ads = attrserv->curDescSet(true);
    const Attrib::Desc* desc = ads->getDesc( as.id() );
    const bool isstored = desc && desc->isStored();

    selattrmnuitem_.removeItems();

    const Pos::GeomID geomid = s2d->getGeomID();
    bool docheckparent = false;
    storeditm_.removeItems();
    for ( int idx=0; idx<datasets.size(); idx++ )
    {
	const StringView nm = datasets.get(idx).buf();
	MenuItem* item = new MenuItem( toUiString(nm) );
	const SeisIOObjInfo si( nm, Seis::Line );
	const MultiID mid = si.ioObj() ? si.ioObj()->key() : MultiID::udf();
	if ( Seis::PLDM().isPresent(mid,geomid) )
	    item->iconfnm = "preloaded";
	const bool docheck = isstored && nm==as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &storeditm_,item,true,docheck)
    }

    mAddMenuItem( &selattrmnuitem_, &storeditm_, true, docheckparent )

    MenuItem* attrmenu = attrserv->calcAttribMenuItem( as, true, false );
    attrserv->filter2DMenuItems( *attrmenu, as, geomid, false, 2 );
    mAddMenuItem( &selattrmnuitem_, attrmenu, attrmenu->nrItems(),
		  attrmenu->checked )

    MenuItem* nla = attrserv->nlaAttribMenuItem( as, true, false );
    if ( nla && nla->nrItems() )
	mAddMenuItem( &selattrmnuitem_, nla, true, false )
    // TODO attrserv->filter2DMenuItems( *nla, as, s2d->getGeomID(), false, 0 );

    BufferStringSet steerdatanames;
    seisserv->get2DStoredAttribs( objnm, steerdatanames, 1 );
    docheckparent = false;
    steeringitm_.removeItems();
    for ( int idx=0; idx<steerdatanames.size(); idx++ )
    {
	StringView nm = steerdatanames.get(idx).buf();
	MenuItem* item = new MenuItem(mToUiStringTodo(nm));
	const bool docheck = isstored && nm==as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &steeringitm_,item,true,docheck)
    }

    mAddMenuItem( &selattrmnuitem_, &steeringitm_, true, docheckparent )

    zattritm_.removeItems();
    mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))

    if ( scene->getZAxisTransform() )
    {
	zattritm_.enabled = false;
	zattritm_.text = toUiString("%1 %2").arg(scene->zDomainKey())
					    .arg(uiStrings::sData());
	BufferStringSet zattribnms;
	seisserv->get2DZdomainAttribs( objnm, scene->zDomainKey(), zattribnms );
	if ( zattribnms.size() )
	{
	    mAddMenuItem( &selattrmnuitem_, &zattritm_, true, false )
	    for ( int idx=0; idx<zattribnms.size(); idx++ )
	    {
		StringView nm = zattribnms.get(idx).buf();
		MenuItem* item = new MenuItem( toUiString(nm) );
		const bool docheck = isstored && nm==as.userRef();
		if ( docheck ) docheckparent=true;
		mAddManagedMenuItem( &zattritm_, item, true, docheck )
	    }

	    zattritm_.enabled = true;
	}
    }

    mAddMenuItem( &selattrmnuitem_, &attrnoneitm_, true, false )
}


void uiOD2DLineSetAttribItem::handleMenuCB( CallBacker* cb )
{
    selcomps.erase();
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( !menu || mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayID()))
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
// TODO: Large part of this should go to the uiAttribPartServer

    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject( displayID() ))
    if ( !s2d ) return false;

    const Pos::GeomID geomid = s2d->getGeomID();
    const SeisIOObjInfo objinfo( attribnm, Seis::Line );
    SeisIOObjInfo::Opts2D opts2d; opts2d.zdomky_ = "*";
    BufferStringSet allattribs;
    objinfo.getDataSetNamesForLine( geomid, allattribs, opts2d );
    if ( !allattribs.isPresent(attribnm) || !objinfo.ioObj() )
	return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const MultiID key = objinfo.ioObj()->key();
    //First time to ensure all components are available
    Attrib::DescID attribid = attrserv->getStoredID( key, true );

    BufferStringSet complist;
    SeisIOObjInfo::getCompNames( key, complist );
    if ( complist.size()>1 && component<0 )
    {
	if ( ( !selcomps.size() &&
	       !attrserv->handleMultiComp( key, true, false, complist,
					   attribid, selcomps ) )
	     || ( selcomps.size() && !attrserv->prepMultCompSpecs(
		     selcomps, key, true, false ) ) )
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

	    s2d->showPanel( true );
	    updateColumnText(0);
	    return rescalc;
	}
    }
    else
    {
	if ( component>=0 )
	    selcomps.addIfNew( component );
	attribid = attrserv->getStoredID( key, true, component );
    }

    if ( !attribid.isValid() ) return false;

    TypeSet<Attrib::SelSpec> as = *visserv->getSelSpecs( displayID(), 0 );
    for ( int idx=0; idx<as.size(); idx++ )
    {
	as[idx].set( attribnm, attribid, false, nullptr );
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

    ConstRefMan<RegularSeisDataPack> rsdp =
			Seis::PLDM().get<RegularSeisDataPack>( key, geomid );
    if ( !rsdp )
    {
	attrserv->setTargetSelSpecs( as );
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
	const StringView zdomainkey = as[0].zDomainKey();
	const bool alreadytransformed =
		scene && zdomainkey==scene->zDomainKey();
	const TrcKeyZSampling tkzs =
		s2d->getTrcKeyZSampling( alreadytransformed );
	Seis::SequentialReader rdr( *objinfo.ioObj(), &tkzs, &selcomps );
	if ( !TaskRunner::execute(&taskrunner,rdr) )
	{
	    uiMSG().error( rdr.uiMessage() );
	    return false;
	}

	rsdp = rdr.getDataPack();
	DPM( DataPackMgr::SeisID() ).add( rsdp );
    }

    const DataPack::ID dpid = rsdp ? rsdp->id() : DataPack::cNoID();
    if ( dpid == DataPack::cNoID() )
	return false;

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    s2d->setSelSpecs( attribNr(), as );
    applMgr()->useDefColTab( displayID(), attribNr() );
    s2d->setDataPackID( attribNr(), dpid, &taskrunner );
    s2d->showPanel( true );
    s2d->enableAttrib( attribNr(), true );

    updateColumnText(0);

    if ( s2d->isOn() != isChecked() )
	setChecked( s2d->isOn(), true );

    return true;
}


void uiOD2DLineSetAttribItem::setAttrib( const Attrib::SelSpec& myas,
					 uiTaskRunner& )
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayID()))
    if ( !s2d )
	return;

    applMgr()->attrServer()->setTargetSelSpec( myas );
    const DataPack::ID dpid = applMgr()->attrServer()->createOutput(
						s2d->getTrcKeyZSampling(false),
							    DataPack::cNoID() );
    if ( dpid == DataPack::cNoID() )
	return;

    s2d->setSelSpecs( attribNr(), TypeSet<Attrib::SelSpec>(1,myas) );
    s2d->setDataPackID( attribNr(), dpid, nullptr );
    s2d->showPanel( true );

    updateColumnText(0);

    if ( s2d->isOn() != isChecked() )
	setChecked( s2d->isOn(), true );

    applMgr()->updateColorTable( displayID(), attribNr() );
}


void uiOD2DLineSetAttribItem::clearAttrib()
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    applMgr()->visServer()->getObject(displayID()))
    if ( !s2d )
	return;

    s2d->clearTexture( attribNr() );
    updateColumnText(0);
    applMgr()->updateColorTable( displayID(), attribNr() );
}
