/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2005
________________________________________________________________________

-*/

#include "treeitem.h"
#include "randcolor.h"
#include "survinfo.h"

#include "uiarrowdlg.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uifileselector.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uislider.h"
#include "uitextedit.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "vissurvscene.h"

#include "visannotimage.h"
#include "visarrow.h"
#include "viscallout.h"
#include "visscalebar.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "dbman.h"
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


ParentTreeItem::ParentTreeItem()
    : uiTreeItem("Annotations")
{}


ParentTreeItem::~ParentTreeItem()
{}


bool ParentTreeItem::rightClick( uiTreeViewItem* itm )
{
    if ( itm == uitreeviewitem_ && !uitreeviewitem_->isOpen() )
        uitreeviewitem_->setOpen( true );

    return uiTreeItem::rightClick( itm );
}


int ParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>(uiODTreeTop::sceneidkey(),sceneid) )
	return -1;
    return sceneid;
}


bool ParentTreeItem::init()
{
    bool ret = uiTreeItem::init();
    if ( !ret ) return false;

    addChild( new ArrowParentItem(), true );
    addChild( new ImageParentItem(), true );
    addChild( new ScaleBarParentItem(), true );
    addChild( new TextParentItem(), true );
    getItem()->setOpen( false );
    return true;
}


const char* ParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }


// TreeItemFactory +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

uiTreeItem* TreeItemFactory::create( int visid, uiTreeItem* treeitem ) const
{
    visBase::DataObject* dataobj =
	ODMainWin()->applMgr().visServer()->getObject(visid);
    if ( !dataobj ) return 0;

    mDynamicCastGet(visSurvey::LocationDisplay*,ld, dataobj );
    if ( !ld ) return 0;

    if ( treeitem->findChild( visid ) )
	return 0;

    const DBKey mid = ld->getDBKey();
    const char* factoryname = 0;

    mDynamicCastGet(ImageDisplay*,id,dataobj);
    if ( id ) factoryname = ImageSubItem::sKeyManager();

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( factoryname );
    int setidx = mgr.indexOf(mid);
    if ( setidx==-1 )
    {
	PtrMan<IOObj> ioobj = mid.getIOObj();
	Pick::Set* ps = new Pick::Set;
	BufferString bs;
	PickSetTranslator::retrieve(*ps,ioobj,true,bs);
	mgr.set( mid, ps );

	setidx = mgr.indexOf(mid);

    }

    if ( id ) return new ImageSubItem( mgr.get(setidx), visid );

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
    mgr.setToBeRemoved.notify( mCB(this,AnnotTreeItem,setRemovedCB));
    return true;
}


void AnnotTreeItem::prepareForShutdown()
{
    Pick::SetMgr::getMgr( managerName() ).removeCBs( this );
}


void AnnotTreeItem::addPickSet( Pick::Set* ps )
{
    if ( !ps ) return;

    uiTreeItem* item = createSubItem( -1, *ps );
    addChild( item, true );
}


void AnnotTreeItem::removePickSet( Pick::Set* ps )
{
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

void AnnotTreeItem::setRemovedCB( CallBacker* cb )
{
    mDynamicCastGet(Pick::Set*,ps,cb)
    if ( !ps ) return;

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(SubItem*,itm,children_[idx])
	    if ( !itm ) continue;
	if ( itm->getSet() == ps )
	{
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    uiTreeItem::removeChild( itm );
	    return;
	}
    }

}


bool AnnotTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    applMgr()->visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	uiMSG().error( "Cannot add Annotations to this scene (yet)" );
	return false;
    }

    uiMenu mnu( getUiParent(), "Action" );
    BufferString addtxt = "&Add "; addtxt += typestr_; addtxt += " group ...";
    mnu.insertItem( new uiAction(addtxt), 0 );
    mnu.insertItem( new uiAction("&Load ..."), 1 );

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

	    if ( SubItem::doesNameExist( txt ) &&
	         !uiMSG().askOverwrite("An object with that name already"
			" exists.\nDo you wish to overwrite it?" ) )
		continue;

	    DBKey mid;
	    if ( SubItem::createIOEntry(txt,true,mid,managerName())!=1 )
		return false;

	    Pick::Set* set = new Pick::Set(txt);
	    set->disp_.color_ = getRandStdDrawColor();
	    if ( defScale()!=-1 ) set->disp_.pixsize_ = defScale();
	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.set( mid, set );
	    uiTreeItem* item = createSubItem( -1, *set );
	    addChild( item, true );
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
    ctio->ctxt.toselect.require_.set( sKey::Type(), managerName(), oldSelKey());
    uiIOObjSelDlg dlg( getUiParent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	mDelCtioRet;

    if ( defScale()!=-1 )
	ps.disp_.pixsize_= defScale();

    BufferString bs;
    if ( !PickSetTranslator::retrieve(ps,dlg.ioObj(),true,bs) )
    { uiMSG().error( bs ); mDelCtioRet; }

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    if ( mgr.indexOf(dlg.ioObj()->key() ) == -1 )
    {
	mgr.set( dlg.ioObj()->key(), &ps );
	const int setidx = mgr.indexOf( ps );
	mgr.setUnChanged( setidx );
	addPickSet( &ps );
    }
    return true;
}

// SubItem ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

SubItem::SubItem( Pick::Set& set, int displayid )
    : set_( &set )
    , defscale_( set.disp_.pixsize_ )
    , scalemnuitem_("Size ...")
    , storemnuitem_("Save")
    , storeasmnuitem_("Save as ...")
{
    name_ = set_->name();
    displayid_ = displayid;

    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
}


SubItem::~SubItem()
{
}


void SubItem::prepareForShutdown()
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    if ( mgr.isChanged(setidx) )
    {
	BufferString msg = "The annotation group ";
	msg += name();
	msg += " is not saved.\n\nDo you want to save it?";
	if ( uiMSG().askSave(msg,false) )
	    store();
    }

    removeStuff();
}


bool SubItem::init()
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
		    visserv_->getObject(displayid_));
    if ( ld )
    {
	ld->setSetMgr( &Pick::SetMgr::getMgr( managerName()) );
	ld->setSet( set_ );
    }

    return uiODDisplayTreeItem::init();
}


void SubItem::createMenu( MenuHandler* menu, bool istb )
{
    if ( !menu || menu->menuID()!=displayID() )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( hasScale() )
	mAddMenuOrTBItem(istb,0,menu,&scalemnuitem_,!islocked,false);

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mAddMenuOrTBItem(istb,menu,menu,&storemnuitem_,
		     mgr.isChanged(setidx),false);
    mAddMenuOrTBItem(istb,0,menu,&storeasmnuitem_,true,false);
}


void SubItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
		    visserv_->getObject(displayid_));
    if ( !ld ) return;

    if ( mnuid==scalemnuitem_.id )
    {
	menu->setIsHandled(true);
	uiDialog dlg( getUiParent(), uiDialog::Setup("Set size","Size",
						     mNoHelpID) );
	uiSliderExtra* sliderfld = new uiSliderExtra( &dlg,
			uiSliderExtra::Setup("Size").nrdec(1).logscale(true),
			"Size" );
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
	updateColumnText( 0 );
    }
}


void SubItem::store() const
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );

    const int setidx = mgr.indexOf( *set_ );
    PtrMan<IOObj> ioobj = mgr.id(setidx).getIOObj();
    if ( !ioobj )
	{ storeAs( true ); return; }

    ioobj->pars().set( sKey::Type(), managerName() );
    ioobj->commitChanges();

    fillStoragePar( set_->pars_ );
    BufferString bs;
    if ( !PickSetTranslator::store( *set_, ioobj, bs ) )
	uiMSG().error(bs);
    else
	mgr.setUnChanged( setidx );
}


bool SubItem::doesNameExist( const char* nm )
{
    DBM().to( PickSetTranslatorGroup::ioContext().getSelDirID() );
    PtrMan<IOObj> local = DBM().getLocal( nm );
    return local;
}


char SubItem::createIOEntry( const char* nm, bool overwrite, DBKey& mid,
			     const char* mannm )
{
    if ( !overwrite && doesNameExist(nm) )
	return 0;

    CtxtIOObj ctio( PickSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = false;
    ctio.ctxt.toselect.require_.set( sKey::Type(), mannm );
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

    const char* nm = set_->name();
    DBKey mid;

    if ( trywitoutdlg )
    {
	const char res = createIOEntry( nm, false, mid, managerName() );
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
	ctio.ctxt.toselect.require_.set( sKey::Type(), managerName() );
	ctio.setName( nm );
	uiIOObjSelDlg dlg( getUiParent(), ctio );
	if ( !dlg.go() )
	{
	    delete ctio.ioobj;
	    return;
	}

	mid = dlg.selected( 0 );
    }

    mgr.setID( setidx, mid );
    store();
}


void SubItem::setScale( float ns )
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
		    visserv_->getObject(displayid_));
    if ( !ld ) return;

    const int newscale = mNINT32( ns );
    Pick::Set* set = ld->getSet();
    if ( set->disp_.pixsize_==newscale )
	return;

    set->disp_.pixsize_ = newscale;

    Pick::SetMgr::getMgr( managerName() ).reportDispChange( this, *set );
}


void SubItem::setColor( Color nc )
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
		    visserv_->getObject(displayid_));
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


void SubItem::removeStuff()
{
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    if ( setidx >= 0 )
    {
	mgr.set( mgr.id(setidx), 0 );
    }
    mgr.removeCBs( this );
}



TextSubItem::TextSubItem( Pick::Set& pck, int displayid )
    : SubItem(pck,displayid)
    , changetextmnuitem_("Change text ...")
    , changecolormnuitem_("Background color ...")
{
    defscale_ = 100;
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.locationChanged.notify( mCB(this,TextSubItem,pickAddedCB) );
}


bool TextSubItem::init()
{
    if (  displayid_==-1 )
    {
	CalloutDisplay* cd = CalloutDisplay::create();
	visserv_->addObject( cd, sceneID(), true );
	displayid_ = cd->id();
	cd->setName( name_ );
    }

    mDynamicCastGet(CalloutDisplay*,cd,visserv_->getObject(displayid_))
    if ( set_->pars_.get(sKeyBoxColor(),boxcolor_) )
	cd->setBoxColor( boxcolor_ );
    if ( !cd ) return false;

    defscale_ = set_->disp_.pixsize_;
    cd->setScale( defscale_ );

    //Read Old format orientation
    const Coord3 inldirvec( SI().binID2Coord().colDir(), 0 );
    const Coord3 crldirvec( SI().binID2Coord().rowDir(), 0 );
    const Sphere inldir = cartesian2Spherical( inldirvec, true );
    const Sphere crldir = cartesian2Spherical( crldirvec, true );
    for ( int idx=set_->size()-1; idx>=0; idx-- )
    {
	BufferString orientation;
	if ( (*set_)[idx].getText("O", orientation ) )
	{
	    if ( orientation[0] == '1' )
		(*set_)[idx].dir_ = crldir;
	    else
		(*set_)[idx].dir_ = inldir;
	}

	(*set_)[idx].unSetText("O");
    }

    return SubItem::init();
}


void TextSubItem::createMenu( MenuHandler* menu, bool istb )
{
    SubItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb )
	return;

    mDynamicCastGet(uiMenuHandler*,uimenu,menu)
    mAddMenuItem( menu, &changetextmnuitem_, uimenu && uimenu->getPath(),false);
    mAddMenuItem( menu, &changecolormnuitem_, true, false );
}


void TextSubItem::handleMenuCB( CallBacker* cb )
{
    SubItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    if ( mnuid==changetextmnuitem_.id )
    {
	menu->setIsHandled(true);
	const TypeSet<int>* path = menu->getPath();

	if ( !path ) return;
	mDynamicCastGet(visSurvey::LocationDisplay*,ld,
			visserv_->getObject(displayid_) );

	int pickidx = -1;
	for ( int idx=path->size()-1; idx>=0; idx-- )
	{
	    pickidx = ld->getPickIdx( visserv_->getObject((*path)[idx]) );
	    if ( pickidx!=-1 )
		break;
	}

	if ( pickidx==-1 )
	    return;

	BufferString text;
	BufferString url;
	(*set_)[pickidx].getText( CalloutDisplay::sKeyText(), text );
	(*set_)[pickidx].getText( CalloutDisplay::sKeyURL(), url );
	bool enab = url[0];
	if ( editText(text, url, enab ) )
	{
	    (*set_)[pickidx].setText( CalloutDisplay::sKeyText(), text.buf() );
	    if ( !enab ) (*set_)[pickidx].unSetText(CalloutDisplay::sKeyURL() );
	    else (*set_)[pickidx].setText( CalloutDisplay::sKeyURL(),url.buf());

	    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Changed,
					 set_, pickidx );

	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.reportChange( this, cd );
	}
    }
    else if ( mnuid==changecolormnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(CalloutDisplay*,cd,visserv_->getObject(displayid_))
	if ( !cd ) return;
	boxcolor_ = cd->getBoxColor();
	if ( selectColor(boxcolor_) )
	{
	    cd->setBoxColor( boxcolor_ );
	    Pick::SetMgr::getMgr(managerName()).reportDispChange( this, *set_ );
	}
    }
}


void TextSubItem::pickAddedCB( CallBacker* cb )
{
    mDynamicCastGet( Pick::SetMgr::ChangeData*, cd, cb );
    if ( !cd || cd->set_!=set_ ||
	 cd->ev_!=Pick::SetMgr::ChangeData::Added ) return;

    Pick::Location& loc = (*set_)[cd->loc_];
    BufferString url;
    bool enab;
    if ( editText(prevtxt_, url, enab ) )
    {
	loc.setText( CalloutDisplay::sKeyText(), prevtxt_.buf() );
	if ( !enab ) loc.unSetText(CalloutDisplay::sKeyURL() );
	else loc.setText( CalloutDisplay::sKeyURL(),url.buf());
    }
}


const char* TextSubItem::parentType() const
{ return typeid(TextParentItem).name(); }


class AnchorGroup : public uiGroup
{
public:
AnchorGroup( uiParent* p, const char* url, bool urlenabled )
    : uiGroup(p,"Anchor Group")
{
    linkfld_ = new uiGenInput( this, "Link to", StringInpSpec(url) );
    linkfld_->setWithCheck( true );
    linkfld_->setChecked( urlenabled );
    linkfld_->checked.notify( mCB(this,AnchorGroup,urlCheckCB) );

    but_ = new uiPushButton( this, "Select file", false );
    but_->activated.notify( mCB(this,AnchorGroup,butPush) );
    but_->attach( rightTo, linkfld_ );
    but_->setSensitive( urlenabled );
    setHAlignObj( linkfld_ );
}


void butPush( CallBacker* )
{
    uiFileSelector uifs( this, linkfld_->text() );
    if ( uifs.go() )
	linkfld_->setText( uifs.fileName() );
}


void urlCheckCB( CallBacker* )
{
    but_->setSensitive( linkfld_->isChecked() );
}

    uiGenInput*		linkfld_;
    uiPushButton*	but_;

};



class uiTextDialog : public uiDialog
{
public:
uiTextDialog( uiParent* p, const char* str, const char* url, bool urlenabled )
    : uiDialog(p,uiDialog::Setup("Text","Text",mNoHelpID))
{
    textedit = new uiTextEdit( this );
    textedit->setPrefWidthInChar( 20 );
    textedit->setPrefHeightInChar( 5 );
    textedit->setText( str );

    anchorgrp = new AnchorGroup( this, url, urlenabled );
    anchorgrp->attach( leftAlignedBelow, textedit );
    textedit->setFocus();
}


const char* text() const
{ return textedit->text(); }

const char* getUrl() const
{ return anchorgrp->linkfld_->text(); }

bool isUrlEnabled() const
{
    const char* url = getUrl();
    return url && *url && anchorgrp->linkfld_->isChecked();
}

    uiTextEdit*         textedit;
    AnchorGroup*        anchorgrp;

};


bool TextSubItem::editText( BufferString& str, BufferString& url, bool& enab )
{
    uiTextDialog dlg( getUiParent(), str, url, enab );
    if ( !dlg.go() ) return false;

    str = dlg.text();
    url = dlg.getUrl();
    enab = dlg.isUrlEnabled();
    return true;
}


void TextSubItem::setScale( float newscale )
{
    SubItem::setScale( newscale );
    mDynamicCastGet(CalloutDisplay*,cd,visserv_->getObject(displayid_));
    cd->setScale( newscale );
}


void TextSubItem::fillStoragePar( IOPar& par ) const
{
    SubItem::fillStoragePar( par );
    BufferString colstr;
    boxcolor_.fill( colstr );
    par.set( sKeyBoxColor(), colstr.buf() );
}


// ArrowSubItem
const char* ArrowSubItem::parentType() const
{ return typeid(ArrowParentItem).name(); }


ArrowSubItem::ArrowSubItem( Pick::Set& pck, int displayid )
    : SubItem( pck, displayid )
    , propmnuitem_( "Properties ..." )
    , arrowtype_( 2 )
{
    defscale_ = set_->disp_.pixsize_;
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.reportDispChange( this, *set_ );

    propmnuitem_.iconfnm = "disppars";
}


bool ArrowSubItem::init()
{
    if (  displayid_==-1 )
    {
	ArrowDisplay* ad = ArrowDisplay::create();
	visserv_->addObject( ad, sceneID(), true );
	displayid_ = ad->id();
	ad->setName( name_ );
    }

    mDynamicCastGet(ArrowDisplay*,ad,visserv_->getObject(displayid_))
    if ( !ad ) return false;

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    PtrMan<IOObj> ioobj = mgr.id(setidx).getIOObj();
    if ( !ioobj )
	return false;

    if ( !ioobj->pars().get(sKeyArrowType(),arrowtype_) )
	set_->pars_.get( sKeyArrowType(), arrowtype_ );
    ad->setType( (ArrowDisplay::Type)arrowtype_ );

    int linewidth = 2;
    if ( !ioobj->pars().get(sKeyLineWidth(),linewidth) )
	set_->pars_.get( sKeyLineWidth(), linewidth );
    ad->setLineWidth( linewidth );

    //Read Old format orientation
    for ( int idx=set_->size()-1; idx>=0; idx-- )
    {
	BufferString orientation;
	if ( (*set_)[idx].getText("O", orientation ) )
	{
	    Sphere& dir = (*set_)[idx].dir_;
	    if ( orientation[0] == '2' )
	    {
		dir.phi = (float) (-M_PI_2-dir.phi);
		dir.theta = M_PI_2;
	    }
	    else
	    {
		dir.phi = (float) (M_PI_2-dir.phi);
		dir.theta -= M_PI_2;
	    }
	}

	delete (*set_)[idx].text_;
	(*set_)[idx].text_ = 0;
    }

    return SubItem::init();
}


void ArrowSubItem::fillStoragePar( IOPar& par ) const
{
    SubItem::fillStoragePar( par );
    mDynamicCastGet(ArrowDisplay*,ad,visserv_->getObject(displayid_))
    par.set( sKeyArrowType(), (int) ad->getType() );
    par.set( sKeyLineWidth(), ad->getLineWidth() );
}


void ArrowSubItem::createMenu( MenuHandler* menu, bool istb )
{
    SubItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() )
	return;

    mAddMenuOrTBItem(istb,menu,menu,&propmnuitem_,true,false );
}


void ArrowSubItem::handleMenuCB( CallBacker* cb )
{
    SubItem::handleMenuCB( cb );
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    if ( mnuid==propmnuitem_.id )
    {
	menu->setIsHandled(true);

	uiArrowDialog dlg( getUiParent() );
	dlg.setColor( set_->disp_.color_ );
	dlg.setArrowType( arrowtype_ );
	mDynamicCastGet(ArrowDisplay*,ad,visserv_->getObject(displayid_));
	dlg.setLineWidth( ad->getLineWidth() );
	dlg.propertyChange.notify( mCB(this,ArrowSubItem,propertyChange) );
	dlg.go();
	if ( set_->disp_.color_!=dlg.getColor() )
	{
	    set_->disp_.color_ = dlg.getColor();
	    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	    mgr.reportDispChange( this, *set_ );
	}

	arrowtype_ = dlg.getArrowType();
	defscale_ *= dlg.getScale();
	updateColumnText( 1 );
    }
}


void ArrowSubItem::propertyChange( CallBacker* cb )
{
    mDynamicCastGet(uiArrowDialog*,dlg,cb)
    if ( !dlg ) return;

    const int arrowtype = dlg->getArrowType();
    setScale( defscale_*dlg->getScale() );
    setColor( dlg->getColor() );

    mDynamicCastGet(ArrowDisplay*,ad,visserv_->getObject(displayid_));
    ad->setType( (ArrowDisplay::Type) arrowtype );
    ad->setLineWidth( dlg->getLineWidth() );

    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mgr.setUnChanged( setidx, false );
}



// Image item
ImageSubItem::ImageSubItem( Pick::Set& pck, int displayid )
    : SubItem( pck, displayid )
    , filemnuitem_( "Select image ..." )
{
    defscale_ = set_->disp_.pixsize_;
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    mgr.reportDispChange( this, *set_ );
}



bool ImageSubItem::init()
{
    ImageDisplay* id = 0;
    if (  displayid_==-1 )
    {
	id = ImageDisplay::create();
	visserv_->addObject( id, sceneID(), true );
	displayid_ = id->id();
	id->setName( name_ );
    }
    else
    {
	mDynamicCast(ImageDisplay*,id,visserv_->getObject(displayid_))
    }

    if ( !id ) return false;

    id->needFileName.notifyIfNotNotified(
			mCB(this,ImageSubItem,retrieveFileName) );

    BufferString filename;
    set_->pars_.get( sKey::FileName(), filename );
    if ( filename.isEmpty() )
    {
	Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
	const int setidx = mgr.indexOf( *set_ );
	PtrMan<IOObj> ioobj = mgr.id(setidx).getIOObj();
	if ( ioobj )
	    ioobj->pars().get(sKey::FileName(), filename );
    }

    if ( !filename.isEmpty() )
	id->setFileName( filename.buf() );

    return SubItem::init();
}



const char* ImageSubItem::parentType() const
{ return typeid(ImageParentItem).name(); }


void ImageSubItem::fillStoragePar( IOPar& par ) const
{
    SubItem::fillStoragePar( par );
    mDynamicCastGet(ImageDisplay*,id,visserv_->getObject(displayid_))
    par.set( sKey::FileName(), id->getFileName() );
}


void ImageSubItem::createMenu( MenuHandler* menu, bool istb )
{
    if ( !menu || menu->menuID()!=displayID() )
	return;

    SubItem::createMenu( menu, istb );
    mAddMenuOrTBItem(istb,0,menu,&filemnuitem_,true,false);
}


void ImageSubItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    SubItem::handleMenuCB( cb );
    if ( mnuid==filemnuitem_.id )
    {
	menu->setIsHandled(true);
	selectFileName();
    }
}


void ImageSubItem::retrieveFileName( CallBacker* cb )
{
    selectFileName();
}


void ImageSubItem::updateColumnText(int col)
{
    if ( col!=1 )
	uiODDisplayTreeItem::updateColumnText(col);
}


void ImageSubItem::selectFileName() const
{
    mDynamicCastGet(Annotations::ImageDisplay*,id,
		    visserv_->getObject(displayid_))
    if ( !id ) return;

    uiFileSelector::Setup fssu( id->getFileName() );
    OD::GetSupportedImageFormats( fssu.formats_, true );
    uiFileSelector uifs( getUiParent(), fssu );
    if ( !uifs.go() )
	return;

    id->setFileName( uifs.fileName() );
    Pick::SetMgr& mgr = Pick::SetMgr::getMgr( managerName() );
    const int setidx = mgr.indexOf( *set_ );
    mgr.setUnChanged( setidx, false );
}


} // namespace Annotations

