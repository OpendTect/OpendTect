/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.27 2006-07-03 16:39:47 cvsbert Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"
#include "uimain.h"
#include "uibody.h"
#include "uiobj.h"
#include "uimainwin.h"
#include "uiparentbody.h"
#include "pixmap.h"
#include "separstr.h"
#include "filegen.h"
#include "bufstringset.h"

#include <qtoolbutton.h>
#include <qapplication.h>

#ifdef USEQT4
# include <q3toolbar.h>
# include <q3mainwindow.h>
# include <q3dockarea.h>
# define mQMainWinClss	Q3MainWindow
# define mDockNmSpc	Qt
#else
# include <qtoolbar.h>
# include <qmainwindow.h>
# define mQMainWinClss	QMainWindow
# define mDockNmSpc	QMainWindow
#endif

#include "qobject.h"
#include "i_qtoolbut.h"


class uiToolBarBody : public uiParentBody
{
public:

			uiToolBarBody( uiToolBar& handle, mQToolBarClss& bar )
			    : uiParentBody("ToolBar")
			    , qbar(&bar)
			    , tbar(handle)
			    {}

			~uiToolBarBody()	{ deepErase(receivers); }


    int 		addButton( const ioPixmap&, const CallBack& cb, 
				   const char* nm="ToolBarButton",
				   bool toggle=false );
    void		turnOn(int idx, bool yn );
    bool		isOn(int idx ) const;
    void		setSensitive(int idx, bool yn );
    void		setSensitive(bool yn);
    bool		isSensitive() const;
    void		setToolTip(int,const char*);
    void		setPixmap(int,const ioPixmap&);

    void		display(bool yn=true);
			//!< you must call this after all buttons are added

    void		reLoadPixMaps();


    static mDockNmSpc::ToolBarDock
			qdock(uiToolBar::ToolBarDock);

protected:

    virtual const QWidget*      managewidg_() const	{ return qbar; }
    virtual const QWidget*	qwidget_() const	{ return qbar; }
    mQToolBarClss*	qbar;
    uiToolBar&			tbar;
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
    ObjectSet<QToolButton>	buttons;
    BufferStringSet		pmsrcs;

};


int uiToolBarBody::addButton(const ioPixmap& pm, const CallBack& cb,
			      const char* nm, bool toggle )
{
    i_QToolButReceiver* br= new i_QToolButReceiver;
    QToolButton* but= new QToolButton( *pm.Pixmap(), QString(nm), QString::null,
                           br,  SLOT(buttonPressed()),qbar, nm );

    if ( toggle ) but->setToggleButton( true );

    receivers += br;
    buttons += but;
    pmsrcs.add( pm.source() );

    br->pressed.notify( cb );
    return buttons.size()-1;
}


void uiToolBarBody::turnOn( int idx, bool yn )
{
    buttons[idx]->setOn( yn );
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
	if ( !File_exists(fnm) )
	    continue;

	buttons[idx]->setPixmap( QPixmap(fnm.buf(), len > 1 ? fms[1] : 0) );
    }
}


bool uiToolBarBody::isOn( int idx ) const
{
    return buttons[idx]->isOn();
}


void uiToolBarBody::setSensitive( int idx, bool yn )
{
    buttons[idx]->setEnabled( yn );
}


void uiToolBarBody::setPixmap( int idx, const ioPixmap& pm )
{
    buttons[idx]->setPixmap( *pm.Pixmap() );
}


void uiToolBarBody::setToolTip( int idx, const char* tip )
{
    buttons[idx]->setTextLabel( QString(tip) );
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
    Qt::ToolBarDock tbdock = uiToolBarBody::qdock(d);
    QWidget* qwidget = parnt && parnt->pbody() ? parnt->pbody()->qwidget() : 0;
    qtoolbar = new mQToolBarClss( QString(nm), (mQMainWinClss*)qwidget, 
	    		     tbdock, newline );
    setBody( &mkbody(nm,*qtoolbar) );
    toolBars() += this;
}


uiToolBar::~uiToolBar()
{
    toolBars() -= this;
    delete body_;
}


uiToolBarBody& uiToolBar::mkbody( const char* nm, mQToolBarClss& qtb )
{ 
    body_ = new uiToolBarBody( *this, qtb );
    return *body_; 
}


int uiToolBar::addButton( const ioPixmap& pm, const CallBack& cb,
			  const char* nm, bool toggle )
{
    return body_->addButton( pm, cb, nm, toggle );
}


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
{
    body_->setToolTip( idx, tip );
}


void uiToolBar::setPixmap( int idx, const ioPixmap& pm )
{
    body_->setPixmap( idx, pm );
}


void uiToolBar::display( bool yn, bool, bool )
{
    if ( yn )
	qtoolbar->show();
    else
	qtoolbar->hide();
}


void uiToolBar::addSeparator()
{ qtoolbar->addSeparator(); }

void uiToolBar::dock()
{ qtoolbar->dock(); }

void uiToolBar::undock()
{ qtoolbar->undock(); }


void uiToolBar::setStretchableWidget( uiObject* obj )
{
    if ( !obj ) return;
    qtoolbar->setStretchableWidget( obj->body()->qwidget() );
}


void uiToolBar::reLoadPixMaps()
{
    body_->reLoadPixMaps();
}


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

mGetFunc(isShown,bool)
