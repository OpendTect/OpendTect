/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimenu.h"

#include "uiaction.h"
#include "uibody.h"
#include "uiicon.h"
#include "uimain.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "uistring.h"

#include "keystrs.h"
#include "menuhandler.h"
#include "texttranslator.h"

#include "q_uiimpl.h"

#include <climits>

#include <QCursor>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>

mUseQtnamespace

// ------------------------------------------------------------------------


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm )
    : uiBaseObject( nm )
    , qmenubar_( new mQtclass(QMenuBar)(parnt->body()->qwidget()) )
{
    qmenubar_->setObjectName( nm );
}


uiMenuBar::uiMenuBar( uiParent* parnt, const char* nm,
		      mQtclass(QMenuBar)* qThing )
    : uiBaseObject( nm )
    , qmenubar_( qThing )
{
}


uiMenuBar::~uiMenuBar()
{
    removeAllActions();
}


void uiMenuBar::setSensitive( bool yn )
{
    qmenubar_->setEnabled( yn );
}


bool uiMenuBar::isSensitive() const
{ return qmenubar_->isEnabled(); }


void uiMenuBar::doInsertMenu(mQtclass(QMenu)* menu,
			   mQtclass(QAction)* before)
{
    qmenubar_->insertMenu( before, menu );
}


void uiMenuBar::doInsertAction(mQtclass(QAction)* action,
			     mQtclass(QAction)* before)
{
    qmenubar_->insertAction( before, action );
}


void uiMenuBar::doInsertSeparator( QAction* )
{
    qmenubar_->addSeparator();
}


void uiMenuBar::doClear()
{
    qmenubar_->clear();
}


void uiMenuBar::doRemoveAction( mQtclass(QAction)* action )
{
    qmenubar_->removeAction( action );
}


QWidget* uiMenuBar::getWidget()
{ return qmenubar_; }

// -----------------------------------------------------------------------

static CallBackSet& interceptors_ = *new CallBackSet;

uiMenu::uiMenu( uiParent* p, const uiString& txt, const char* pmnm )
    : uiBaseObject( txt.getFullString() )
    , submenuaction_( 0 )
    , qmenu_( new mQtclass(QMenu)(toQString(txt),p ? p->getWidget() : 0))
    , text_(txt)
{
    setIcon( pmnm );
    useStyleSheet();
}


uiMenu::uiMenu( const uiString& txt, const char* pmnm )
    : uiBaseObject( txt.getFullString() )
    , submenuaction_( 0 )
    , qmenu_(new mQtclass(QMenu)(toQString(txt)))
    , text_(txt)
{
    setIcon( pmnm );
    useStyleSheet();
}


uiMenu::uiMenu( const MenuItem& itm )
    : uiBaseObject(itm.text.getFullString())
    , submenuaction_( 0 )
    , qmenu_(new mQtclass(QMenu)(toQString(itm.text)))
    , text_(itm.text)
{
    setIcon( itm.iconfnm );
    addItems( itm.getItems() );
    useStyleSheet();
}


uiMenu::~uiMenu()
{
    removeAllActions();
    delete qmenu_;
}


void uiMenu::useStyleSheet()
{
    if ( qmenu_ )
	qmenu_->setStyleSheet( "QMenu { menu-scrollable: 1; }" );
}


void uiMenu::addItems( const ObjectSet<MenuItem>& subitms )
{
    ObjectSet<const MenuItem> validsubitms;
    for ( int idx=0; idx<subitms.size(); idx++ )
    {
	if ( subitms[idx]->id >= 0 )
	    validsubitms += subitms[idx];
    }

    if ( validsubitms.isEmpty() )
	return;

    BoolTypeSet handled( validsubitms.size(), false );

    while ( true )
    {
	int lowest = mUdf(int);
	int lowestitem = -1;
	for ( int idx=0; idx<validsubitms.size(); idx++ )
	{
	    if ( lowestitem==-1 || lowest<validsubitms[idx]->placement )
	    {
		if ( handled[idx] ) continue;

		lowest = validsubitms[idx]->placement;
		lowestitem = idx;
	    }
	}

	if ( lowestitem==-1 )
	    break;

	const MenuItem& subitm = *validsubitms[lowestitem];
	if ( subitm.nrItems() )
	{
	    uiMenu* submenu = new uiMenu( subitm );
	    if ( submenu )
	    {
		addMenu( submenu );
		submenu->setEnabled( subitm.enabled );
		submenu->setCheckable( subitm.checkable );
		submenu->setChecked( subitm.checked );
	    }
	}
	else
	{
	    uiAction* mnuitem = new uiAction(subitm.text);
	    insertAction( mnuitem, subitm.id );
	    mnuitem->setEnabled( subitm.enabled );
	    mnuitem->setCheckable( subitm.checkable );
	    mnuitem->setChecked( subitm.checked );
	    if ( !subitm.iconfnm.isEmpty() )
		mnuitem->setIcon( subitm.iconfnm );
	}

	handled[lowestitem] = true;
    }
}


void uiMenu::setAction( uiAction* action )
{
    if ( (submenuaction_ && action) || (!submenuaction_ && !action ) )
    { //Sanity check
	pErrMsg("Action set");
    }
    submenuaction_ = action;
}


bool uiMenu::isCheckable() const
{ return submenuaction_->isCheckable(); }

void uiMenu::setCheckable( bool yn )
{ submenuaction_->setCheckable( yn ); }

bool uiMenu::isChecked() const
{ return submenuaction_->isChecked(); }

void uiMenu::setChecked( bool yn )
{ submenuaction_->setChecked( yn ); }

bool uiMenu::isEnabled() const
{ return submenuaction_->isEnabled(); }

void uiMenu::setEnabled( bool yn )
{ submenuaction_->setEnabled( yn ); }


void uiMenu::setText( const uiString& txt )
{
    uiBaseObject::setName( txt.getFullString() );
    text_ = txt;
    if ( submenuaction_ )
	submenuaction_->setText( txt );
}


const uiString& uiMenu::text() const
{ return text_; }


void uiMenu::setIcon( const uiIcon& icon )
{ setIcon( icon.source() ); }


void uiMenu::setIcon( const char* iconnm )
{
    iconnm_ = iconnm;
    if ( iconnm_.isEmpty() )
	qmenu_->setIcon( QIcon() );
    else
    {
	uiIcon icon( iconnm );
	qmenu_->setIcon( icon.qicon() );
    }
}


const char* uiMenu::getIconName() const
{ return iconnm_; }


int uiMenu::exec()
{
    if ( !qmenu_ ) return -1;

    if ( !interceptors_.isEmpty() )
    {
	dointercept_ = false;
	interceptaction_ = 0;
	interceptors_.doCall( this );

	if ( dointercept_ )
	{
	    if ( !interceptaction_ )
		return -1;

	    interceptaction_->triggered.trigger();
	    return interceptaction_->getID();
	}
    }

    QAction* qaction = qmenu_->exec(QCursor::pos());
    return getID( qaction );
}


void uiMenu::doIntercept( bool yn, uiAction* interceptaction )
{
    if ( !dointercept_ && yn )
    {
	dointercept_ = yn;
	interceptaction_ = interceptaction;
    }
    else if ( dointercept_ && yn )
    {
	pErrMsg( "Can handle multiple passive, \
		  but only one active popup menu interceptor" );
    }
}


void uiMenu::removeInterceptor( const CallBack& cb )
{
    interceptors_ -= cb;
}


void uiMenu::addInterceptor( const CallBack& cb )
{
    interceptors_ += cb;
}


void uiMenu::doInsertMenu(mQtclass(QMenu)* menu,
			mQtclass(QAction)* before)
{
    qmenu_->insertMenu( before, menu );
}


void uiMenu::doInsertAction(mQtclass(QAction)* action,
			  mQtclass(QAction)* before)
{
    qmenu_->insertAction( before, action );
}


void uiMenu::doInsertSeparator(QAction* before)
{
    qmenu_->insertSeparator( before );
}


void uiMenu::doClear()
{
    qmenu_->clear();
}


void uiMenu::doRemoveAction( mQtclass(QAction)* action )
{
    qmenu_->removeAction( action );
}


QWidget* uiMenu::getWidget()
{ return qmenu_; }
