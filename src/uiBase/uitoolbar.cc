/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolbar.h"

#include "uiaction.h"
#include "uibaseobject.h"
#include "uimainwin.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uistrings.h"
#include "uitoolbutton.h"

#include "bufstringset.h"
#include "menuhandler.h"

#include "q_uiimpl.h"

#include <QToolBar>
#include <QToolButton>
#include "i_qtoolbar.h"

mUseQtnamespace

ObjectSet<uiToolBar>& uiToolBar::toolBars()
{
    mDefineStaticLocalObject( ObjectSet<uiToolBar>, ret, );
    return ret;
}



uiToolBar::uiToolBar( uiParent* parnt, const uiString& nm, ToolBarArea tba,
		      bool newline )
    : uiParent(nm.getFullString(),nullptr)
    , buttonClicked(this)
    , orientationChanged(this)
    , tbarea_(tba)
    , parent_(parnt)
{
    label_ = nm;

    qtoolbar_ = new QToolBar( toQString(nm),
			      parnt ? parnt->getWidget() : nullptr );
    qtoolbar_->setObjectName( toQString(nm) );
    msgr_ = new i_ToolBarMessenger( qtoolbar_, this );

    mDynamicCastGet(uiMainWin*,uimw,parnt)
    if ( uimw )
    {
	if ( newline )
	    uimw->addToolBarBreak();
	uimw->addToolBar( this );
    }

    const int iconsz = uiObject::iconSize();
    qtoolbar_->setIconSize( QSize(iconsz,iconsz) );

    toolBars() += this;
}


uiToolBar::uiToolBar( uiParent* p, const char* nm, ToolBarArea d, bool newline )
    : uiToolBar(p,toUiString(nm),d,newline)
{}


uiToolBar::~uiToolBar()
{
    removeAllActions();
    deepErase( addedobjects_ );

    delete qtoolbar_;
    delete msgr_;
    toolBars() -= this;
}


int uiToolBar::addButton( const char* fnm, const uiString& tt,
			  const CallBack& cb, bool toggle, int id )
{
    uiAction* action = new uiAction( uiStrings::sEmptyString(), cb, fnm );
    action->setToolTip( tt );
    action->setCheckable( toggle );
    return insertAction( action, id );
}


int uiToolBar::addButton( const uiToolButtonSetup& su )
{
    uiAction* action = new uiAction( su.name_, su.cb_, su.icid_ );
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
    if ( addedobjects_.isPresent(obj) )
	return;

    QWidget* qw = obj && obj->body() ? obj->body()->qwidget() : nullptr;
    if ( qw )
    {
	QAction* qact = qtoolbar_->addWidget( qw );
	qact->setVisible( true );
	mDynamicCastGet(uiToolButton*,button,obj)
	if ( !button )
	    qw->setMaximumHeight( uiObject::iconSize() );

	addedobjects_ += obj;
	addedactions_ += qact;
    }
    else
    {
	pErrMsg("Not a valid object");
    }
}


void uiToolBar::removeObject( uiObject* obj )
{
    const int idx = addedobjects_.indexOf( obj );
    if ( !addedactions_.validIdx(idx) )
	return;

    QAction* qaction = addedactions_[idx];
    qtoolbar_->removeAction( qaction );
    addedobjects_.removeSingle( idx );
    addedactions_.removeSingle( idx );
}


bool uiToolBar::hasObject( const uiObject* obj ) const
{
    return addedobjects_.isPresent( obj );
}


void uiToolBar::setLabel( const uiString& lbl )
{
    label_ = lbl;
    qtoolbar_->setWindowTitle( toQString(lbl) );
    setName( lbl.getFullString() );
}


uiString uiToolBar::getDispNm() const
{
    return label_;
}

#define mHandleErr(erraction) \
    if ( !action ) \
    { \
	pErrMsg("Action not found"); \
	erraction; \
    }

#define mGetConstAction( erraction ) \
    const uiAction* action = findAction( id );\
    mHandleErr(erraction)

#define mGetAction( erraction ) \
    uiAction* action = const_cast<uiAction*>( findAction(id) ); \
    mHandleErr(erraction)

void uiToolBar::turnOn( int id, bool yn )
{
    mGetAction( return );
    action->setChecked( yn );
}

bool uiToolBar::isOn( int id ) const
{
    mGetConstAction( return false );
    return action->isChecked();
}


void uiToolBar::setSensitive( int id, bool yn )
{
    mGetAction( return );

    action->setEnabled( yn );
}


void uiToolBar::setSensitive( bool yn )
{
    qtoolbar_->setEnabled( yn );
}

bool uiToolBar::isSensitive() const
{ return qtoolbar_->isEnabled(); }


void uiToolBar::setToolTip( int id, const uiString& tip )
{
    mGetAction( return );
    action->setToolTip( tip );
}

void uiToolBar::setShortcut( int id, const char* sc )
{
    mGetAction( return );
    action->setShortcut( sc );
}


void uiToolBar::setToggle( int id, bool yn )
{
    mGetAction( return );
    action->setCheckable( yn );
}


void uiToolBar::setIcon( int id, const char* fnm )
{
    mGetAction( return );
    action->setIcon( fnm );
}


void uiToolBar::setIcon( int id, const uiIcon& icon )
{
    mGetAction( return );
    action->setIcon( icon );
}


void uiToolBar::setButtonMenu( int id, uiMenu* mnu,
			       uiToolButton::PopupMode mode, bool preventclose )
{
    mGetAction( return );
    action->setMenu( mnu );
    mnu->preventClose( preventclose );
    QWidget* qw = qtoolbar_->widgetForAction( action->qaction() );
    mDynamicCastGet(QToolButton*,qtb,qw)
    if ( qtb )
	qtb->setPopupMode( (QToolButton::ToolButtonPopupMode)mode );
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


OD::Orientation uiToolBar::getOrientation() const
{
    return qtoolbar_->orientation()==Qt::Horizontal ?
		OD::Horizontal : OD::Vertical;
}


void uiToolBar::setToolBarMenuAction( uiAction* action )
{
    toolbarmenuaction_ = action;
    if ( toolbarmenuaction_ )
	toolbarmenuaction_->setChecked( !isHidden() );
}


void uiToolBar::handleFinalize( bool pre )
{
    if ( pre )
	preFinalize().trigger();

    for ( int idx=0; idx<addedobjects_.size(); idx++ )
    {
	uiObject& uiobj = *addedobjects_[idx];
	if ( pre )
	    uiobj.preFinalize().trigger();
	else
	    uiobj.finalize();
    }

    if ( !pre )
	postFinalize().trigger();
}


void uiToolBar::setTabOrder( uiObject* obj1, uiObject* obj2 )
{
    qtoolbar_->setTabOrder( obj1->getWidget(), obj2->getWidget() );
}


void uiToolBar::clear()
{
    removeAllActions();
    deepErase( addedobjects_ );
    addedactions_.erase();
}


void uiToolBar::translateText()
{
    if ( !label_.isEmpty() )
	qtoolbar_->setWindowTitle( toQString(label_) );

    for ( int idx=0; idx<addedobjects_.size(); idx++ )
	addedobjects_[idx]->translateText();
}


void uiToolBar::doInsertMenu( mQtclass(QMenu)*, mQtclass(QAction)* )
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
	const QAction* qaction = qtoolbar_->actions().at( actidx );
	const int id = getID( qaction );
	const uiAction* action = const_cast<uiToolBar*>(this)->findAction( id );

	if ( !action )
	{
	    const QWidget* qw =
		qtoolbar_->widgetForAction( cCast(QAction*,qaction) );
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
