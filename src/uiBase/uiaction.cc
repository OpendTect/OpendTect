/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiaction.h"
#include "i_qaction.h"

#include <limits.h> 
#include "texttranslator.h"
#include "uimenu.h"
#include "staticstring.h"
#include "menuhandler.h"
#include "pixmap.h"
#include "uimain.h"

mUseQtnamespace


static ObjectSet<uiAction> uiactionlist_;

#define mInit toggled(this), \
    triggered(this), \
    msgr_(0),  \
    parentcontainer_( 0 ),  \
    cmdrecrefnr_( 0 ), \
    translateid_( -1 ), \
    menu_( 0 ), \
    qnormaltooltipstr_( new QString ), \
    qtranslatedtooltipstr_( new QString )

uiAction::uiAction( QAction* qact )
    : mInit
    , qaction_(qact)
{
    msgr_ = new i_ActionMessenger( qact, this );
}


uiAction::uiAction( const char* txt )
    : mInit
{
    init( txt );
}


uiAction::uiAction( const char* txt, const CallBack& cb )
    : mInit
{
    init( txt );
    triggered.notify( cb );
}


uiAction::uiAction( const char* txt, const CallBack& cb, const ioPixmap& icon )
    : mInit
{
    init( txt );
    setIcon( icon );
    triggered.notify( cb );
}


uiAction::uiAction( const char* txt, const CallBack& cb, const char* iconfile )
    : mInit
{
    FixedString pixmapfile( iconfile );
    init( txt );
    if ( pixmapfile )
	setIcon( ioPixmap(pixmapfile) );
    
    triggered.notify( cb );
}


uiAction::uiAction( const char* txt, const char* iconfile )
    : mInit
{
    FixedString pixmapfile( iconfile );
    init( txt );
    if ( pixmapfile )
	setIcon( ioPixmap(pixmapfile) );
}



uiAction::uiAction( const MenuItem& itm )
    : mInit
{
    init( itm.text );
    setToolTip( itm.tooltip );
    setCheckable( itm.checkable );
    setChecked( itm.checked );
    setEnabled( itm.enabled );
    if ( !itm.iconfnm.isEmpty() )
	setIcon( ioPixmap(itm.iconfnm) );
}

uiAction::~uiAction()
{
    delete msgr_;
    delete qnormaltooltipstr_;
    delete qtranslatedtooltipstr_;
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
}


void uiAction::init( const char* txt )
{
    qaction_ = new QAction( QString(txt), 0 );
    msgr_ = new i_ActionMessenger( qaction_, this );
    uiactionlist_ += this;
    updateToolTip();
}


void uiAction::setShortcut( const char* sctxt )
{
    if ( sctxt && *sctxt )
	qaction_->setShortcut( QString(sctxt) );
}


void uiAction::setText( const char* txt )
{
    qaction_->setText( QString(txt) );
    updateToolTip();
}


void uiAction::setText( const wchar_t* txt )
{
    qaction_->setText( QString::fromWCharArray(txt) );
    updateToolTip();
}


const char* uiAction::text() const
{
    mDeclStaticString( ret );
    static StaticStringManager stm;
    ret = qaction_->text().toLatin1().data();
    return ret.buf();
}


void uiAction::setIconText( const char* txt )
{ qaction_->setIconText( txt ); }


const char* uiAction::iconText() const
{
    mDeclStaticString( ret );
    ret = qaction_->iconText().toLatin1().data();
    return ret.buf();
}


void uiAction::setToolTip( const char* txt )
{ setToolTip( QString(txt) ); }


void uiAction::setToolTip( const wchar_t* txt )
{ setToolTip( QString::fromWCharArray(txt) ); }


void uiAction::setToolTip( const QString& txt )
{
    if ( txt == *qnormaltooltipstr_ )
	return;

    const bool wastranslated = translateid_>=0 ||
			       qtranslatedtooltipstr_->size();

    *qnormaltooltipstr_ = txt;
    *qtranslatedtooltipstr_ = "";
    updateToolTip();


    if ( TrMgr().tr() && TrMgr().tr()->enabled() && wastranslated )
	doTranslate();
}


const char* uiAction::toolTip() const
{
    mDeclStaticString( ret );
    ret = qnormaltooltipstr_->toLatin1().data();
    return ret.buf();
}


void uiAction::updateToolTip()
{
    if ( uiMain::isNameToolTipUsed() )
    {
	BufferString namestr = *text() ? text() : toolTip();
	uiMain::formatNameToolTipString( namestr );
	qaction_->setToolTip( namestr.buf() );
    }
    else if ( qtranslatedtooltipstr_->size() )
	qaction_->setToolTip( *qtranslatedtooltipstr_ );
    else
	qaction_->setToolTip( *qnormaltooltipstr_ );
}


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


void uiAction::trigger(bool checked)
{
    checked_ = checked;
    triggered.trigger();
}


void uiAction::doTranslate()
{
    if ( !TrMgr().tr() ) return;

    BufferString txt( *toolTip() ? toolTip() : text() );
    removeCharacter( txt.buf(), '&' );
    removeStartAndEndSpaces( txt.buf() );
    if ( txt.isEmpty() || isNumberString(txt.buf()) )
	return;

    mAttachCB(TrMgr().tr()->ready, uiAction::translationReadyCB );
    translateid_ = TrMgr().tr()->translate( txt );
}


void uiAction::translate() 
{
    doTranslate();

    if ( menu_ ) menu_->translate();
}


void uiAction::reloadIcon()
{
    if ( iconfile_.isEmpty() )
	return;

    if ( iconfile_[0] == '[' )
	return;
    
    FileMultiString fms( iconfile_ );
    const int len = fms.size();
    const BufferString fnm( fms[0] );
    const ioPixmap pm( fnm.buf(), len > 1 ? fms[1] : 0 );
    if ( pm.isEmpty() )
	return;
    
    qaction_->setIcon( *pm.qpixmap() );
}


void uiAction::translationReadyCB( CallBacker* cb )
{
    mCBCapsuleUnpack(int,translationid,cb);
    if ( translationid != translateid_ )
	return;

    const wchar_t* translation = TrMgr().tr()->get();
    QString txt = QString::fromWCharArray( translation );
    QString tt( *toolTip() ? toolTip() : text() );
    tt += "\n\n"; tt += txt;

    *qtranslatedtooltipstr_ = tt;
    updateToolTip();

    translateid_ = -1;
    mDetachCB(TrMgr().tr()->ready, uiAction::translationReadyCB );
}



void uiAction::setIcon( const ioPixmap& pm )
{
    iconfile_ = pm.source();
    qaction_->setIcon( *pm.qpixmap() );
}


void uiAction::setIcon( const char* file )
{
    setIcon( ioPixmap(file) );
}



#define mSetGet(setfn,getfn) \
void uiAction::setfn( bool yn ) \
{ qaction_->setfn( yn ); } \
\
bool uiAction::getfn() const \
{ return qaction_->getfn(); }


mSetGet( setCheckable, isCheckable )
mSetGet( setChecked, isChecked )
mSetGet( setEnabled, isEnabled )
mSetGet( setVisible, isVisible )

static CallBackSet cmdrecorders_;

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
	if ( !strcmp(itm->text(),itmtxt) )
	    return itm;
    }
    
    return 0;
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


int uiActionContainer::insertItem( uiMenu* pm )
{
    addMenu( pm, 0 );
    uiAction* menuaction = findAction( pm );
    return menuaction->getID();
}


uiMenu* uiActionContainer::addMenu( uiMenu* pm, const uiMenu* before )
{
    uiAction* submenuitem = new uiAction( pm->name() );
    submenuitem->setMenu( pm );

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
    
    insertItem( submenuitem, -1, beforeaction );
    return pm;
}


void uiActionContainer::insertSeparator()
{
    doInsertSeparator( 0 );
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


void uiActionContainer::translate()
{
    mObjectSetApplyToAll( actions_, actions_[idx]->translate() );
}


void uiActionContainer::reloadIcons()
{
    mObjectSetApplyToAll( actions_, actions_[idx]->reloadIcon() );
}


