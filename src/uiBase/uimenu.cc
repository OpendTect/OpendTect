/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uimenu.h"
#include "i_qmenu.h"

#include "uiaction.h"
#include "uibody.h"
#include "uimain.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "pixmap.h"
#include "texttranslator.h"
#include <climits>

#include <QCursor>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>

class uiMenuItemContainerBody
{
public:

    virtual			~uiMenuItemContainerBody()
    				{ deepErase( itms_ ); }

    int				nrItems() const 	{ return itms_.size(); }

    virtual int			addItem(uiMenuItem*,int id) =0;
    virtual int			addItem(uiPopupMenu*,int id) =0;
    virtual int			insertMenu(uiPopupMenu*,uiPopupMenu* before) =0;

    virtual QMenuBar*		bar()			{ return 0; }
    virtual QMenu*		popup()			{ return 0; }

    void			setIcon( const QPixmap& pm )
				{
    			    	    if ( bar() )
					bar()->setWindowIcon( pm );
    			    	    if ( popup() )
					popup()->setWindowIcon( pm );
				}

    void			setSensitive( bool yn )
				{
				    if ( bar() ) bar()->setEnabled( yn );
				}

    ObjectSet<uiMenuItem>	itms_;
    ObjectSet<uiAction>		uiactions_;
    ObjectSet<QAction>		qactions_;

protected:
				uiMenuItemContainerBody()	{}
};


template <class T>
class uiMenuItemContainerBodyImpl : public uiMenuItemContainerBody
			 , public uiBodyImpl<uiMenuItemContainer,T>
{
public:
uiMenuItemContainerBodyImpl( uiMenuItemContainer& hndle, uiParent* parnt,
			     T& qThing )
    : uiBodyImpl<uiMenuItemContainer,T>( hndle, parnt, qThing )
    , qmenu_( &qThing )
    , msgr_(0)
{
    QMenuBar* qmenubar = bar();
    if ( qmenubar )
	msgr_ = new i_MenuMessenger( qmenubar );
}

~uiMenuItemContainerBodyImpl()
{
    delete msgr_;
}


int getIndexFromID( int id )
{
    if ( id < 0 ) return -1;
    for ( int idx=0; idx<itms_.size(); idx++ )
	if ( itms_[idx]->id() == id )
	    return idx;
    return -1;
}


int addItem( uiMenuItem* it, int id )
{
    QString nm( it->name() );
    i_MenuMessenger* msgr = it->messenger();
    QAction* action = qmenu_->addAction( nm, msgr, SLOT(activated()) );
    init( it, action, id, -1 );
    return id;
}


int addItem( uiPopupMenu* pmnu, int id )
{
    uiPopupItem* it = &pmnu->item();
    QMenu* pu = pmnu->body_->popup();
    pu->setTitle( it->name().buf() );
    QAction* action = qmenu_->addMenu( pu );
    init( it, action, id, -1 );
    return id;
}


int insertMenu( uiPopupMenu* pmnu, uiPopupMenu* before )
{
    const int beforeidx = before ? getIndexFromID( before->item().id() ) : -1;
    QAction* actionbefore = before ? before->item().qaction_ : 0;
    uiPopupItem* it = &pmnu->item();
    QMenu* pu = pmnu->body_->popup();
    pu->setTitle( it->name().buf() );
    QAction* action = qmenu_->insertMenu( actionbefore, pu );
    init( it, action, it->id(), beforeidx );
    return it->id();
}


void init( uiMenuItem* it, QAction* action, int id, int idx )
{
    if ( !action ) return;

    action->setIconVisibleInMenu( true );
    action->setToolTip( action->text() );
    if ( it->pixmap_ )
    {
	action->setIcon( *it->pixmap_->qpixmap() );
	delete it->pixmap_; it->pixmap_ = 0;
    }

    it->setId( id );
    it->setMenu( this );
    it->setAction( action );
    if ( it->isChecked() )
	action->setChecked( it->isChecked() );
    action->setEnabled( it->isEnabled() );
    if ( idx>=0 )
    {
	itms_.insertAt( it, idx );
	qactions_.insertAt( action, idx );
	uiactions_.insertAt( new uiAction(action), idx );
    }
    else
    {
	itms_ += it;
	qactions_ += action;
	uiactions_ += new uiAction( action );
    }
}


void clear()
{
    qmenu_->clear();
    deepErase( itms_ );
    qactions_.erase();
    uiactions_.erase();
}

QMenuBar* bar()
{ mDynamicCastGet(QMenuBar*,qbar,qmenu_) return qbar; }

QMenu* popup()
{ mDynamicCastGet(QMenu*,qpopup,qmenu_) return qpopup; }

virtual const QWidget* managewidg_() const
{ return qmenu_; }

private:

    T*			qmenu_;
    i_MenuMessenger*	msgr_;
};


//-----------------------------------------------------------------------

#define mInitMembers \
    : NamedObject(nm) \
    , activated(this) \
    , messenger_( *new i_MenuMessenger(this) )  \
    , translateid_(-1) \
    , id_(-1) \
    , menu_(0) \
    , qaction_(0) \
    , enabled_(true) \
    , checked_(false) \
    , pixmap_(0) \
    , checkable_(false) \
    , cmdrecrefnr_(0)

uiMenuItem::uiMenuItem( const char* nm, const char* pmnm )
    mInitMembers
{
    if ( pmnm && *pmnm )
	pixmap_ = new ioPixmap(pmnm);
}


uiMenuItem::uiMenuItem( const char* nm, const CallBack& cb, const char* pmnm )
    mInitMembers
{ 
    activated.notify( cb ); 
    if ( pmnm && *pmnm )
	pixmap_ = new ioPixmap(pmnm);
}


uiMenuItem::~uiMenuItem()
{
    delete pixmap_;
    delete &messenger_;
}

bool uiMenuItem::isEnabled() const
{ return qaction_ ? qaction_->isEnabled() : enabled_; }

void uiMenuItem::setEnabled( bool yn )
{
    enabled_ = yn;
    if ( qaction_ && isMainThreadCurrent() ) qaction_->setEnabled( yn );
}


bool uiMenuItem::isCheckable() const
{ return qaction_ ? qaction_->isCheckable() : checkable_; }

void uiMenuItem::setCheckable( bool yn )
{
    checkable_ = yn;
    if ( qaction_ ) qaction_->setCheckable( yn );
}


bool uiMenuItem::isChecked() const
{
    if ( !checkable_ ) return false;
    return qaction_ ? qaction_->isChecked() : checked_;
}


void uiMenuItem::setChecked( bool yn )
{
    if ( !checkable_ ) return;
    if ( qaction_ ) qaction_->setChecked( yn );

    checked_ = yn;
}


const char* uiMenuItem::text() const
{
    static BufferString txt;
    txt = qaction_ ? mQStringToConstChar(qaction_->text()) : "";
    return txt;
}


void uiMenuItem::setText( const char* txt )
{ if ( qaction_ ) qaction_->setText( txt ); }

void uiMenuItem::setText( const wchar_t* txt )
{ if ( qaction_ ) qaction_->setText( QString::fromWCharArray(txt) ); }

void uiMenuItem::setPixmap( const char* pmnm )
{
    if ( qaction_ )
	setPixmap( ioPixmap(pmnm) );
    else
	pixmap_ = new ioPixmap( pmnm );
}

void uiMenuItem::setPixmap( const ioPixmap& pm )
{
    if ( !qaction_ )
	pixmap_ = new ioPixmap( pm );
    else if ( pm.qpixmap() )
	qaction_->setIcon( *pm.qpixmap() );
}


void uiMenuItem::setShortcut( const char* sctxt )
{
    if ( qaction_ && sctxt && *sctxt )
	qaction_->setShortcut( QString(sctxt) );
}


CallBack* uiMenuItem::cmdrecorder_ = 0;

static CallBackSet cmdrecorders_;

int uiMenuItem::beginCmdRecEvent( const char* msg )
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


void uiMenuItem::endCmdRecEvent( int refnr, const char* msg )
{
    if ( !cmdrecorders_.isEmpty() )
    {
	BufferString actstr( "End " );
	actstr += refnr; actstr += " "; actstr += msg;
	CBCapsule<const char*> caps( actstr, this );
	cmdrecorders_.doCall( &caps );
    }
}


void uiMenuItem::unsetCmdRecorder()
{
    if ( cmdrecorder_ )
	delete cmdrecorder_;
    cmdrecorder_ = 0;
}


void uiMenuItem::setCmdRecorder( const CallBack& cb )
{
    unsetCmdRecorder();
    cmdrecorder_ = new CallBack( cb );
}


void uiMenuItem::removeCmdRecorder( const CallBack& cb )
{
    cmdrecorders_ -= cb;
}


void uiMenuItem::addCmdRecorder( const CallBack& cb )
{
    cmdrecorders_ += cb;
}


void uiMenuItem::translate()
{
    if ( !TrMgr().tr() ) return;

    TrMgr().tr()->ready.notify( mCB(this,uiMenuItem,trlReady) );
    BufferString txt = text();
    removeCharacter( txt.buf(), '&' );
    translateid_ = TrMgr().tr()->translate( txt );
}


void uiMenuItem::trlReady( CallBacker* cb )
{
    if ( !qaction_ )
	return;

    mCBCapsuleUnpack(int,mnuid,cb);
    if ( mnuid != translateid_ )
	return;

    const wchar_t* translation = TrMgr().tr()->get();
    QString txt = QString::fromWCharArray( translation );
    qaction_->setToolTip( txt );
    TrMgr().tr()->ready.remove( mCB(this,uiMenuItem,trlReady) );
}


//-----------------------------------------------------------------------


uiMenuItemContainer::uiMenuItemContainer( const char* nm, uiBody* b,
					  uiMenuItemContainerBody* db )
    : uiBaseObject( nm, b )
    , body_( db )				{}


uiMenuItemContainer::~uiMenuItemContainer()	{ delete body_; }


int uiMenuItemContainer::nrItems() const
    { return body_->nrItems(); }


const ObjectSet<uiMenuItem>& uiMenuItemContainer::items() const
    { return body_->itms_; }


uiMenuItem* uiMenuItemContainer::find( int mnuid )
{
    for ( int idx=0; idx<body_->nrItems(); idx++ )
    {
	uiMenuItem* itm = body_->itms_[idx];
	if ( itm->id() == mnuid )
	    return itm;

	mDynamicCastGet(uiPopupItem*,popupitm,itm)
	if ( popupitm )
	{
	    uiMenuItem* mnuitm = popupitm->menu().find( mnuid );
	    if ( mnuitm ) return mnuitm;
	}
    }

    return 0;
}


uiMenuItem* uiMenuItemContainer::find( const MenuItemSeparString& str )
{
    uiMenuItemContainer* parent = this;
    for ( int idx=0; idx<str.size(); idx++ )
    {
	if ( !parent ) return 0;

	uiMenuItem* itm = parent->find( str[idx] );
	if ( !itm ) return 0;
	if ( idx == str.size()-1 )
	    return itm;

	mDynamicCastGet(uiPopupItem*,popupitm,itm)
	parent = popupitm ? &popupitm->menu() : 0;
    }

    return 0;
}


uiMenuItem* uiMenuItemContainer::find( const char* itmtxt )
{
    for ( int idx=0; idx<body_->nrItems(); idx++ )
    {
	const uiMenuItem* itm = body_->itms_[idx];
	if ( !strcmp(itm->name(),itmtxt) )
	    return const_cast<uiMenuItem*>(itm);
    }

    return 0;
}


int uiMenuItemContainer::insertItem( uiMenuItem* it, int id )
    { return body_->addItem( it, id ); }

int uiMenuItemContainer::insertItem( uiPopupMenu* it, int id )
    { return body_->addItem( it, id ); }

int uiMenuItemContainer::insertMenu( uiPopupMenu* pm, uiPopupMenu* before )
{ return body_->insertMenu( pm, before ); }

void uiMenuItemContainer::insertSeparator() 
{
    if ( body_->bar() )
	body_->bar()->addSeparator();
    else if ( body_->popup() )
	body_->popup()->addSeparator();
}


void uiMenuItemContainer::clear()
{
    if ( body_->bar() )		body_->bar()->clear();
    if ( body_->popup() )	body_->popup()->clear();

    deepErase( body_->itms_ );
    body_->qactions_.erase();
}


void uiMenuItemContainer::removeItem( uiMenuItem* itm )
{
    for ( int idx=0; idx<body_->itms_.size(); idx++ )
    {
	if ( body_->itms_[idx] != itm )
	    continue;

	if ( body_->popup() )
	    body_->popup()->removeAction( body_->qactions_[idx] );

	body_->itms_.remove( idx );
	body_->qactions_.remove( idx );
	return;
    }
}


void uiMenuItemContainer::removeItem( int id, bool withdelete )
{
    for ( int idx=0; idx<body_->itms_.size(); idx++ )
    {
	if ( body_->itms_[idx]->id() != id )
	    continue;

	if ( body_->popup() )
	    body_->popup()->removeAction( body_->qactions_[idx] );

	uiMenuItem* itm = body_->itms_.remove( idx );
	if ( withdelete ) delete itm;
	body_->qactions_.remove( idx );
	return;
    }
}


void uiMenuItemContainer::translate()
{
    for ( int idx=0; idx<body_->itms_.size(); idx++ )
    {
	uiMenuItem* itm = body_->itms_[idx];
	itm->translate();

	mDynamicCastGet(uiPopupItem*,popupitm,itm)
	if ( popupitm )
	    popupitm->menu().translate();
    }
}

// ------------------------------------------------------------------------


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm )
    : uiMenuItemContainer( nm, 0, 0 )
{
    QMenuBar* qmenubar = new QMenuBar( parnt->body()->qwidget() );
    qmenubar->setObjectName( nm );
    uiMenuItemContainerBodyImpl<QMenuBar>* bd =
	new uiMenuItemContainerBodyImpl<QMenuBar>( *this, parnt, *qmenubar );
    body_ = bd;
    setBody( bd );
}


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm, QMenuBar& qThing )
    : uiMenuItemContainer( nm, 0, 0 )
{ 
    uiMenuItemContainerBodyImpl<QMenuBar>* bd =
	    new uiMenuItemContainerBodyImpl<QMenuBar>( *this, parnt, qThing );
    body_ = bd;
    setBody( bd );
}


void uiMenuBar::reDraw( bool deep )
{ if ( body_->bar() ) body_->bar()->update(); }

void uiMenuBar::setIcon( const QPixmap& pm )
{ body_->setIcon( pm ); }

void uiMenuBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }

bool uiMenuBar::isSensitive() const
{ return body_->bar() && body_->bar()->isEnabled(); }

// -----------------------------------------------------------------------

CallBack* uiPopupMenu::interceptor_ = 0;

static CallBackSet interceptors_;

uiPopupMenu::uiPopupMenu( uiParent* parnt, const char* nm,
			  const char* pmnm )
    : uiMenuItemContainer( nm, 0, 0 )
    , item_( *new uiPopupItem( *this, nm, pmnm ) )
{
    QMenu* qmenu = new QMenu( parnt->body()->qwidget() );
    uiMenuItemContainerBodyImpl<QMenu>* bd =
	new uiMenuItemContainerBodyImpl<QMenu>( *this, parnt, *qmenu );
    body_ = bd;
    setBody( bd );

    item_.setMenu( body_ );
}


uiPopupMenu::~uiPopupMenu() 
{ delete &item_; }

bool uiPopupMenu::isCheckable() const
{ return item_.isCheckable(); }

void uiPopupMenu::setCheckable( bool yn ) 
{ item_.setCheckable( yn ); }

bool uiPopupMenu::isChecked() const
{ return item_.isChecked(); }

void uiPopupMenu::setChecked( bool yn )
{ item_.setChecked( yn ); }

bool uiPopupMenu::isEnabled() const
{ return item_.isEnabled(); }

void uiPopupMenu::setEnabled( bool yn )
{ item_.setEnabled( yn ); }


int uiPopupMenu::findIdForAction( QAction* qaction ) const
{
    const int actidx = body_->qactions_.indexOf( qaction );
    if ( body_->itms_.validIdx(actidx) )
	return body_->itms_[actidx]->id();

    int id = -1;
    for ( int idx=0; idx<body_->itms_.size(); idx++ )
    {
	mDynamicCastGet(uiPopupItem*,itm,body_->itms_[idx])
	if ( itm ) id = itm->menu().findIdForAction( qaction );
	if ( id >= 0 ) break;
    }
    return id;
}


int uiPopupMenu::exec()
{
    QMenu* mnu = body_->popup();
    if ( !mnu ) return -1;

    if ( !interceptors_.isEmpty() )
    {
	dointercept_ = false;
	interceptitem_ = 0;
	interceptors_.doCall( this );

	if ( dointercept_ )
	{
	    if ( !interceptitem_ )
		return -1;

	    interceptitem_->activated.trigger();
	    return interceptitem_->id();
	}
    }

    QAction* qaction = body_->popup()->exec( QCursor::pos() );
    return findIdForAction( qaction );
}

    
void uiPopupMenu::doIntercept( bool yn, uiMenuItem* interceptitm )
{
    if ( !dointercept_ && yn )
    {
	dointercept_ = yn;
	interceptitem_ = interceptitm;
    }
    else if ( dointercept_ && yn )
    {
	pErrMsg( "Can handle multiple passive, \
		  but only one active popup menu interceptor" );
    }
}


void uiPopupMenu::unsetInterceptor()
{ 
    if ( interceptor_ )
	delete interceptor_;
    interceptor_ = 0;
}


void uiPopupMenu::setInterceptor( const CallBack& cb )
{ 
    unsetInterceptor();
    interceptor_ = new CallBack( cb );
}


void uiPopupMenu::removeInterceptor( const CallBack& cb )
{
    interceptors_ -= cb;
}


void uiPopupMenu::addInterceptor( const CallBack& cb )
{
    interceptors_ += cb;
}
