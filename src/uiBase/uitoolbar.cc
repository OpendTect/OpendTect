/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          30/05/2001
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uitoolbar.h"

#include "uiaction.h"
#include "uitoolbutton.h"
#include "uimainwin.h"
#include "uiparentbody.h"

#include "bufstringset.h"
#include "menuhandler.h"
#include "pixmap.h"
#include "separstr.h"

#include <QToolBar>
#include "i_qtoolbar.h"

mUseQtnamespace

const char* uiIcon::save()		{ return "save"; }
const char* uiIcon::saveAs()		{ return "saveas"; }
const char* uiIcon::openObject()	{ return "openstorage"; }
const char* uiIcon::newObject()		{ return "newstorage"; }
const char* uiIcon::removeObject()	{ return "trashcan"; }
const char* uiIcon::None()		{ return "-"; }

ObjectSet<uiToolBar>& uiToolBar::toolBars()
{
    static ObjectSet<uiToolBar>* ret = 0;
    if ( !ret )
	ret = new ObjectSet<uiToolBar>;
    return *ret;
}



uiToolBar::uiToolBar( uiParent* parnt, const char* nm, ToolBarArea tba,
		      bool newline )
    : uiParent(nm,0)
    , tbarea_(tba)
    , buttonClicked(this)
    , toolbarmenuaction_(0)
    , qtoolbar_(new QToolBar(QString(nm), parnt ? parnt->getWidget() : 0))
{
    qtoolbar_->setObjectName( nm );
    msgr_ = new i_ToolBarMessenger( qtoolbar_, this );

    mDynamicCastGet(uiMainWin*,uimw,parnt)
    if ( uimw )
    {
	if ( newline ) uimw->addToolBarBreak();
	uimw->addToolBar( this );
    }

    toolBars() += this;
}


uiToolBar::~uiToolBar()
{
    removeAllActions();

    delete qtoolbar_;
    delete msgr_;

    toolBars() -= this;
}


int uiToolBar::addButton( const char* fnm, const char* tt, const CallBack& cb,
			  bool toggle )
{
    uiToolButtonSetup su( fnm, tt, cb );
    su.istoggle( toggle );
    return addButton( su );
}


int uiToolBar::addButton( const uiToolButtonSetup& su )
{    
    uiAction* action = new uiAction( su.name_, su.cb_, su.filename_ );
    action->setToolTip( su.tooltip_ );
    action->setCheckable( su.istoggle_ );
    action->setShortcut( su.shortcut_ );
    action->setChecked( su.ison_ );
    if ( su.arrowtype_!=uiToolButton::NoArrow )
    {
	pErrMsg("Not implemented yet");
    }
    
    return insertAction( action );
}


int uiToolBar::addButton( const MenuItem& itm )
{
    return insertAction( new uiAction(itm), itm.id );
}


void uiToolBar::addButton( uiButton* button )
{
    addObject( button );
}


void uiToolBar::addObject( uiObject* obj )
{
    QWidget* qw = obj && obj->body() ? obj->body()->qwidget() : 0;
    if ( qw )
    {
	qtoolbar_->addWidget( qw );
	mDynamicCastGet(uiToolButton*,button,obj)
	if ( !button ) qw->setMaximumHeight( uiObject::iconSize() );
	addedobjects_ += obj;
    }
    else
    {
	pErrMsg("Not a valid object");
    }
};


void uiToolBar::setLabel( const char* lbl )
{
    qtoolbar_->setWindowTitle( QString(lbl) );
    setName( lbl );
}

#define mGetAction( conststatement, erraction ) \
conststatement uiAction* action = const_cast<uiToolBar*>(this)->findAction( id ); \
if ( !action ) \
{ \
    pErrMsg("Action not found"); \
    erraction; \
}


void uiToolBar::turnOn( int id, bool yn )
{
    mGetAction( ,return );
    action->setChecked( yn );
}

bool uiToolBar::isOn( int id ) const
{
    mGetAction( const, return false );
    return action->isChecked();
}


void uiToolBar::setSensitive( int id, bool yn )
{
    mGetAction( , return );
    
    action->setEnabled( yn );
}


void uiToolBar::setSensitive( bool yn )
{
    qtoolbar_->setEnabled( yn );
}

bool uiToolBar::isSensitive() const
{ return qtoolbar_->isEnabled(); }


void uiToolBar::setToolTip( int id, const char* tip )
{
    mGetAction( , return );
    action->setToolTip( tip );
}

void uiToolBar::setShortcut( int id, const char* sc )
{
    mGetAction( , return );
    action->setShortcut( sc );
}


void uiToolBar::setToggle( int id, bool yn )
{
    mGetAction( , return );
    action->setCheckable( yn );
}


void uiToolBar::setIcon( int id, const char* fnm )
{
    setIcon( id, ioPixmap(fnm) );
}


void uiToolBar::setIcon( int id, const ioPixmap& pm )
{
    mGetAction( , return );
    action->setIcon( pm );
}


void uiToolBar::setButtonMenu( int id, uiMenu* mnu )
{
    mGetAction( , return );
    action->setMenu( mnu );
}


void uiToolBar::display( bool yn, bool, bool )
{
    if ( yn )
	qtoolbar_->show();
    else
	qtoolbar_->hide();

    if ( toolbarmenuaction_ ) 
	toolbarmenuaction_->setChecked( yn );
}


bool uiToolBar::isHidden() const
{ return qtoolbar_->isHidden(); }


bool uiToolBar::isVisible() const
{ return qtoolbar_->isVisible(); }


void uiToolBar::setToolBarMenuAction( uiAction* action )
{
    toolbarmenuaction_ = action;
    if ( toolbarmenuaction_ )
	toolbarmenuaction_->setChecked( !isHidden() );
}


void uiToolBar::clear()
{
    removeAllActions();
    addedobjects_.erase();
}


void uiToolBar::translate()
{
    uiActionContainer::translate();
    for ( int idx=0; idx<addedobjects_.size(); idx++ )
	addedobjects_[idx]->translate();
}


void uiToolBar::doInsertMenu(mQtclass(QMenu)* menu,
			  mQtclass(QAction)* before)
{
    pErrMsg("Not implemented. Should not be called");
}


void uiToolBar::doInsertAction(mQtclass(QAction)* action,
			       mQtclass(QAction)* before)
{
    qtoolbar_->insertAction( before, action );
}


void uiToolBar::doInsertSeparator(QAction* before)
{
    qtoolbar_->insertSeparator( before );
}


void uiToolBar::doClear()
{
    qtoolbar_->clear();
}


void uiToolBar::doRemoveAction( mQtclass(QAction)* action )
{
    qtoolbar_->removeAction( action );
}


void uiToolBar::getEntityList( ObjectSet<const CallBacker>& entities ) const
{
    entities.erase();

    for ( int actidx=0; actidx<qtoolbar_->actions().size(); actidx++ )
    {
	QAction* qaction = qtoolbar_->actions()[actidx];
	const int id = getID( qaction );
	const uiAction* action = const_cast<uiToolBar*>(this)->findAction( id );

	if ( !action )
	{
	    const QWidget* qw = qtoolbar_->widgetForAction( qaction );
	    for ( int objidx=0; objidx<addedobjects_.size(); objidx++ )
	    {
		if ( qw==addedobjects_[objidx]->qwidget() )
		{
		    entities += addedobjects_[objidx];
		    break;
		}
	    }
	}
	else
	    entities += action;
    }
}
