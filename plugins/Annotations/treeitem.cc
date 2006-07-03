/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2005
 RCS:           $Id: treeitem.cc,v 1.1 2006-07-03 20:02:06 cvskris Exp $
________________________________________________________________________

-*/

#include "treeitem.h"

#include "uiarrowdlg.h"
//#include "uifiledlg.h"
#include "uilistview.h"
#include "uislider.h"
#include "uimenu.h"
#include "uiodapplmgr.h"
#include "uivispartserv.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
//#include "uitextedit.h"
//#include "visannotdisplay.h"
//#include "vistransform.h"
//#include "vistransmgr.h"
#include "vissurvscene.h"

#include "visarrow.h"
#include "viscallout.h"
#include "visimage.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "pickset.h"
#include "picksettr.h"
#include "ptrman.h"
#include "uicolor.h"
#include "uiioobjsel.h"
#include "uimsg.h"


namespace Annotations
{

int AnnotTreeItem::defcolnr = 0;


ParentTreeItem::ParentTreeItem()
    : uiTreeItem("Annotations")
{}


ParentTreeItem::~ParentTreeItem()
{}


int ParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>(uiODTreeTop::sceneidkey,sceneid) )
	return -1;
    return sceneid;
}


bool ParentTreeItem::init()
{
    bool ret = uiTreeItem::init();
    if ( !ret ) return false;
    
    addChild( new SymbolParentItem(), true );
    //addChild( new ImageParentItem(), true );
    addChild( new TextParentItem(), true );
    getItem()->setOpen( false );
    return true;
}


const char* ParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }


// TreeItemFactory +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

uiTreeItem* TreeItemFactory::create( int visid, uiTreeItem* treeitem ) const
{
    return 0;
}


// Base AnnotTreeItem ++++++++++++++++++++++++++++++++++++++++++++++++++++++

AnnotTreeItem::AnnotTreeItem( const char* type_ )
    : uiODTreeItem(type_)
    , typestr_(type_)
{ }


AnnotTreeItem::~AnnotTreeItem()
{}


const char* AnnotTreeItem::parentType() const
{ return typeid(ParentTreeItem).name(); }


bool AnnotTreeItem::init()
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.setAdded.notify( mCB(this,AnnotTreeItem,setAddedCB));
    mgr.setToBeRemoved.notify( mCB(this,AnnotTreeItem,setRemovedCB));
    for ( int idx=0; idx<mgr.size(); idx++ )
    {
	uiTreeItem* item = createSubItem( -1,  mgr.get(idx) );
	addChild( item, true );
	item->setChecked( false );
    }

    return true;
}


void AnnotTreeItem::prepareForShutdown()
{
    Pick::SetMgr::getMgr( managerName() ).removeCBs( this );
}


void AnnotTreeItem::setAddedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps ) return;

    uiTreeItem* item = createSubItem( -1, *ps );
    addChild( item, true );
}



void AnnotTreeItem::setRemovedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps ) return;

    uiVisPartServer* visserv = applMgr()->visServer();

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,itm,children_[idx])
	if ( !itm ) continue;

	const int displayid = itm->displayID();
	mDynamicCastGet(visSurvey::LocationDisplay*,ld,
			visserv->getObject(displayid));
	if ( !ld ) continue;

	if ( ld->getSet() == ps )
	{
	    applMgr()->visServer()->removeObject( displayid, sceneID() );
	    uiTreeItem::removeChild( itm );
	    return;
	}
    }
}


bool AnnotTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    applMgr()->visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add Annotations to this scene (yet)" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    BufferString addtxt = "Add "; addtxt += typestr_; addtxt += " group ...";
    mnu.insertItem( new uiMenuItem(addtxt), 0 );
    mnu.insertItem( new uiMenuItem("Load ..."), 1 );

    const int mnusel = mnu.exec();
    if ( mnusel < 0 ) return false;

    if ( mnusel == 0 )
    {
	BufferString title = typestr_; title += " Annotations";
	uiGenInputDlg dlg( getUiParent(), title, "Group name", 
			   new StringInpSpec );
	dlg.setCaption( "Annotations" );

	while ( true )
	{
	    if ( !dlg.go() ) return false;

	    const char* txt = dlg.text();
	    if ( !txt || !*txt ) continue;

	    if ( SubItem::doesNameExist( txt, managerName() ) &&
	         !uiMSG().askGoOn("An object with that name does allready"
			" exists.\nDo you wish to overwrite it?" ) )
		continue;

	    MultiID mid;
	    if ( SubItem::createIOEntry(txt,true,mid,managerName())!=1 )
		return false;

	    Pick::Set* set = new Pick::Set(txt);
	    set->disp_.color_ = Color::drawDef(defcolnr++);
	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.set( mid, set );
	    break;
	}
    }
    else if ( mnusel == 1 )
    {
	Pick::Set* ps = new Pick::Set;
	if ( !readPicks(*ps) ) { delete ps; return false; }
    }

    return true;
}

#define mDelCtioRet { delete ctio->ioobj; delete ctio; return false; }

bool AnnotTreeItem::readPicks( Pick::Set& ps )
{
    CtxtIOObj* ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt.forread = true;
    ctio->ctxt.parconstraints.set( sKey::Type, typestr_ );
    ctio->ctxt.includeconstraints = true;
    uiIOObjSelDlg dlg( getUiParent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	mDelCtioRet

    BufferString bs;
    if ( !PickSetTranslator::retrieve(ps,dlg.ioObj(),bs) )
    { uiMSG().error( bs ); mDelCtioRet }

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.set( dlg.ioObj()->key(), &ps );

    return true;
}




// SubItem ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

SubItem::SubItem( Pick::Set& set, int displayid )
    : set_( &set )
    , defscale_( set.disp_.pixsize_ )
    , scalemnuitem_("Size ...")
    , storemnuitem_("Store")
    , storeasmnuitem_("Store as ...")
{
    name_ = set_->name();
    displayid_ = displayid;
}


SubItem::~SubItem()
{}


void SubItem::prepareForShutdown()
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    if ( mgr.isChanged( setidx ) )
    {
	BufferString msg = "The annotation group ";
	msg += name();
	msg += " is not saved.\n\nDo you want to save it?";
	if ( uiMSG().notSaved( msg,0,false ) )
	    store();
    }
}


bool SubItem::init()
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
	    	    visserv->getObject(displayid_));
    if ( ld )
    {
	ld->setSetMgr( &Pick::SetMgr::getMgr( managerName()) );
	ld->setSet( set_ );
    }

    /*
    if (  displayid_==-1 )
    {
	visSurvey::AnnotDisplay* ad = visSurvey::AnnotDisplay::create();
	ad->clicked.notify( mCB(this,SubItem,clickCB) );
	ad->mousemoved.notify( mCB(this,SubItem,mouseMoveCB) );
	ad->rightclicked.notify( mCB(this,SubItem,rightclickCB) );
	visserv->addObject( ad, sceneID(), true );
	displayid_ = ad->id();
	ad->setName( name_ );
    }

    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return false;
    */

    return uiODDisplayTreeItem::init();
}


void SubItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    const bool islocked = visserv->isLocked( displayid_ );
    uiODDisplayTreeItem::createMenuCB(cb);
    if ( hasScale() )
	mAddMenuItem(menu,&scalemnuitem_,!islocked,false);

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mAddMenuItem(menu,&storemnuitem_,mgr.isChanged(setidx),false);
    mAddMenuItem(menu,&storeasmnuitem_,true,false);
}


void SubItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->menuID() != displayID() )
	return;

    mDynamicCastGet( visSurvey::LocationDisplay*,ld,
	    	     visserv->getObject(displayid_));
    if ( !ld ) return;

    uiODDisplayTreeItem::handleMenuCB(cb);
    if ( mnuid==scalemnuitem_.id )
    {
	menu->setIsHandled(true);
	uiDialog dlg( getUiParent(), uiDialog::Setup("Set size") );
	uiSliderExtra* sliderfld = new uiSliderExtra( &dlg, 
			uiSliderExtra::Setup("Size").nrdec(1).logscale(true) );
	sliderfld->sldr()->setMinValue( 0.1 );
	sliderfld->sldr()->setMaxValue( 10 );
	sliderfld->sldr()->setValue( 1 );
	sliderfld->sldr()->valueChanged.notify( mCB(this,SubItem,scaleChg) );
	dlg.go();
	defscale_ *= sliderfld->sldr()->getValue();
    }
    else if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled(true);
	store();
    }
    else if ( mnuid==storeasmnuitem_.id )
    {
	menu->setIsHandled(true);
	storeAs();
    }
}


void SubItem::store() const
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );

    const int setidx = mgr.indexOf( *set_ );
    PtrMan<IOObj> ioobj = IOM().get( mgr.id(setidx) );
    if ( !ioobj )
    {
	storeAs( true );
	return;
    }

    BufferString bs;
    if ( !PickSetTranslator::store( *set_, ioobj, bs ) )
	uiMSG().error(bs);
    else
	mgr.setUnChanged( setidx );
}


bool SubItem::doesNameExist( const char* nm, const char* mannm )
{
    IOM().to( PickSetTranslatorGroup::ioContext().stdSelKey() );
    PtrMan<IOObj> local = IOM().getLocal( nm );
    if ( !local )
	return false;
    
    const char* type = local->pars()[sKey::Type];
    if ( type && !strcmp( type, mannm ))
	return true;

    return false;
}


char SubItem::createIOEntry( const char* nm, bool overwrite, MultiID& mid,
			     const char* mannm )
{
    if ( !overwrite && doesNameExist(nm,mannm) )
	return 0;

    CtxtIOObj ctio( PickSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    ctio.ctxt.maychdir = false;
    ctio.ctxt.parconstraints.set( sKey::Type, mannm );
    ctio.ctxt.includeconstraints = true;
    ctio.setName( nm );
    ctio.fillObj();
    if ( !ctio.ioobj )
	return -1;

    mid = ctio.ioobj->key();
    delete ctio.ioobj;
    return 1;
}


void SubItem::storeAs( bool trywitoutdlg ) const
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );

    const char* name = set_->name();
    MultiID mid;

    if ( trywitoutdlg )
    {
	const char res = createIOEntry( name, false, mid, managerName() );
	if ( res==-1 )
	    return;
	if ( res==0 )
	{
	    storeAs( false );
	    return;
	}
    }
    else
    {
	CtxtIOObj ctio( PickSetTranslatorGroup::ioContext() );
	ctio.ctxt.forread = false;
	ctio.ctxt.maychdir = false;
	ctio.ctxt.parconstraints.set( sKey::Type, managerName() );
	ctio.ctxt.includeconstraints = true;
	ctio.setName( name );
	uiIOObjSelDlg dlg( getUiParent(), ctio );
	if ( !dlg.go() )
	{
	    delete ctio.ioobj;
	    return;
	}

	mid = ctio.ioobj->key();
    }

    mgr.setID( setidx, mid );
    store();
}


void SubItem::setScale( float ns )
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
	    	    visserv->getObject(displayid_));
    if ( !ld ) return;

    const int newscale = mNINT( ns );
    Pick::Set* set = ld->getSet();
    if ( set->disp_.pixsize_==newscale )
	return;

    set->disp_.pixsize_ = newscale;

    Pick::SetMgr::getMgr( managerName() ).reportDispChange( this, *set );
}


void SubItem::setColor( Color nc )
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
	    	    visserv->getObject(displayid_));
    if ( !ld ) return;

    Pick::Set* set = ld->getSet();
    if ( set->disp_.color_==nc )
	return;

    set->disp_.color_ = nc;

    Pick::SetMgr::getMgr( managerName() ).reportDispChange( this, *set );
}


void SubItem::scaleChg( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,slider,cb);
    if ( !slider ) return;
    const float newscale = defscale_ * slider->getValue();
    setScale( newscale );
}


TextSubItem::TextSubItem( Pick::Set& pck, int displayid )
    : SubItem(pck,displayid)
{
    defscale_ = 10;
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.locationChanged.notify( mCB(this, TextSubItem, pickAddedCB ) );
}


bool TextSubItem::init()
{
    if (  displayid_==-1 )
    {
	visSurvey::CalloutDisplay* cd =
	    visSurvey::CalloutDisplay::create();
	visserv->addObject( cd, sceneID(), true );
	displayid_ = cd->id();
	cd->setName( name_ );
    }

    mDynamicCastGet(visSurvey::CalloutDisplay*,cd,
	    visserv->getObject(displayid_))
    if ( !cd ) return false;
    cd->rightClicked()->notify( mCB(this,TextSubItem,rightclickCB) );

    return SubItem::init();
}


void TextSubItem::pickAddedCB( CallBacker* cb )
{
    mDynamicCastGet( Pick::SetMgr::ChangeData*, cd, cb );
    if ( !cd || cd->set_!=set_ ||
	 cd->ev_!=Pick::SetMgr::ChangeData::Added ) return;

    Pick::Location& loc = (*set_)[cd->loc_];
    if ( editText(prevtxt_) )
    {
	if ( !loc.text ) loc.text = new BufferString( prevtxt_ );
	else (*loc.text) = prevtxt_;
	//Trigger CB?
    }
}


void TextSubItem::rightclickCB( CallBacker* cb )
{
    mDynamicCastGet( visBase::DataObject*, dataobj, cb );
    const TypeSet<int>* path = dataobj->rightClickedPath();

    if ( !path ) return;
    mDynamicCastGet( visSurvey::LocationDisplay*,ld,
		     visserv->getObject(displayid_) );

    int pickidx = -1;
    for ( int idx=path->size()-1; idx>=0; idx-- )
    {
	pickidx = ld->getPickIdx( visserv->getObject((*path)[idx]) );
	if ( pickidx!=-1 )
	    break;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Change text ..."), 0 );
    const int mnusel = mnu.exec();
    if ( mnusel == 0 )
    {
	BufferString text = *(*set_)[pickidx].text;
	if ( editText(text) )
	{
	    *(*set_)[pickidx].text = text;

	    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Changed,
		    		      set_, pickidx );

	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.reportChange( this, cd );
	}
    }
}


const char* TextSubItem::parentType() const
{ return typeid(TextParentItem).name(); }


bool TextSubItem::editText( BufferString& str )
{
    uiGenInputDlg dlg( getUiParent(), "Text", "Text",
		       new StringInpSpec( (const char*) str ) );
    if ( !dlg.go() ) return false;
    str = dlg.text();
    return true;
}


void TextSubItem::setScale( float newscale )
{
    mDynamicCastGet( visSurvey::CalloutDisplay*,cd,
		     visserv->getObject(displayid_) );

    mDynamicCastGet(visSurvey::Scene*,scene,
		    applMgr()->visServer()->getObject(sceneID()));

    const float zscale = scene->getZScale();
    cd->setScale( Coord3(newscale,newscale,2*newscale/zscale) );
}

/*
void TextSubItem::getPickLocations( Pick::Set& ps, BufferString& tp )
{
    tp = "Text";
    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    ps.setName( ad->name() );
    ps.disp_.pixsize_ = (int)defscale_;
    for ( int idx=0; idx<ad->nrObjects(); idx++ )
    {
	mDynamicCastGet(visBase::Callout*,callout,ad->getObject(idx))
	if ( callout )
	{
	    Pick::Location loc( callout->getLocation() );
	    loc.dir = Sphere( callout->getMarkerLocation() );
	    ps += loc;
	    Pick::Location& pl = ps[ps.size()-1];
	    pl.setText( "T", callout->getText() );
	    pl.setText( "O", BufferString(callout->getOrientation()) );
	}
    }
}


void TextSubItem::setPickLocations( const Pick::Set& ps )
{
    const int nrpicks = ps.size();
    if ( nrpicks == 0 ) return;

    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    defscale_ = ps.disp_.pixsize_;
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const Pick::Location& loc = ps[idx];
	Sphere dir( loc.dir );

	visBase::Callout* callout = visBase::Callout::create();
	BufferString res;
	loc.getKey( "T", res );
	callout->setText( res );
	callout->setLocation( loc.pos, Coord3(dir.radius,dir.theta,dir.phi) );
	loc.getKey( "O", res );
	callout->setOrientation( atoi(res) );

	const float zscale = visSurvey::STM().currentScene()->getZScale();
	callout->setScale( Coord3(defscale_,defscale_,2*defscale_/zscale) );
	ad->addObject( callout );
    }
}
*/


const char* SymbolSubItem::parentType() const
{ return typeid(SymbolParentItem).name(); }


SymbolSubItem::SymbolSubItem( Pick::Set& pck, int displayid )
    : SubItem( pck, displayid )
    , propmnuitem_( "Properties ..." )
    , arrowtype_( 2 )
{
    defscale_ = set_->disp_.pixsize_ = 100;
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.reportDispChange( this, *set_ );
}



bool SymbolSubItem::init()
{
    if (  displayid_==-1 )
    {
	visSurvey::ArrowAnnotationDisplay* ad =
	    visSurvey::ArrowAnnotationDisplay::create();
	//ad->clicked.notify( mCB(this,SubItem,clickCB) );
	//ad->mousemoved.notify( mCB(this,SubItem,mouseMoveCB) );
	//ad->rightclicked.notify( mCB(this,SubItem,rightclickCB) );
	visserv->addObject( ad, sceneID(), true );
	displayid_ = ad->id();
	ad->setName( name_ );
    }

    mDynamicCastGet(visSurvey::ArrowAnnotationDisplay*,ad,
	    visserv->getObject(displayid_))
    if ( !ad ) return false;

    return SubItem::init();
}


void SymbolSubItem::clickCB( CallBacker* )
{
    /*
    mDynamicCastGet(visSurvey::ArrowAnnotationDisplay*,ad,
	    	    visserv->getObject(displayid_))
    if ( !ad ) return;

    ad->need2Clicks( true );
    const Coord3 pos = ad->getPickedWorldPos();
    if ( ad->hadFirstClick() && !ad->hadSecondClick() )
    {
	visBase::Arrow* arrow = visBase::Arrow::create();
	ad->addObject( arrow );
	arrow->setPosition( pos );
	const float zscale = visSurvey::STM().currentScene()->getZScale();
	arrow->setScale( Coord3(defscale_,defscale_,2*defscale_/zscale) );
	arrow->setOrientation( ad->getOrientation() );
	arrow->setColor( color_ );
	arrow->setType( (visBase::Arrow::Type)arrowtype_ );
    }
    else if ( ad->hadSecondClick() )
	ad->reset();
	*/
}


void SymbolSubItem::mouseMoveCB( CallBacker* )
{
    /*
    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    const Coord3 mousepos = ad->getPickedWorldPos();
    if ( ad->hadFirstClick() && !ad->hadSecondClick() )
    {
	const int nrobj = ad->nrObjects();
	mDynamicCastGet(visBase::Arrow*,arrow,ad->getObject(nrobj-1))
	if ( arrow ) arrow->setTailPosition( mousepos );
    }
    */
}


void SymbolSubItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    SubItem::createMenuCB( cb );
    mAddMenuItem(menu,&propmnuitem_,true,false );
}


void SymbolSubItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->menuID() != displayID() )
	return;

    SubItem::handleMenuCB( cb );
    if ( mnuid==propmnuitem_.id )
    {
	menu->setIsHandled(true);

	uiArrowDialog dlg( getUiParent() );
	dlg.setColor( set_->disp_.color_ );
	dlg.setArrowType( arrowtype_ );
	mDynamicCastGet(visSurvey::ArrowAnnotationDisplay*,ad,
			visserv->getObject(displayid_));
	dlg.setLineWidth( ad->getLineWidth() );
	dlg.propertyChange.notify( mCB(this,SymbolSubItem,propertyChange) );
	dlg.go();
	if ( set_->disp_.color_!=dlg.getColor() )
	{
	    set_->disp_.color_ = dlg.getColor();
	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.reportDispChange( this, *set_ );
	}

	arrowtype_ = dlg.getArrowType();
	defscale_ *= dlg.getScale();
    }
}


void SymbolSubItem::propertyChange( CallBacker* cb )
{
    mDynamicCastGet(uiArrowDialog*,dlg,cb)
    if ( !dlg ) return;

    const int arrowtype = dlg->getArrowType();
    setScale( defscale_*dlg->getScale() );
    setColor( dlg->getColor() );

    mDynamicCastGet(visSurvey::ArrowAnnotationDisplay*,ad,
	    	    visserv->getObject(displayid_));

    ad->setType( (visSurvey::ArrowAnnotationDisplay::Type) arrowtype );
    ad->setLineWidth( dlg->getLineWidth() );
}



// Image item
/*
const char* ImageSubItem::parentType() const
{ return typeid(ImageParentItem).name(); }


void ImageSubItem::clickCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    if ( imgfilenm_=="" && !selectFileName(imgfilenm_) )
	return;

    visBase::Image* image = visBase::Image::create();
    ad->addObject( image );
    image->setImageFileName( imgfilenm_ );
    image->setPosition( ad->getPickedWorldPos() );
    image->setOrientation( ad->getOrientation() );
    const float zscale = visSurvey::STM().currentScene()->getZScale();
    image->setScale( Coord3(defscale_,defscale_,2*defscale_/zscale) );
}


void ImageSubItem::setScale( float newscale )
{
    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    const float zscale = visSurvey::STM().currentScene()->getZScale();
    for ( int idx=0; idx<ad->nrObjects(); idx++ )
    {
	mDynamicCastGet(visBase::Image*,image,ad->getObject(idx))
	if ( image ) 
	    image->setScale( Coord3(newscale,newscale,2*newscale/zscale) );
    }
}


void ImageSubItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(MenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    SubItem::createMenuCB( cb );
    mAddMenuItem(menu,&filemnuitem_,true,false);
}


void ImageSubItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->menuID() != displayID() )
	return;

    SubItem::handleMenuCB( cb );
    if ( mnuid==filemnuitem_.id )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::AnnotDisplay*,ad,
			visserv->getObject(displayid_))
	if ( !ad ) return;

	if ( !selectFileName(imgfilenm_) ) return;

	for ( int idx=0; idx<ad->nrObjects(); idx++ )
	{
	    mDynamicCastGet(visBase::Image*,image,ad->getObject(idx))
	    if ( image )
		image->setImageFileName( imgfilenm_ );
	}
    }
}


bool ImageSubItem::selectFileName( BufferString& fnm )
{
    BufferString filter = "JPEG (*.jpg *.jpeg);;PNG (*.png)";
    uiFileDialog dlg( getUiParent(), true, fnm, filter );
    if ( !dlg.go() ) return false;

    fnm = dlg.fileName();
    return true;
}


void ImageSubItem::getPickLocations( Pick::Set& ps, BufferString& type )
{
    type = "Image";
    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    ps.setName( ad->name() );
    ps.disp_.pixsize_ = (int)defscale_;
    for ( int idx=0; idx<ad->nrObjects(); idx++ )
    {
	mDynamicCastGet(visBase::Image*,image,ad->getObject(idx))
	if ( !image ) continue;

	Pick::Location loc( image->getPosition() );
	loc.setText( "T", image->imageFileName() );
	loc.setText( "O", BufferString(image->getOrientation()) );
	ps += loc;
    }
}


void ImageSubItem::setPickLocations( const Pick::Set& ps )
{
    const int nrpicks = ps.size();
    if ( nrpicks == 0 ) return;

    mDynamicCastGet(visSurvey::AnnotDisplay*,ad,visserv->getObject(displayid_))
    if ( !ad ) return;

    defscale_ = ps.disp_.pixsize_;
    for ( int idx=0; idx<nrpicks; idx++ )
    {
	const Pick::Location& loc = ps[idx];

	visBase::Image* image = visBase::Image::create();
	ad->addObject( image );

	BufferString res;
	loc.getKey( "T", res );
	image->setImageFileName( res );
	imgfilenm_ = res;
	loc.getKey( "O", res );
	image->setOrientation( atoi(res) );
	image->setPosition( loc.pos );

	const float zscale = visSurvey::STM().currentScene()->getZScale();
	image->setScale( Coord3(defscale_,defscale_,2*defscale_/zscale) );
    }
}

*/


}; // namespace Annotations
