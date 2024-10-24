/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodseis2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiattr2dsel.h"
#include "uigeninput.h"
#include "uigisexp.h"
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
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "visrgbatexturechannel2rgba.h"

#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribsel.h"
#include "emmanager.h"
#include "externalattrib.h"
#include "ioman.h"
#include "ioobj.h"
#include "mousecursor.h"
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
#define mExpGISIdx	4

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

    if ( !children_.isEmpty() )
    {
	mnu.insertAction( new uiAction(m3Dots(uiGISExpStdFld::sToolTipTxt()),
			  uiGISExpStdFld::strIcon()), mExpGISIdx );
    }

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
	auto* dispmnu = new uiMenu( getUiParent(), tr("Display All") );
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
			applMgr()->visServer()->getObject(itm->displayID()))
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

	if ( action==0 || geomids.isEmpty() )
	{
	    const int nrchildren = children_.size();
	    if ( nrchildren>10 )
		collapseAllChildren();

	    return true;
	}

	if ( action==1 )
	    loadDefaultData();
	else if ( action==2 )
	    selectLoadAttribute( geomids );
    }
    else if ( mnuid == mGridFrom3D )
	applMgr()->create2DGrid();
    else if ( mnuid == mFrom3D )
	applMgr()->create2DFrom3D();
    else if ( mnuid == mTo3D )
	applMgr()->create3DFrom2D();
    else if ( mnuid == mExpGISIdx )
    {
	TypeSet<Pos::GeomID> geomids;
	for ( const auto* treeitm : children_ )
	{
	    if ( !treeitm->isChecked() )
		continue;

	    mDynamicCastGet(const uiOD2DLineTreeItem*,linetreeitm,treeitm)
	    if ( linetreeitm )
		geomids += linetreeitm->getGeomID();
	}

	applMgr()->seisServer()->exportLinesToGIS( getUiParent(), &geomids );
    }
    else if ( mnuid == mAddAttr )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx])
	    const BufferString topattrnm = itm->nrChildren()<=0
		     ? BufferString::empty()
		     : itm->getChild(itm->nrChildren()-1)->name().getString();
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
	BufferString attrnm = itm->text().getString();
	if ( attrnm == sKeyUnselected() )
	    attrnm = sKeyRightClick();

	selectLoadAttribute( displayedgeomids, attrnm );
    }
    else if ( removeattritm_ && removeattritm_->findAction(mnuid) )
    {
	const uiAction* itm = removeattritm_->findAction( mnuid );
	BufferString attrnm = itm->text().getString();
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
	const BufferString attrnm = itm->text().getString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	for ( int idx=0; idx<set.size(); idx++ )
	    set[idx]->setChecked( disp, true );
    }
    else if ( editcoltabitm_ && editcoltabitm_->findAction(mnuid) )
    {
	const uiAction* itm = editcoltabitm_->findAction( mnuid );
	const BufferString attrnm = itm->text().getString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	if ( set.size() )
	{
	    uiODEditAttribColorDlg dlg( getUiParent(), set, attrnm );
	    dlg.go();
	}
    }
    else if ( mnuid >= mDispNames && mnuid <= mHidePolyLines )
    {
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx])
	    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
			    applMgr()->visServer()->getObject(itm->displayID()))
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

    ConstRefMan<Attrib::Desc> desc = ads->getDesc( descid );
    if ( !desc )
    {
	ads = Attrib::eDSHolder().getDescSet( true, true );
	if ( !ads )
	    return false;

	desc = ads->getDesc( descid ).ptr();
	if ( !desc )
	    return false;
    }

    const char* attrnm = desc->userRef();
    uiTaskRunner uitr( getUiParent() );
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
    uiVisPartServer* visserv = applMgr()->visServer();
    ConstRefMan<visSurvey::Scene> scene = visserv->getScene( sceneID() );
    const ZDomain::Info& info = scene->zDomainInfo();
    uiAttr2DSelDlg dlg( getUiParent(), ds, geomids, nla, info, curattrnm );

    uiTaskRunner uitr( getUiParent() );
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
	    ConstRefMan<Attrib::Desc> desc = ds->getDesc(dlg.getSelDescID());
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
uiTreeItem* Line2DTreeItemFactory::createForVis( const VisID& visid,
						 uiTreeItem* treeitem ) const
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    ODMainWin()->applMgr().visServer()->getObject(visid))
    if ( !s2d || !treeitem )
	return nullptr;

    mDynamicCastGet(visBase::RGBATextureChannel2RGBA*,rgba,
		    s2d->getChannels2RGBA())

    auto* newsubitm = new uiOD2DLineTreeItem( s2d->getGeomID(), visid, rgba );

    if ( newsubitm )
       treeitem->addChild( newsubitm,true );

    return nullptr;
}


uiOD2DLineTreeItem::uiOD2DLineTreeItem( const Pos::GeomID& geomid,
					const VisID& displayid, bool rgba )
    : geomid_(geomid)
    , linenmitm_(tr("Show Linename"))
    , panelitm_(tr("Show 2D Plane"))
    , polylineitm_(tr("Show Line Geometry"))
    , positionitm_(m3Dots(tr("Position")))
    , rgba_( rgba )
{
    displayid_ = displayid;
    name_ = toUiString(Survey::GM().getName( geomid ));

    positionitm_.iconfnm = "orientation64";
    linenmitm_.checkable = true;
    panelitm_.checkable = true;
    polylineitm_.checkable = true;
}


uiOD2DLineTreeItem::~uiOD2DLineTreeItem()
{
    detachAllNotifiers();
    visserv_->removeObject( displayid_, sceneID() );
}


const char* uiOD2DLineTreeItem::parentType() const
{ return typeid(uiODLine2DParentTreeItem).name(); }


bool uiOD2DLineTreeItem::init()
{
    bool newdisplay = false;
    if ( !displayid_.isValid() )
    {
	RefMan<visSurvey::Seis2DDisplay> s2d = new visSurvey::Seis2DDisplay;
	displayid_ = s2d->id();
	s2d->setGeomID( geomid_ );
	s2d->turnOn( true );
	if ( rgba_ )
	{
	    RefMan<visBase::RGBATextureChannel2RGBA> text2rgba =
				visBase::RGBATextureChannel2RGBA::create();
	    s2d->setChannels2RGBA( text2rgba.ptr() );
	    s2d->addAttrib();
	    s2d->addAttrib();
	    s2d->addAttrib();
	}

	visserv_->addObject( s2d.ptr(), sceneID(), true);
	newdisplay = true;
    }

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d )
	return false;

    const Survey::Geometry* geom = Survey::GM().getGeometry( geomid_ );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
    if ( !geom2d )
	return false;

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
    if ( newdisplay )
    {
	const bool hasworkzrg = SI().zRange(true) != SI().zRange(false);
	const bool hastransform = s2d->getZAxisTransform();
	if ( hasworkzrg && !hastransform )
	{
	    ZSampling newzrg = geom2d->data().zRange();
	    newzrg.limitTo( SI().zRange(true) );
	    s2d->setZRange( newzrg );
	}
    }
    else
    {
	if ( !oldtrcnrrg.isUdf() )
	    s2d->setTraceNrRange( oldtrcnrrg );

	if ( !oldzrg.isUdf() )
	    s2d->setZRange( oldzrg );
    }

    if ( applMgr() )
	mAttachCB( applMgr()->getOtherFormatData,
		   uiOD2DLineTreeItem::getNewData );

    if ( rgba_ )
	selectRGBA( geomid_ );

    seis2ddisplay_ = s2d;
    return uiODDisplayTreeItem::init();
}


ConstRefMan<visSurvey::Seis2DDisplay> uiOD2DLineTreeItem::getDisplay() const
{
    return seis2ddisplay_.get();
}


RefMan<visSurvey::Seis2DDisplay> uiOD2DLineTreeItem::getDisplay()
{
    return seis2ddisplay_.get();
}


uiString uiOD2DLineTreeItem::createDisplayName() const
{
    return visserv_->getUiObjectName( displayid_ );
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
    ConstRefMan<visSurvey::Seis2DDisplay> seis2ddisplay = getDisplay();
    if ( !menu || !isDisplayID(menu->menuID()) || !seis2ddisplay )
	return;

    mAddMenuOrTBItem( istb, nullptr, &displaymnuitem_, &linenmitm_,
		      true, seis2ddisplay->isLineNameShown() )
    mAddMenuOrTBItem( istb, nullptr, &displaymnuitem_, &panelitm_,
		      true, seis2ddisplay->isPanelShown() )
    mAddMenuOrTBItem( istb, nullptr, &displaymnuitem_, &polylineitm_,
		      true, seis2ddisplay->isPolyLineShown() )
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionitm_, true, false)
}


void uiOD2DLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller)
    if ( !menu || menu->isHandled() || mnuid==-1 )
	return;

    RefMan<visSurvey::Seis2DDisplay> seis2ddisplay = getDisplay();
    if ( !seis2ddisplay || !isDisplayID(menu->menuID()) )
	return;

    if ( mnuid==linenmitm_.id )
    {
	menu->setIsHandled(true);
	seis2ddisplay->showLineName( !seis2ddisplay->isLineNameShown() );
    }
    else if ( mnuid==panelitm_.id )
    {
	menu->setIsHandled(true);
	seis2ddisplay->showPanel( !seis2ddisplay->isPanelShown() );
    }
    else if ( mnuid==polylineitm_.id )
    {
	menu->setIsHandled(true);
	seis2ddisplay->showPolyLine( !seis2ddisplay->isPolyLineShown() );
    }
    else if ( mnuid==positionitm_.id )
    {
	menu->setIsHandled(true);

	const TrcKeyZSampling tkzs( seis2ddisplay->getTrcKeyZSampling(true) );
	TrcKeyZSampling maxcs( tkzs.hsamp_.getGeomID() );
	maxcs.hsamp_.setTrcRange( seis2ddisplay->getMaxTraceNrRange() );
	assign( maxcs.zsamp_, seis2ddisplay->getMaxZRange(true)  );

	RefMan<visSurvey::Scene> scene = visserv_->getScene(sceneID());
	CallBack dummy;
	uiSliceSelDlg positiondlg( getUiParent(), tkzs,
				   maxcs, dummy, uiSliceSel::TwoD,
				   scene->zDomainInfo() );
	if ( !positiondlg.go() )
	    return;

	const TrcKeyZSampling newcs = positiondlg.getTrcKeyZSampling();
	const Interval<float> newzrg = newcs.zsamp_;
	if ( !newzrg.isEqual(seis2ddisplay->getZRange(true),mDefEpsF) )
	{
	    seis2ddisplay->annotateNextUpdateStage( true );
	    seis2ddisplay->setZRange( newzrg );
	}

	const Interval<int> ntrcnrrg = newcs.hsamp_.trcRange();
	if ( ntrcnrrg != seis2ddisplay->getTraceNrRange() )
	{
	    if ( !seis2ddisplay->getUpdateStageNr() )
		seis2ddisplay->annotateNextUpdateStage( true );

	    seis2ddisplay->setTraceNrRange( ntrcnrrg );
	}

	if ( seis2ddisplay->getUpdateStageNr() )
	{
	    seis2ddisplay->annotateNextUpdateStage( true );
	    for ( int idx=0; idx<seis2ddisplay->nrAttribs(); idx++ )
	    {
		if ( seis2ddisplay->getSelSpec(idx) &&
		     seis2ddisplay->getSelSpec(idx)->id().isValid() )
		    visserv_->calculateAttrib( displayid_, idx, false );
	    }

	    seis2ddisplay->annotateNextUpdateStage( false );
	}

	updateColumnText( 0 );
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
    ConstRefMan<Attrib::Desc> desc = ads->getDesc( descid );
    if ( !desc )
	return false;

    selcomps.erase();
    const char* attrnm = desc->userRef();
    uiTaskRunner uitr( getUiParent() );
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
    RefMan<visSurvey::Seis2DDisplay> seis2ddisplay = getDisplay();
    if ( !seis2ddisplay )
	return;

    const TrcKeyZSampling tkzs = seis2ddisplay->getTrcKeyZSampling( false );
    const TypeSet<Attrib::SelSpec>& as = *seis2ddisplay->getSelSpecs( attribnr);

    ConstRefMan<RegularSeisDataPack> newdp;
    uiTaskRunner uitr( getUiParent() );
    if ( as[0].id().asInt() == Attrib::SelSpec::cOtherAttrib().asInt() )
    {
	PtrMan<Attrib::ExtAttribCalc> calc =
	    Attrib::ExtAttrFact().create( nullptr, as[0], false );
	if ( !calc )
	{
	    uiMSG().error( tr("Attribute cannot be created") );
	    return;
	}

	newdp = calc->createAttrib( tkzs, &uitr );
    }
    else
    {
	applMgr()->attrServer()->setTargetSelSpecs( as );
	newdp = applMgr()->attrServer()->createOutput( tkzs, nullptr );
    }

    if ( !newdp )
	return;

    ((visSurvey::SurveyObject*) seis2ddisplay.ptr())->
		setSeisDataPack( attribnr, newdp.getNonConstPtr(), &uitr );
    seis2ddisplay->showPanel( true );
}


void uiOD2DLineTreeItem::showLineName( bool yn )
{
    RefMan<visSurvey::Seis2DDisplay> seis2ddisplay = getDisplay();
    if ( seis2ddisplay )
	seis2ddisplay->showLineName( yn );
}


void uiOD2DLineTreeItem::setZRange( const Interval<float> newzrg )
{
    RefMan<visSurvey::Seis2DDisplay> seis2ddisplay = getDisplay();
    if ( !seis2ddisplay )
	return;

    seis2ddisplay->annotateNextUpdateStage( true );
    seis2ddisplay->setZRange( newzrg );
    seis2ddisplay->annotateNextUpdateStage( true );
    for ( int idx=0; idx<seis2ddisplay->nrAttribs(); idx++ )
    {
	if ( seis2ddisplay->getSelSpec(idx) &&
	     seis2ddisplay->getSelSpec(idx)->id().isValid() )
	    visserv_->calculateAttrib( displayid_, idx, false );
    }

    seis2ddisplay->annotateNextUpdateStage( false );
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
    ConstRefMan<Attrib::Desc> desc = ads->getDesc( as.id() );
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
	auto* item = new MenuItem( toUiString(nm) );
	const bool docheck = isstored && nm==as.userRef();
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &steeringitm_,item,true,docheck)
    }

    mAddMenuItem( &selattrmnuitem_, &steeringitm_, true, docheckparent )

    zattritm_.removeItems();
    RefMan<visSurvey::Scene> scene = visserv_->getScene(sceneID());
    if ( scene && scene->getZAxisTransform() )
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

    uiTaskRunner uitr( getUiParent() );
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
    if ( !s2d )
	return false;

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
		visserv->setSelObjectId( displayID() );

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

    if ( !attribid.isValid() )
	return false;

    TypeSet<Attrib::SelSpec> as = *visserv->getSelSpecs( displayID(), 0 );
    for ( int idx=0; idx<as.size(); idx++ )
    {
	as[idx].set( attribnm, attribid, false, nullptr );
	as[idx].set2DFlag();
	const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true, true );
	if ( !ds )
	    return false;

	as[idx].setRefFromID( *ds );
	ConstRefMan<Attrib::Desc> targetdesc = ds->getDesc( attribid );
	if ( !targetdesc )
	    return false;

	BufferString defstring;
	targetdesc->getDefStr( defstring );
	as[idx].setDefString( defstring );
    }

    ConstRefMan<RegularSeisDataPack> rsdp =
			Seis::PLDM().get<RegularSeisDataPack>( key, geomid );
    if ( !rsdp )
    {
	attrserv->setTargetSelSpecs( as );
	const RefMan<visSurvey::Scene> scene = visserv->getScene( sceneID() );
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
    }

    if ( !rsdp )
	return false;

    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    s2d->setSelSpecs( attribNr(), as );
    applMgr()->useDefColTab( displayID(), attribNr() );
    ((visSurvey::SurveyObject*) s2d)->
	setSeisDataPack( attribNr(), rsdp.getNonConstPtr(), &taskrunner );
    s2d->showPanel( true );
    s2d->enableAttrib( attribNr(), true );

    updateColumnText(0);
    updateColumnText(1);

    if ( s2d->isOn() != isChecked() )
	setChecked( s2d->isOn(), true );

    return true;
}


void uiOD2DLineSetAttribItem::setAttrib( const Attrib::SelSpec& myas,
					 uiTaskRunner& )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayID()))
    if ( !s2d )
	return;

    applMgr()->attrServer()->setTargetSelSpec( myas );
    ConstRefMan<RegularSeisDataPack> regsd =
	applMgr()->attrServer()->createOutput( s2d->getTrcKeyZSampling(false),
					       nullptr );
    if ( !regsd )
	return;

    s2d->setSelSpecs( attribNr(), TypeSet<Attrib::SelSpec>(1,myas) );
    ((visSurvey::SurveyObject*) s2d)->
	setSeisDataPack( attribNr(), regsd.getNonConstPtr(), nullptr );
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
