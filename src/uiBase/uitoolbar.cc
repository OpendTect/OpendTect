/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.36 2007-09-11 14:27:28 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"

#include "uibody.h"
#include "uimainwin.h"
#include "uiobj.h"
#include "uiparentbody.h"

#include "bufstringset.h"
#include "filegen.h"
#include "pixmap.h"
#include "separstr.h"

#include <qtoolbutton.h>
#include <qapplication.h>

#ifdef USEQT3
# include <qtoolbar.h>
# include <qmainwindow.h>
# define mDockNmSpc	QMainWindow
#else
# include <QAction>
# include <QMainWindow>
# include <QToolBar>
# define mDockNmSpc	Qt
#endif

#include "qobject.h"
#include "i_qtoolbut.h"


class uiToolBarBody : public uiParentBody
{
public:

			uiToolBarBody(uiToolBar&,QToolBar&);

			~uiToolBarBody()	{ deepErase(receivers); }


    int 		addButton(const ioPixmap&,const CallBack&, 
				  const char*,bool);
    int 		addButton(const char*,const CallBack&,const char*,bool);
    void		addObject(uiObject*);
    void		turnOn(int idx, bool yn );
    bool		isOn(int idx ) const;
    void		setSensitive(int idx, bool yn );
    void		setSensitive(bool yn);
    bool		isSensitive() const;
    void		setToolTip(int,const char*);
    void		setShortcut(int,const char*);
    void		setPixmap(int,const char*);
    void		setPixmap(int,const ioPixmap&);

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

    void		reLoadPixMaps();

    static mDockNmSpc::ToolBarDock
			qdock(uiToolBar::ToolBarDock);

protected:

    virtual const QWidget*      managewidg_() const	{ return qbar; }
    virtual const QWidget*	qwidget_() const	{ return qbar; }
    QToolBar*			qbar;
    uiToolBar&			tbar;
    int				iconsz_;

    virtual void        attachChild ( constraintType tp,
                                      uiObject* child,
                                      uiObject* other, int margin,
                                      bool reciprocal )
			{
			    pErrMsg("Cannot attach uiObjects in a uiToolBar");
			    return;
			}


private:

    ObjectSet<i_QToolButReceiver> receivers; // for deleting
#ifdef USEQT3
    ObjectSet<QToolButton>	buttons;
#else
    ObjectSet<QAction>		actions;
#endif
    BufferStringSet		pmsrcs;

};


uiToolBarBody::uiToolBarBody( uiToolBar& handle, QToolBar& bar )
    : uiParentBody("ToolBar")
    , qbar(&bar)
    , tbar(handle)
{
}


int uiToolBarBody::addButton( const char* fnm, const CallBack& cb,
			      const char* nm, bool toggle )
{ return addButton( ioPixmap(fnm), cb, nm, toggle ); }


#ifdef USEQT3
int uiToolBarBody::addButton( const ioPixmap& pm, const CallBack& cb,
			      const char* nm, bool toggle )
{
    i_QToolButReceiver* br= new i_QToolButReceiver;
    QToolButton* but= new QToolButton( *pm.qpixmap(), QString(nm),
	    			       QString::null, br,
				       SLOT(buttonPressed()),qbar, nm );
    if ( toggle ) but->setToggleButton( true );

    receivers += br;
    buttons += but;
    pmsrcs.add( pm.source() );

    br->pressed.notify( cb );
    return buttons.size()-1;
}
#else
int uiToolBarBody::addButton( const ioPixmap& pm, const CallBack& cb,
			      const char* nm, bool toggle )
{
    i_QToolButReceiver* br= new i_QToolButReceiver;
    QAction* qaction = qbar->addAction( *pm.qpixmap(), nm, br,
					SLOT(buttonPressed()) );
    if ( toggle ) qaction->setCheckable( true );

    receivers += br;
    actions += qaction;
    pmsrcs.add( pm.source() );

    br->pressed.notify( cb );
    return actions.size()-1;
}
#endif


void uiToolBarBody::addObject( uiObject* obj )
{
#ifndef USEQT3
    if ( obj && obj->body() )
	qbar->addWidget( obj->body()->qwidget() );
#endif
}


void uiToolBarBody::turnOn( int idx, bool yn )
{
#ifdef USEQT3
    buttons[idx]->setOn( yn );
#else
    actions[idx]->setChecked( yn );
#endif
}


void uiToolBarBody::reLoadPixMaps()
{
    for ( int idx=0; idx<pmsrcs.size(); idx++ )
    {
	const BufferString& pmsrc = pmsrcs.get( idx );
	if ( pmsrc[0] == '[' )
	    continue;

	FileMultiString fms( pmsrc );
	const int len = fms.size();
	const BufferString fnm( fms[0] );
	const ioPixmap pm( fnm.buf(), len > 1 ? fms[1] : 0 );
	if ( pm.isEmpty() )
	    continue;
#ifdef USEQT3
	buttons[idx]->setPixmap( *pm.qpixmap() );
#else
	actions[idx]->setIcon( *pm.qpixmap() );
#endif
    }
}


bool uiToolBarBody::isOn( int idx ) const
{
#ifdef USEQT3
    return buttons[idx]->isOn();
#else
    return actions[idx]->isChecked();
#endif
}


void uiToolBarBody::setSensitive( int idx, bool yn )
{
#ifdef USEQT3
    buttons[idx]->setEnabled( yn );
#else
    actions[idx]->setEnabled( yn );
#endif
}


void uiToolBarBody::setPixmap( int idx, const char* fnm )
{
    setPixmap( idx, ioPixmap(fnm) );
}


void uiToolBarBody::setPixmap( int idx, const ioPixmap& pm )
{
#ifdef USEQT3
    buttons[idx]->setPixmap( *pm.qpixmap() );
#else
    actions[idx]->setIcon( *pm.qpixmap() );
#endif
}


void uiToolBarBody::setToolTip( int idx, const char* tip )
{
#ifdef USEQT3
    buttons[idx]->setTextLabel( QString(tip) );
#else
    actions[idx]->setToolTip( QString(tip) );
#endif
}


void uiToolBarBody::setShortcut( int idx, const char* sc )
{
#ifndef USEQT3
    actions[idx]->setShortcut( QString(sc) );
#endif
}


void uiToolBarBody::setSensitive( bool yn )
{
    if ( qwidget() ) qwidget()->setEnabled( yn );
}


bool uiToolBarBody::isSensitive() const
{
    return qwidget() ? qwidget()->isEnabled() : false;
}


mDockNmSpc::ToolBarDock uiToolBarBody::qdock( uiToolBar::ToolBarDock d )
{
    switch( d )
    {
	case uiToolBar::Top:		return mDockNmSpc::Top;
	case uiToolBar::Bottom:		return mDockNmSpc::Bottom;
	case uiToolBar::Right:		return mDockNmSpc::Right;
	case uiToolBar::Left:		return mDockNmSpc::Left;
	case uiToolBar::Minimized:	return mDockNmSpc::Minimized;
    }
    return (mDockNmSpc::ToolBarDock) 0;
}


ObjectSet<uiToolBar>& uiToolBar::toolBars()
{
    static ObjectSet<uiToolBar>* ret = 0;
    if ( !ret )
	ret = new ObjectSet<uiToolBar>;
    return *ret;
}


uiToolBar::uiToolBar( uiParent* parnt, const char* nm, ToolBarDock d,
		      bool newline )
    : uiParent(nm,0)
{
    //TODO: impl preferred dock and newline
    Qt::ToolBarDock tbdock = uiToolBarBody::qdock(d);
    QWidget* qwidget = parnt && parnt->pbody() ? parnt->pbody()->qwidget() : 0;
    qtoolbar = new QToolBar( QString(nm), (QMainWindow*)qwidget );
    setBody( &mkbody(nm,*qtoolbar) );
    toolBars() += this;

    mDynamicCastGet(uiMainWin*,uimw,parnt)
    if ( uimw ) uimw->addToolBar( this );
}


uiToolBar::~uiToolBar()
{
    toolBars() -= this;
    delete body_;
}


uiToolBarBody& uiToolBar::mkbody( const char* nm, QToolBar& qtb )
{ 
    body_ = new uiToolBarBody( *this, qtb );
    return *body_; 
}


int uiToolBar::addButton( const char* fnm, const CallBack& cb,
			  const char* nm, bool toggle )
{ return body_->addButton( fnm, cb, nm, toggle ); }


int uiToolBar::addButton( const ioPixmap& pm, const CallBack& cb,
			  const char* nm, bool toggle )
{ return body_->addButton( pm, cb, nm, toggle ); }


void uiToolBar::addObject( uiObject* obj )
{ body_->addObject( obj ); };


void uiToolBar::setLabel( const char* lbl )
{
    qtoolbar->setLabel( QString(lbl) );
    setName( lbl );
}


void uiToolBar::turnOn( int idx, bool yn )
{ body_->turnOn( idx, yn ); }

bool uiToolBar::isOn( int idx ) const
{ return body_->isOn( idx ); }

void uiToolBar::setSensitive( int idx, bool yn )
{ body_->setSensitive( idx, yn ); }

void uiToolBar::setSensitive( bool yn )
{ body_->setSensitive( yn ); }

void uiToolBar::setToolTip( int idx, const char* tip )
{ body_->setToolTip( idx, tip ); }

void uiToolBar::setShortcut( int idx, const char* sc )
{ body_->setShortcut( idx, sc ); }

void uiToolBar::setPixmap( int idx, const char* fnm )
{ body_->setPixmap( idx, fnm ); }

void uiToolBar::setPixmap( int idx, const ioPixmap& pm )
{ body_->setPixmap( idx, pm ); }


void uiToolBar::display( bool yn, bool, bool )
{
    if ( yn )
	qtoolbar->show();
    else
	qtoolbar->hide();
}


void uiToolBar::addSeparator()
{ qtoolbar->addSeparator(); }

#ifdef USEQT3
void uiToolBar::dock()
{ qtoolbar->dock(); }


void uiToolBar::undock()
{ qtoolbar->undock(); }
#endif


void uiToolBar::setStretchableWidget( uiObject* obj )
{
    if ( !obj ) return;
#ifdef USEQT3
    qtoolbar->setStretchableWidget( obj->body()->qwidget() );
#endif
}


void uiToolBar::reLoadPixMaps()
{ body_->reLoadPixMaps(); }


void uiToolBar::clear()
{ qtoolbar->clear(); }


#define mSetFunc(func,var) \
void uiToolBar::func( var value ) \
{ \
    qtoolbar->func( value ); \
}

#define mGetFunc(func,var) \
var uiToolBar::func() const \
{ \
    return qtoolbar->func(); \
}

#ifdef USEQT3
mSetFunc(setMovingEnabled,bool)
mGetFunc(isMovingEnabled,bool)

mSetFunc(setNewLine,bool)

mSetFunc(setCloseMode,int)
mGetFunc(closeMode,int)

mSetFunc(setHorizontallyStretchable,bool)
mGetFunc(isHorizontallyStretchable,bool)

mSetFunc(setVerticallyStretchable,bool)
mGetFunc(isVerticallyStretchable,bool)

mSetFunc(setResizeEnabled,bool)
mGetFunc(isResizeEnabled,bool)
#endif

mGetFunc(isShown,bool)
