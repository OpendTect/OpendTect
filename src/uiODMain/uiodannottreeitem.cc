/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2005
________________________________________________________________________

-*/

#include "uiodannottreeitem.h"
#include "randcolor.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uicolor.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uislider.h"
#include "uitextedit.h"
#include "uitreeview.h"
#include "uivispartserv.h"
#include "uipicksetsel.h"
#include "vislocationdisplay.h"
#include "vissurvscene.h"

#include "ioobjctxt.h"
#include "ioobj.h"
#include "iopar.h"
#include "picksetmanager.h"
#include "ptrman.h"
#include "uicolor.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"



uiODAnnotParentTreeItem::uiODAnnotParentTreeItem()
    : uiTreeItem( uiStrings::sAnnotation(mPlural) )
{
}


uiODAnnotParentTreeItem::~uiODAnnotParentTreeItem()
{
}


bool uiODAnnotParentTreeItem::rightClick( uiTreeViewItem* itm )
{
    if ( itm == uitreeviewitem_ && !uitreeviewitem_->isOpen() )
        uitreeviewitem_->setOpen( true );

    return uiTreeItem::rightClick( itm );
}


int uiODAnnotParentTreeItem::sceneID() const
{
    mDynamicCastGet(uiODSceneTreeTop*,treetop,parent_);
    return treetop ? treetop->sceneID() : -1;
}


bool uiODAnnotParentTreeItem::init()
{
    if ( !uiTreeItem::init() )
	return false;

    addChild( new ArrowParentItem(), true );
    addChild( new ImageParentItem(), true );
    addChild( new ScaleBarParentItem(), true );
    getItem()->setOpen( false );
    return true;
}


const char* uiODAnnotParentTreeItem::parentType() const
{
    return typeid(uiODSceneTreeTop).name();
}


// TreeItemFactory +++++++++++++++++++++++++++++++++++++++++++++++++++++++++

uiTreeItem* uiODAnnotTreeItemFactory::create( int visid,
					      uiTreeItem* treeitem ) const
{
    return 0;
}


// Base uiODAnnotTreeItem ++++++++++++++++++++++++++++++++++++++++++++++++++++++

uiODAnnotTreeItem::uiODAnnotTreeItem( const uiString& type )
    : uiODSceneTreeItem(type)
    , typestr_(type)
{
}


uiODAnnotTreeItem::~uiODAnnotTreeItem()
{
}


const char* uiODAnnotTreeItem::parentType() const
{
    return typeid(uiODAnnotParentTreeItem).name();
}


bool uiODAnnotTreeItem::init()
{
    return uiODSceneTreeItem::init();
}


bool uiODAnnotTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
		    applMgr()->visServer()->getObject(sceneID()));
    if ( scene && scene->getZAxisTransform() )
    {
	//TODO
	mTIUiMsg().error( tr("Cannot add Annotations to this scene") );
	return false;
    }

    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    const uiString addtxt = m3Dots(tr("New %1 group").arg(typestr_));
    mnu.insertAction( new uiAction(addtxt), 0 );
    mnu.insertAction( new uiAction(m3Dots(uiStrings::sAdd())), 1 );
    addStandardItems( mnu );

    const int mnusel = mnu.exec();
    if ( mnusel < 0 )
	return false;

    RefMan<Pick::Set> newps = 0;
    if ( mnusel == 0 )
    {
	const uiString title = tr("%1 Annotations").arg(typestr_);
	uiGenInputDlg dlg( getUiParent(), title, tr("Group name"),
			       new StringInpSpec );
	dlg.setCaption( uiStrings::sAnnotation(mPlural) );

	while ( true )
	{
	    if ( !dlg.go() )
		return false;

	    const char* txt = dlg.text();
	    if ( !txt || !*txt )
		continue;

	    const bool exists = Pick::SetMGR().nameExists( txt );
	    if ( exists && !mTIUiMsg().askOverwrite(
		 tr("An object with that name already "
		    "exists.\nDo you wish to overwrite it?")))
		continue;

	    newps = makeNewSet( txt );
	    break;
	}
    }
    else if ( mnusel == 1 )
    {
	newps = readExistingSet();
    }
    else
    {
	handleStandardItems( mnusel );
	return true;
    }

    if ( !newps )
	return false;

    uiTreeItem* item = createSubItem( -1, *newps );
    addChild( item, true );
    return true;
}

#define mDelCtioRet { delete ctio->ioobj_; delete ctio; return false; }

Pick::Set* uiODAnnotTreeItem::makeNewSet( const char* nm ) const
{
    RefMan<Pick::Set> ps = new Pick::Set( nm, getCategory() );
    ps->setDispColor( getRandStdDrawColor() );
    if ( defScale() >= 0 )
	ps->setDispSize( defScale() );

    SilentTaskRunnerProvider trprov;
    uiString errmsg = Pick::SetMGR().store( *ps, trprov );
    if ( !errmsg.isEmpty() )
	{ mTIUiMsg().error( errmsg ); return 0; }

    ps.setNoDelete( true );
    return ps;
}


Pick::Set* uiODAnnotTreeItem::readExistingSet() const
{
    IOObjContext ctxt = uiPickSetIOObjSel::getCtxt( uiPickSetIOObjSel::AllSets,
						    true, getCategory() );
    uiIOObjSelDlg dlg( getUiParent(), ctxt );
    dlg.setCaption( uiStrings::phrLoad(typestr_) );
    dlg.setTitleText( uiStrings::phrSelect(typestr_) );
    if ( !dlg.go() || !dlg.ioObj() )
	return 0;

    uiRetVal uirv;
    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( dlg.ioObj()->key(),
							uirv );
    if ( uirv.isError() )
	{ mTIUiMsg().error( uirv ); return 0; }

    ps->ref(); // need to ref now because RefMan will go out of scope
    return ps;
}

// SubItem ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

uiODAnnotSubItem::uiODAnnotSubItem( Pick::Set& set, int displayid )
    : set_( set )
    , defscale_(mCast(float,set.dispSize()))
    , scalemnuitem_(m3Dots(uiStrings::sSize()))
    , storemnuitem_(uiStrings::sSave())
    , storeasmnuitem_(m3Dots(uiStrings::sSaveAs()))
{
    set_.ref();
    name_ = toUiString( set_.name() );
    displayid_ = displayid;

    storemnuitem_.iconfnm = "save";
    storeasmnuitem_.iconfnm = "saveas";
}


uiODAnnotSubItem::~uiODAnnotSubItem()
{
    set_.unRef();
}


bool uiODAnnotSubItem::init()
{
    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
		    visserv_->getObject(displayid_));
    if ( ld )
	ld->setSet( &set_ );

    return uiODDisplayTreeItem::init();
}


DBKey uiODAnnotSubItem::getSetID() const
{
    return Pick::SetMGR().getID( set_ );
}


void uiODAnnotSubItem::createMenu( MenuHandler* menu, bool istb )
{
    const DBKey setid = getSetID();
    if ( !menu || menu->menuID()!=displayID() || setid.isInvalid() )
	return;

    const bool islocked = visserv_->isLocked( displayid_ );
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( hasScale() )
	mAddMenuOrTBItem(istb,0,menu,&scalemnuitem_,!islocked,false);

    mAddMenuOrTBItem(istb,menu,menu,&storemnuitem_,
		     Pick::SetMGR().needsSave(setid),false);
    mAddMenuOrTBItem(istb,0,menu,&storeasmnuitem_,true,false);
}


void uiODAnnotSubItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( menu->isHandled() || menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::LocationDisplay*,ld,
		    visserv_->getObject(displayid_));
    if ( !ld )
	return;

    if ( mnuid==scalemnuitem_.id )
    {
	menu->setIsHandled( true );
	uiDialog dlg( getUiParent(), uiDialog::Setup(tr("Set Size"),
                                                     uiStrings::sSize(),
						     mNoHelpKey) );
	uiSlider* sliderfld = new uiSlider( &dlg,
                    uiSlider::Setup(uiStrings::sSize()).nrdec(1).logscale(true),
			"Size" );
	sliderfld->setMinValue( 0.1 );
	sliderfld->setMaxValue( 10 );
	sliderfld->setValue( mCast(float,set_.dispSize()/defscale_));
	sliderfld->valueChanged.notify( mCB(this,uiODAnnotSubItem,scaleChg) );
	dlg.go();
    }
    else if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	store();
    }
    else if ( mnuid==storeasmnuitem_.id )
    {
	menu->setIsHandled( true );
	storeAs();
	updateColumnText( 0 );
    }
}


void uiODAnnotSubItem::store() const
{
    IOPar ioobjpars; fillStoragePar( ioobjpars );
    SilentTaskRunnerProvider trprov;
    uiString errmsg =
		Pick::SetMGR().store( set_, getSetID(), trprov, &ioobjpars );
    if ( !errmsg.isEmpty() )
	mTIUiMsg().error( errmsg );
}


void uiODAnnotSubItem::storeAs() const
{
    const char* nm = set_.name();
    IOObjContext ctxt( uiPickSetIOObjSel::getCtxt(uiPickSetIOObjSel::AllSets,
					    false,getCategory()) );
    ctxt.setName( nm );
    uiIOObjSelDlg dlg( getUiParent(), ctxt );
    if ( !dlg.go() || !dlg.ioObj() )
	return;

    SilentTaskRunnerProvider trprov;
    uiString errmsg =
	    Pick::SetMGR().saveAs( getSetID(), dlg.ioObj()->key(), trprov );
    if ( !errmsg.isEmpty() )
	mTIUiMsg().error( errmsg );
}


void uiODAnnotSubItem::setScale( float ns )
{
    const int newscale = mNINT32( ns );
    set_.setDispSize( newscale );
}


void uiODAnnotSubItem::setColor( Color nc )
{
    set_.setDispColor( nc );
}


void uiODAnnotSubItem::scaleChg( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,slider,cb);
    if ( !slider ) return;
    const float newscale = defscale_ * slider->getValue();
    setScale( newscale );
}
