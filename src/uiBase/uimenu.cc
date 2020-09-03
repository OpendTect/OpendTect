/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          26/04/2000
________________________________________________________________________

-*/

#include "uimenu.h"

#include "uiaction.h"
#include "uibody.h"
#include "uiicon.h"
#include "uimain.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "uimainwin.h"
#include "uistring.h"

#include "menuhandler.h"
#include "texttranslation.h"

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


QWidget* uiMenuBar::getWidget( int )
{ return qmenubar_; }

// -----------------------------------------------------------------------

static CallBackSet& interceptors_ = *new CallBackSet;

static uiParent* gtParent( uiParent* p )
{
    if ( !p || p->getNrWidgets()<1 )
	p = uiMain::theMain().topLevel();
    return p;
}


uiMenu::uiMenu( uiParent* p, const uiString& txt, const char* pmnm )
    : uiBaseObject( toString(txt) )
    , submenuaction_( 0 )
    , qmenu_( new mQtclass(QMenu)(toQString(txt), gtParent(p)->getWidget(0) ))
    , text_(txt)
{
    setIcon( pmnm );
    useStyleSheet();
}


uiMenu::uiMenu( const uiString& txt, const char* pmnm )
    : uiBaseObject( toString(txt) )
    , submenuaction_( 0 )
    , qmenu_(new mQtclass(QMenu)(toQString(txt)))
    , text_(txt)
{
    setIcon( pmnm );
    useStyleSheet();
}


uiMenu::uiMenu( const MenuItem& itm )
    : uiBaseObject( toString(itm.text) )
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
}


uiMenu* uiMenu::addSubMenu( uiParent* par, const uiString& nm,
			    const char* icnm )
{
    uiMenu* submnu = new uiMenu( par, nm, icnm );
    addMenu( submnu );
    return submnu;
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
	    uiAction* mnuitem = new uiAction( subitm.text );
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
    uiBaseObject::setName( toString(txt) );
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


bool uiMenu::isEmpty() const
{
    return qmenu_->isEmpty();
}


void uiMenu::doClear()
{
    qmenu_->clear();
}


void uiMenu::doRemoveAction( mQtclass(QAction)* action )
{
    qmenu_->removeAction( action );
}


QWidget* uiMenu::getWidget(int)
{ return qmenu_; }
