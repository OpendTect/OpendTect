/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimenu.h"

#include "keystrs.h"

#include "uiaction.h"
#include "uibody.h"
#include "uimain.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "pixmap.h"
#include "texttranslator.h"
#include "uistring.h"

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

static CallBackSet interceptors_;

uiMenu::uiMenu( uiParent* p, const uiString& txt, const char* pmnm )
    : uiBaseObject( txt.getFullString() )
    , submenuaction_( 0 )
    , qmenu_( new mQtclass(QMenu)( p ? p->getWidget() : 0 ) )
    , text_(txt)
{
    qmenu_->setTitle( txt.getQtString() );
}


uiMenu::uiMenu( const uiString& txt, const char* pmnm )
    : uiBaseObject( txt.getFullString() )
    , submenuaction_( 0 )
    , qmenu_( new mQtclass(QMenu)( 0 ) )
    , text_(txt)
{
    qmenu_->setTitle( txt.getQtString() );
}


uiMenu::~uiMenu()
{
    removeAllActions();
    delete qmenu_;
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
