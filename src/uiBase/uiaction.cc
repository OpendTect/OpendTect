/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/

#include "uiaction.h"
#include "i_qaction.h"

#include "uiicon.h"
#include "uimain.h"
#include "uimenu.h"
#include "uipixmap.h"

#include "menuhandler.h"
#include "perthreadrepos.h"
#include "texttranslator.h"

#include "q_uiimpl.h"

#include <limits.h>

mUseQtnamespace


static ObjectSet<uiAction> uiactionlist_;

#define mInit toggled(this), \
    triggered(this), \
    msgr_(0),  \
    parentcontainer_( 0 ),  \
    cmdrecrefnr_( 0 ), \
    menu_( 0 )

uiAction::uiAction( QAction* qact )
    : mInit
    , qaction_(qact)
{
    msgr_ = new i_ActionMessenger( qact, this );
}


uiAction::uiAction( const uiString& txt )
    : mInit
{
    init( txt );
}


uiAction::uiAction( const uiString& txt, const CallBack& cb )
    : mInit
{
    init( txt );
    triggered.notify( cb );
}


uiAction::uiAction( const uiString& txt, const CallBack& cb,
		    const uiIcon& icon )
    : mInit
{
    init( txt );
    setIcon( icon );
    triggered.notify( cb );
}


uiAction::uiAction( const uiString& txt, const CallBack& cb,
		    const char* iconfile )
    : mInit
{
    init( txt );
    setIcon( iconfile );
    triggered.notify( cb );
}


uiAction::uiAction( const uiString& txt, const char* iconfile )
    : mInit
{
    init( txt );
    setIcon( iconfile );
}


uiAction::uiAction( const MenuItem& itm )
    : mInit
{
    init( itm.text );
    setToolTip( mToUiStringTodo(itm.tooltip) );
    setCheckable( itm.checkable );
    setChecked( itm.checked );
    setEnabled( itm.enabled );
    if ( !itm.iconfnm.isEmpty() )
	setIcon( itm.iconfnm );
    if ( itm.cb.willCall() )
	triggered.notify( itm.cb );
}


uiAction::~uiAction()
{
    detachAllNotifiers();

    uiactionlist_ -= this;

    if ( menu_ )
    {
	//Sanity check. Does not cost much
	if ( menu_->submenuaction_!=this )
	{
	    pErrMsg("Wrong action");
	}
	menu_->setAction( 0 );
	delete menu_;
    }

    delete msgr_;
    delete qaction_;
}


void uiAction::init( const uiString& txt )
{
    mAttachCB( TrMgr().languageChange, uiAction::translateCB );
    text_ = txt;
    qaction_ = new QAction( toQString(text_), 0 );
    msgr_ = new i_ActionMessenger( qaction_, this );
    uiactionlist_ += this;

    updateToolTip();
}


void uiAction::setShortcut( const char* sctxt )
{
    if ( sctxt && *sctxt )
	qaction_->setShortcut( QString(sctxt) );
}


void uiAction::setText( const uiString& txt )
{
    text_ = txt;
    qaction_->setText( toQString(text_) );
    updateToolTip();
}


const uiString& uiAction::text() const
{
    return text_;
}


void uiAction::setIconText( const uiString& txt )
{
    icontext_ = txt;
    qaction_->setIconText( toQString(icontext_) );
}


const uiString& uiAction::iconText() const
{
    return icontext_;
}


void uiAction::setToolTip( const uiString& txt )
{
    tooltip_ = txt;
    updateToolTip();
}


const uiString& uiAction::toolTip() const
{
    return tooltip_;
}


void uiAction::updateToolTip( CallBacker* )
{
    mEnsureExecutedInMainThread( uiAction::updateToolTip );

    if ( uiMain::isNameToolTipUsed() )
    {
	BufferString namestr =
		(text_.isEmpty() ? tooltip_ : text_ ).getFullString();
	uiMain::formatNameToolTipString( namestr );
	qaction_->setToolTip( namestr.buf() );
    }
    else
    {
	qaction_->setToolTip( toQString(tooltip_) );
    }
}


void uiAction::setSeparator( bool yn )
{ qaction_->setSeparator( yn ); }


bool uiAction::isSeparator() const
{ return qaction_->isSeparator(); }


void uiAction::updateToolTips()
{
    for ( int idx=uiactionlist_.size()-1; idx>=0; idx-- )
	uiactionlist_[idx]->updateToolTip();
}


void uiAction::setMenu( uiMenu* menu )
{
    delete menu_;

    menu_ = menu;
    if ( menu_ )
    {
	menu_->setAction( this );
	qaction_->setMenu( menu_->getQMenu() );
	if ( iconfile_.isEmpty() )
	{
	    const FixedString menuicon( menu_->getIconName() );
	    if ( !menuicon.isEmpty() )
		setIcon( menuicon.buf() );
	}
    }
}


void uiAction::setParentContainer( const uiActionContainer* parentcontainer )
{
    if ( parentcontainer_ ) //Sanity check
    {
	pErrMsg("Hmm, Perhaps already added somewhere" );
    }
    parentcontainer_ = parentcontainer;
}


int uiAction::getID() const
{
    if ( !parentcontainer_ )
	return -1;

    return parentcontainer_->getID( this );
}


void uiAction::trigger( bool checked )
{
    checked_ = checked;
    triggered.trigger();
}


void uiAction::reloadIcon()
{
    if ( iconfile_.isEmpty() || iconfile_[0] == '[' )
	return;

    const uiIcon icon( iconfile_ );
    if ( icon.isEmpty() )
	return;

    qaction_->setIcon( icon.qicon() );
}


void uiAction::translateCB( CallBacker* cb )
{
    qaction_->setText( toQString(text_) );
    qaction_->setIconText( toQString(icontext_) );
    updateToolTip();
}


void uiAction::setIcon( const char* file )
{
    iconfile_ = file;
    const uiIcon icon( iconfile_ );
    setIcon( icon );
}


void uiAction::setIcon( const uiIcon& icon )
{
    iconfile_ = icon.source();
    qaction_->setIcon(
	iconfile_.isEmpty() || icon.isEmpty() ? QIcon() : icon.qicon() );
}


void uiAction::setPixmap( const uiPixmap& pm )
{
    iconfile_ = pm.source();
    if ( pm.qpixmap() )
	qaction_->setIcon( *pm.qpixmap() );
}


#define mSetGet(setfn,getfn,updatefn,var) \
void uiAction::updatefn( CallBacker* ) \
{ \
    mEnsureExecutedInMainThread( uiAction::updatefn ); \
    qaction_->setfn( var ); \
} \
\
void uiAction::setfn( bool yn ) \
{ var = yn; updatefn(); } \
\
bool uiAction::getfn() const \
{ return qaction_->getfn(); }


mSetGet( setCheckable, isCheckable, updateCheckable, checkable_ )
mSetGet( setChecked, isChecked, updateChecked, ischecked_ )
mSetGet( setEnabled, isEnabled, updateEnabled, enabled_ )
mSetGet( setVisible, isVisible, updateVisible, visible_ )

static CallBackSet& cmdrecorders_ = *new CallBackSet;

int uiAction::beginCmdRecEvent( const char* msg )
{
    if ( cmdrecorders_.isEmpty() )
	return -1;

    cmdrecrefnr_ = cmdrecrefnr_==INT_MAX ? 1 : cmdrecrefnr_+1;

    BufferString actstr( "Begin " );
    actstr += cmdrecrefnr_; actstr += " "; actstr += msg;
    CBCapsule<const char*> caps( actstr, this );
    cmdrecorders_.doCall( &caps );
    return cmdrecrefnr_;
}


void uiAction::endCmdRecEvent( int refnr, const char* msg )
{
    if ( !cmdrecorders_.isEmpty() )
    {
	BufferString actstr( "End " );
	actstr += refnr; actstr += " "; actstr += msg;
	CBCapsule<const char*> caps( actstr, this );
	cmdrecorders_.doCall( &caps );
    }
}


void uiAction::removeCmdRecorder( const CallBack& cb )
{
    cmdrecorders_ -= cb;
}


void uiAction::addCmdRecorder( const CallBack& cb )
{
    cmdrecorders_ += cb;
}


uiActionContainer::uiActionContainer()
{
}


uiActionContainer::~uiActionContainer()
{
    deepErase( actions_ );
}


int uiActionContainer::nrActions() const
{ return actions_.size(); }


const ObjectSet<uiAction>& uiActionContainer::actions() const
{ return actions_; }


bool uiActionContainer::isEmpty() const
{ return actions_.isEmpty(); }


uiAction* uiActionContainer::findAction( int mnuid )
{
    for ( int idx=0; idx<nrActions(); idx++ )
    {
	if ( ids_[idx] == mnuid )
	    return actions_[idx];

	if ( actions_[idx]->getMenu() )
	{
	    uiAction* mnuitm = actions_[idx]->getMenu()->findAction( mnuid );
	    if ( mnuitm ) return mnuitm;
	}
    }

    return 0;
}


uiAction* uiActionContainer::findAction( const uiMenu* menu )
{
    if ( !menu )
	return 0;

    for ( int idx=0; idx<nrActions(); idx++ )
    {
	if ( actions_[idx]->getMenu() == menu )
	    return actions_[idx];

	if ( actions_[idx]->getMenu() )
	{
	    uiAction* mnuitm = actions_[idx]->getMenu()->findAction( menu );
	    if ( mnuitm ) return mnuitm;
	}
    }

    return 0;
}


uiAction* uiActionContainer::findAction( const uiActionSeparString& str )
{
    uiActionContainer* curcontainer = this;
    for ( int idx=0; idx<str.size(); idx++ )
    {
	if ( !curcontainer ) return 0;

	uiAction* itm = curcontainer->findAction( str[idx] );
	if ( !itm ) return 0;
	if ( idx == str.size()-1 )
	    return itm;

	curcontainer = itm->getMenu();
    }

    return 0;
}


uiAction* uiActionContainer::findAction( const char* itmtxt )
{
    for ( int idx=0; idx<actions_.size(); idx++ )
    {
	uiAction* itm = actions_[idx];
	if ( itm->text().getString().startsWith(itmtxt,CaseInsensitive) )
	    return itm;
    }

    return 0;
}


uiAction* uiActionContainer::findAction( const uiString& itmtxt )
{
    return findAction( itmtxt.getString() );
}


int uiActionContainer::getID( const uiAction* action ) const
{
    int idx = actions_.indexOf( action );
    if ( actions_.validIdx(idx) )
	return ids_[idx];

    for ( idx=0; idx<actions_.size(); idx++ )
    {
	const uiAction* curaction = actions_[idx];
	if ( curaction->getMenu() )
	{
	    const int id = curaction->getMenu()->getID( action );
	    if ( id>=0 )
		return id;
	}
    }

    return -1;
}


int uiActionContainer::getID( const QAction* qaction ) const
{
    for ( int idx=0; idx<actions_.size(); idx++ )
    {
	if ( actions_[idx]->qaction()==qaction )
	    return ids_[idx];

	if ( !actions_[idx]->getMenu() )
	    continue;

	int res = actions_[idx]->getMenu()->getID(qaction);
	if ( res>=0 )
	    return res;
    }

    return -1;
}


int uiActionContainer::getFreeID() const
{
    int curid = 100000000;
    /* Huge number to not interfere with internal numbers that typically
       starts at 1, 2, 3, .. */

    uiActionContainer* myptr = const_cast<uiActionContainer*>( this );

    while( myptr->findAction(curid) )
	curid++;

    return curid;
}


int uiActionContainer::insertAction( const MenuItem& itm )
{
    return insertAction( new uiAction(itm), itm.id, 0 );
}


int uiActionContainer::insertAction( uiAction* action, int id,
				     const uiAction* before )
{
    if ( actions_.isPresent(action) )
    {
	pErrMsg("Already inserted");
    }

    if ( action->getMenu() )
    { //Sanity check
	uiAction* prevaction = findAction( action->getMenu() );

	if ( prevaction )
	{
	    pErrMsg("Menu is already present.");
	    return -1;
	}
    }

    const int idx = before ? actions_.indexOf( before ) : -1;
    QAction* beforeaction = actions_.validIdx(idx)
	? actions_[idx]->qaction()
	: 0;

    if ( action->getMenu() )
	doInsertMenu( action->getMenu()->getQMenu(), beforeaction );
    else
    {
	doInsertAction( action->qaction(), beforeaction );
	if ( id<0 )
	{
	    id = getFreeID();
	}
	else if ( findAction( id ) )
	{
	    pErrMsg("Duplicate menu id found.");
	}
    }

    action->setParentContainer( this );

    if ( idx>=0 )
    {
	actions_.insertAt( action, idx );
	ids_.insert( idx, id );
    }
    else
    {
	actions_ += action;
	ids_ += id;
    }

    return id;
}


int uiActionContainer::insertItem( uiMenu* mnu )
{
    addMenu( mnu, 0 );
    uiAction* menuaction = findAction( mnu );
    return menuaction->getID();
}


uiMenu* uiActionContainer::addMenu( uiMenu* mnu, const uiMenu* before )
{
    uiAction* submenuitem = new uiAction( mnu->text() );
    submenuitem->setMenu( mnu );

    uiAction* beforeaction = 0;
    if ( before )
    {
	for ( int idx=0; idx<actions_.size(); idx++ )
	{
	    if ( actions_[idx]->getMenu() == before )
	    {
		beforeaction = actions_[idx];
		break;
	    }
	}
    }

    insertAction( submenuitem, -1, beforeaction );
    return mnu;
}


uiAction* uiActionContainer::insertSeparator()
{
    uiAction* action = new uiAction( uiString() );
    action->setSeparator( true );
    insertAction( action );
    return action;
}


void uiActionContainer::removeAllActions()
{
    deepErase( actions_ );
    ids_.erase();

    doClear();
}


void uiActionContainer::removeAction( uiAction* action )
{
    const int idx = actions_.indexOf( action );
    if ( actions_.validIdx(idx) )
    {
	doRemoveAction( actions_[idx]->qaction() );

	delete actions_.removeSingle( idx );
	ids_.removeSingle( idx );
    }
}


void uiActionContainer::removeAction( int id )
{
    const int idx=ids_.indexOf(id);
    if ( actions_.validIdx(idx) )
	removeItem( actions_[idx] );
}


void uiActionContainer::reloadIcons()
{
    mObjectSetApplyToAll( actions_, actions_[idx]->reloadIcon() );
}
