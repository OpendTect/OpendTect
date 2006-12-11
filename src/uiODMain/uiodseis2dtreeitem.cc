/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		May 2006
 RCS:		$Id: uiodseis2dtreeitem.cc,v 1.7 2006-12-11 19:38:57 cvskris Exp $
___________________________________________________________________

-*/

#include "uiodseis2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uicursor.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uivispartserv.h"
#include "visseis2ddisplay.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "linekey.h"
#include "segposinfo.h"
#include "survinfo.h"



uiODSeis2DParentTreeItem::uiODSeis2DParentTreeItem()
    : uiODTreeItem("2D" )
{
}


bool uiODSeis2DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );

    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) return false;

    MultiID mid;
    const bool success =
	ODMainWin()->applMgr().seisServer()->select2DSeis( mid );
    if ( !success ) return false;

    uiOD2DLineSetTreeItem* newitm = new uiOD2DLineSetTreeItem( mid );
    addChild( newitm, true );
    newitm->selectAddLines();

    return true;
}


uiTreeItem* Seis2DTreeItemFactory::create( int visid,
					   uiTreeItem* treeitem ) const
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    ODMainWin()->applMgr().visServer()->getObject(visid))
    if ( !s2d || !treeitem ) return 0;

    uiOD2DLineSetSubItem* newsubitm =
	new uiOD2DLineSetSubItem( s2d->name(), visid );
    mDynamicCastGet(uiOD2DLineSetSubItem*,subitm,treeitem)
    if ( subitm )
	return newsubitm;

    const MultiID& setid = s2d->lineSetID();
    BufferString linesetname;
    ODMainWin()->applMgr().seisServer()->get2DLineSetName( setid, linesetname );
    uiTreeItem* linesetitm = treeitem->findChild( linesetname );
    if ( linesetitm )
    {
	linesetitm->addChild( newsubitm, true );
	return 0;
    }

    uiOD2DLineSetTreeItem* newlinesetitm = new uiOD2DLineSetTreeItem( setid );
    treeitem->addChild( newlinesetitm, true );
    newlinesetitm->addChild( newsubitm, true );
    return 0;
}


uiOD2DLineSetTreeItem::uiOD2DLineSetTreeItem( const MultiID& mid )
    : uiODTreeItem("")
    , setid_( mid )
    , menuhandler_(0)
    , addlinesitm_("Add line(s) ...")
    , showitm_("Show all")
    , hideitm_("Hide all")
    , showlineitm_("Lines")
    , hidelineitm_("Lines")
    , showlblitm_("Linenames")
    , hidelblitm_("Linenames")
    , removeitm_("Remove")
    , storeditm_("Stored 2D data")
    , selattritm_("Select Attribute")
    , zrgitm_("Set Z-Range ...")
{
    assign(curzrg_,SI().zRange(true));
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


bool uiOD2DLineSetTreeItem::showSubMenu()
{
    if ( !menuhandler_ )
    {
	menuhandler_ = new uiMenuHandler( getUiParent(), -1 );
	menuhandler_->createnotifier.notify(
		mCB(this,uiOD2DLineSetTreeItem,createMenuCB) );
	menuhandler_->handlenotifier.notify(
		mCB(this,uiOD2DLineSetTreeItem,handleMenuCB) );
	menuhandler_->ref();
    }

    menuhandler_->executeMenu( uiMenuHandler::fromTree );
    return true;
}


void uiOD2DLineSetTreeItem::selectAddLines()
{
    BufferStringSet linenames;
    applMgr()->seisServer()->select2DLines( setid_, linenames );

    uiCursorChanger cursorchgr( uiCursor::Wait );
    for ( int idx=linenames.size()-1; idx>=0; idx-- )
	addChild( new uiOD2DLineSetSubItem(linenames.get(idx)), true );
}


void uiOD2DLineSetTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    mAddMenuItem( menu, &addlinesitm_, true, false );

    if ( children_.size() )
    {
	Attrib::SelSpec as;
	MenuItem* dummy = applMgr()->attrServer()->storedAttribMenuItem( as );
	dummy->removeItems();

	BufferStringSet allstored;
	Attrib::SelInfo::getAttrNames( setid_, allstored );
	allstored.sort();
	storeditm_.createItems( allstored );
	mAddMenuItem( &selattritm_, &storeditm_, storeditm_.nrItems(), false );

	MenuItem* attrmenu = applMgr()->attrServer()->calcAttribMenuItem( as );
	mAddMenuItem( &selattritm_, attrmenu, attrmenu->nrItems(), false );

	MenuItem* nla = applMgr()->attrServer()->nlaAttribMenuItem( as );
	if ( nla && nla->nrItems() )
	    mAddMenuItem( &selattritm_, nla, true, false );

	mAddMenuItem( menu, &selattritm_, true, false );
	mAddMenuItem( menu, &showitm_, true, false );
	mAddMenuItem( &showitm_, &showlineitm_, true, false );
	mAddMenuItem( &showitm_, &showlblitm_, true, false );

	mAddMenuItem( menu, &hideitm_, true, false );
	mAddMenuItem( &hideitm_, &hidelineitm_, true, false );
	mAddMenuItem( &hideitm_, &hidelblitm_, true, false );

	mAddMenuItem( menu, &zrgitm_, true, false );
    }
    else
    {
	mResetMenuItem( &showitm_ );
	mResetMenuItem( &showlineitm_ );
	mResetMenuItem( &showlblitm_ );

	mResetMenuItem( &hideitm_ );
	mResetMenuItem( &hidelineitm_ );
	mResetMenuItem( &hidelblitm_ );

	mResetMenuItem( &zrgitm_ );
    }

    mAddMenuItem( menu, &removeitm_, true, false );
}


void uiOD2DLineSetTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==addlinesitm_.id )
    {
	menu->setIsHandled(true);
	selectAddLines();
	return;
    }

    Attrib::SelSpec as;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	const char idx = storeditm_.itemIndex( mnuid );
	const char* attribnm = storeditm_.getItem(idx)->text;
	for ( int idx=0; idx<children_.size(); idx++ )
	   ((uiOD2DLineSetSubItem*)children_[idx])->displayStoredData(attribnm);
    }
    else if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,as) )
    {
	menu->setIsHandled( true );
	for ( int idx=0; idx<children_.size(); idx++ )
	    ((uiOD2DLineSetSubItem*)children_[idx])->setAttrib( as );
    }
    else if ( mnuid==removeitm_.id )
    {
	menu->setIsHandled( true );
	if ( children_.size()>0 &&
	     !uiMSG().askGoOn("All lines in this lineset will be removed."
			      "\nDo you want to continue?") )
	    return;

	while ( children_.size() )
	{
	    uiOD2DLineSetSubItem* itm = (uiOD2DLineSetSubItem*)children_[0];
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
	parent_->removeChild( this );
    }
    else if ( mnuid==showlineitm_.id || mnuid==hidelineitm_.id )
    {
	menu->setIsHandled( true );
	const bool turnon = mnuid==showlineitm_.id;
	for ( int idx=0; idx<children_.size(); idx++ )
	    children_[idx]->setChecked( turnon );
    }
    else if ( mnuid==showlblitm_.id || mnuid==hidelblitm_.id )
    {
	menu->setIsHandled( true );
	const bool turnon = mnuid==showlblitm_.id;
	for ( int idx=0; idx<children_.size(); idx++ )
	    ((uiOD2DLineSetSubItem*)children_[idx])->showLineName( turnon );
    }
    else if ( mnuid == zrgitm_.id )
    {
	menu->setIsHandled( true );

	BufferString lbl( "Z-Range " ); lbl += SI().getZUnit();
	Interval<int> intzrg( mNINT(curzrg_.start*1000), 
			      mNINT(curzrg_.stop*1000) );
	uiGenInputDlg dlg( getUiParent(), "Specify 2D line Z_Range", lbl,
			   new IntInpIntervalSpec(intzrg) );
	if ( !dlg.go() ) return;

	intzrg = dlg.getFld()->getIInterval();
	curzrg_.start = float(intzrg.start) / 1000;
	curzrg_.stop = float(intzrg.stop) / 1000;
	for ( int idx=0; idx<children_.size(); idx++ )
	    ((uiOD2DLineSetSubItem*)children_[idx])->setZRange( curzrg_ );
    }
}


const char* uiOD2DLineSetTreeItem::parentType() const
{ return typeid(uiODSeis2DParentTreeItem).name(); }



bool uiOD2DLineSetTreeItem::init()
{
    applMgr()->seisServer()->get2DLineSetName( setid_, name_ );
    return true;
}


uiOD2DLineSetSubItem::uiOD2DLineSetSubItem( const char* nm, int displayid )
    : linenmitm_("Show linename")
    , positionitm_("Position ...")
{
    name_ = nm;
    displayid_ = displayid;
}


uiOD2DLineSetSubItem::~uiOD2DLineSetSubItem()
{
    applMgr()->getOtherFormatData.remove(
	    mCB(this,uiOD2DLineSetSubItem,getNewData) );
}


const char* uiOD2DLineSetSubItem::parentType() const
{ return typeid(uiOD2DLineSetTreeItem).name(); }


bool uiOD2DLineSetSubItem::init()
{
    bool newdisplay = false;
    if ( displayid_==-1 )
    {
	mDynamicCastGet(uiOD2DLineSetTreeItem*,lsitm,parent_)
	if ( !lsitm ) return false;

	visSurvey::Seis2DDisplay* s2d = visSurvey::Seis2DDisplay::create();
	visserv->addObject( s2d, sceneID(), false );
	s2d->setLineName( name_ );
	s2d->setLineSetID( lsitm->lineSetID() );
	displayid_ = s2d->id();

	s2d->turnOn( true );
	newdisplay = true;
    }

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
	    	    visserv->getObject(displayid_))
    if ( !s2d ) return false;

    PtrMan<PosInfo::Line2DData> geometry = new PosInfo::Line2DData;
    if ( !applMgr()->seisServer()->get2DLineGeometry(s2d->lineSetID(),name_,
	  *geometry) )
	return false;

    s2d->setGeometry( *geometry );

    if ( applMgr() )
	applMgr()->getOtherFormatData.notify(
	    mCB(this,uiOD2DLineSetSubItem,getNewData) );

    return uiODDisplayTreeItem::init();
}


BufferString uiOD2DLineSetSubItem::createDisplayName() const
{
    return BufferString( visserv->getObjectName(displayid_) );
}


uiODDataTreeItem* uiOD2DLineSetSubItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiOD2DLineSetAttribItem::create( *as, parenttype ) : 0;

    if ( !res ) res = new uiOD2DLineSetAttribItem( parenttype );
    return res;
}



void uiOD2DLineSetSubItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb)
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( !menu || menu->menuID() != displayID() || !s2d ) return;

    mAddMenuItem( menu, &linenmitm_, true, s2d->lineNameShown() );
    mAddMenuItem( menu, &positionitm_, true, false );
}


void uiOD2DLineSetSubItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_));
    if ( !menu || !s2d || menu->menuID()!=displayID() || 
	 mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==linenmitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showLineName( !s2d->lineNameShown() );
    }
    else if ( mnuid==positionitm_.id )
    {
	menu->setIsHandled(true);

	CubeSampling maxcs;
	assign( maxcs.zrg, s2d->getMaxZRange()  );
	maxcs.hrg.start.crl = s2d->getMaxTraceNrRange().start;
	maxcs.hrg.stop.crl = s2d->getMaxTraceNrRange().stop;

	CubeSampling curcs;
	curcs.zrg.start = maxcs.zrg.atIndex( s2d->getZRange().start );
	curcs.zrg.stop = maxcs.zrg.atIndex( s2d->getZRange().stop );
	curcs.hrg.start.crl = s2d->getTraceNrRange().start;
	curcs.hrg.stop.crl = s2d->getTraceNrRange().stop;

	CallBack dummy;
	uiSliceSel positiondlg( getUiParent(), curcs,
				maxcs, dummy, uiSliceSel::TwoD );
	if ( !positiondlg.go() ) return;
	const CubeSampling newcs = positiondlg.getCubeSampling();

	bool doupdate = false;
	bool usecache = true;
	const Interval<int> nzrg(maxcs.zrg.nearestIndex(newcs.zrg.start),
				 maxcs.zrg.nearestIndex(newcs.zrg.stop));
	const Interval<int> ntrcnrrg(newcs.hrg.start.crl,newcs.hrg.stop.crl);

	if ( nzrg!=s2d->getZRange() )
	{
	    doupdate = true;
	    if ( !s2d->setZRange( nzrg ) )
		usecache = false;
	}

	if ( ntrcnrrg!=s2d->getTraceNrRange() )
	{
	    doupdate = true;
	    if ( !s2d->setTraceNrRange( ntrcnrrg ) )
		usecache = false;
	}

	if ( doupdate )
	{
	    if ( usecache )
		s2d->updateDataFromCache();
	    else
	    {
		for ( int idx=s2d->nrAttribs()-1; idx>=0; idx-- )
		{
		    if ( s2d->getSelSpec(idx) && s2d->getSelSpec(idx)->id()>=0 )
			visserv->calculateAttrib( displayid_, idx, false );
		}
	    }
	}
    }
}


bool uiOD2DLineSetSubItem::displayStoredData( const char* nm )
{
    if ( !children_.size() ) return false;

    mDynamicCastGet( uiOD2DLineSetAttribItem*, lsai, children_[0] );
    if ( !lsai ) return false;

    return lsai->displayStoredData(nm);
}


void uiOD2DLineSetSubItem::setAttrib( const Attrib::SelSpec& myas )
{
    if ( !children_.size() ) return;

    mDynamicCastGet( uiOD2DLineSetAttribItem*, lsai, children_[0] );
    if ( !lsai ) return;

    lsai->setAttrib( myas );
}


void uiOD2DLineSetSubItem::getNewData( CallBacker* cb )
{
    const int visid = applMgr()->otherFormatVisID();
    if ( visid != displayid_ ) return;

    const int attrib = applMgr()->otherFormatAttrib();

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( !s2d ) return;

    CubeSampling cs;
    cs.hrg.start.crl = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl = s2d->getTraceNrRange().stop;
    cs.zrg.start = s2d->getMaxZRange().atIndex(s2d->getZRange().start);
    cs.zrg.stop = s2d->getMaxZRange().atIndex(s2d->getZRange().stop);

    const char* objnm = s2d->name();

    const Attrib::SelSpec& as = *s2d->getSelSpec( attrib );
    applMgr()->attrServer()->setTargetSelSpec( as );
    RefMan<Attrib::Data2DHolder> dataset = new Attrib::Data2DHolder;

    if ( !applMgr()->attrServer()->create2DOutput( cs, objnm, *dataset) )
	return;

    if ( dataset->size() )
	s2d->setTraceData( attrib, *dataset );
}


void uiOD2DLineSetSubItem::showLineName( bool yn )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( s2d ) s2d->showLineName( yn );
}


void uiOD2DLineSetSubItem::setZRange( const Interval<float> newzrg )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( !s2d ) return;

    const StepInterval<float>& maxrg = s2d->getMaxZRange();
    if ( s2d->setZRange( Interval<int>( maxrg.nearestIndex(newzrg.start),
		    			maxrg.nearestIndex(newzrg.stop))) )
    {
	s2d->updateDataFromCache();
    }
    else
    {
	for ( int idx=s2d->nrAttribs()-1; idx>=0; idx-- )
	{
	    if ( s2d->getSelSpec(idx) && s2d->getSelSpec(idx)->id()>=0 )
		visserv->calculateAttrib( displayid_, idx, false );
	}
    }
}


uiOD2DLineSetAttribItem::uiOD2DLineSetAttribItem( const char* pt )
    : uiODAttribTreeItem( pt )
    , attrnoneitm_("None")
    , storeditm_("Stored 2D data")
{}


void uiOD2DLineSetAttribItem::createMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    const uiVisPartServer* visserv = applMgr()->visServer();

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject( displayID() ))
    if ( !menu || !s2d ) return;

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec& as = *visserv->getSelSpec( displayID(),
	    					      attribNr() );
    const char* objnm = visserv->getObjectName( displayID() );

    BufferStringSet attribnames;
    seisserv->get2DStoredAttribs( s2d->lineSetID(), objnm, attribnames );
    const int idx = attribnames.indexOf( "Steering" );
    if ( idx>=0 ) attribnames.remove( idx );

    const Attrib::DescSet* ads = attrserv->curDescSet();
    const Attrib::Desc* desc = ads->getDesc( as.id() );
    const bool isstored = desc && desc->isStored();

    selattrmnuitem_.removeItems();

    bool docheckparent = false;
    storeditm_.removeItems();
    for ( int idx=0; idx<attribnames.size(); idx++ )
    {
	const char* nm = attribnames.get(idx);
	MenuItem* item = new MenuItem(nm);
	const bool docheck = isstored && !strcmp(nm,as.userRef());
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &storeditm_,item,true,docheck);
    }

    mAddMenuItem( &selattrmnuitem_, &storeditm_, true, docheckparent );

    MenuItem* attrmenu = attrserv->calcAttribMenuItem(as);
    mAddMenuItem( &selattrmnuitem_, attrmenu, attrmenu->nrItems(), false );

    MenuItem* nla = attrserv->nlaAttribMenuItem(as);
    if ( nla && nla->nrItems() )
	mAddMenuItem( &selattrmnuitem_, nla, true, false );

    mAddMenuItem( &selattrmnuitem_, &attrnoneitm_, as.id()>=0, false );
}


void uiOD2DLineSetAttribItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
    visserv->getObject( displayID() ));
    if ( !menu || !s2d || mnuid==-1 || menu->isHandled() )
	return;

    Attrib::SelSpec myas;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	uiCursorChanger cursorchgr( uiCursor::Wait );
	menu->setIsHandled(true);
	displayStoredData( storeditm_.findItem(mnuid)->text );
    }
    else if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,myas ) )
    {
	menu->setIsHandled(true);
	setAttrib( myas );
    }
    else if ( mnuid==attrnoneitm_.id )
    {
	//TODO
	uiCursorChanger cursorchgr( uiCursor::Wait );
	menu->setIsHandled(true);
	//s2d->clearTexture();
	updateColumnText(0);
    }
}


bool uiOD2DLineSetAttribItem::displayStoredData( const char* attribnm )
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject( displayID() ))
    if ( !s2d ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::DescID attribid =
		attrserv->createStored2DAttrib( s2d->lineSetID(), attribnm );
    if ( attribid < 0 ) return false;

    const Attrib::SelSpec* as = visserv->getSelSpec(  displayID(),0 );
    Attrib::SelSpec myas( *as );
    LineKey linekey( s2d->name(), attribnm );
    myas.set( attribnm, attribid, false, 0 );
    attrserv->setTargetSelSpec( myas );
    RefMan<Attrib::Data2DHolder> dataset = new Attrib::Data2DHolder;

    CubeSampling cs;
    cs.hrg.start.crl = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl = s2d->getTraceNrRange().stop;
    cs.zrg.start = s2d->getMaxZRange().atIndex(s2d->getZRange().start);
    cs.zrg.stop = s2d->getMaxZRange().atIndex(s2d->getZRange().stop);

    if ( !applMgr()->attrServer()->create2DOutput( cs, linekey, *dataset) )
	return false;

    if ( dataset->size() )
    {
	uiCursorChanger cursorchgr( uiCursor::Wait );
	s2d->setSelSpec( attribNr(), myas );
	s2d->setTraceData( attribNr(), *dataset );
    }
    updateColumnText(0);
    setChecked( s2d->isOn() );

    return true;
}


void uiOD2DLineSetAttribItem::setAttrib( const Attrib::SelSpec& myas )
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayID()))

    CubeSampling cs;
    cs.hrg.start.crl = s2d->getTraceNrRange().start;
    cs.hrg.stop.crl = s2d->getTraceNrRange().stop;
    cs.zrg.start = s2d->getMaxZRange().atIndex(s2d->getZRange().start);
    cs.zrg.stop = s2d->getMaxZRange().atIndex(s2d->getZRange().stop);

    BufferString linekey = s2d->name();
    applMgr()->attrServer()->setTargetSelSpec( myas );
    RefMan<Attrib::Data2DHolder> dataset = new Attrib::Data2DHolder;

    if ( !applMgr()->attrServer()->
	    create2DOutput( cs, linekey, *dataset ) )
	return;

    if ( dataset->size() )
    {
	s2d->setSelSpec( attribNr(), myas );
	s2d->setTraceData( attribNr(), *dataset );
    }

    updateColumnText(0);
    setChecked( s2d->isOn() );
}
