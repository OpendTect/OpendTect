/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          30/05/2001
 RCS:           $Id: uitoolbar.cc,v 1.38 2007-09-19 17:18:50 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uitoolbar.h"

#include "uibody.h"
#include "uibutton.h"
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

			~uiToolBarBody();


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

    const ObjectSet<uiObject>& 
			objectList() const		{ return objects_; }

protected:

    virtual const QWidget*      managewidg_() const	{ return qbar_; }
    virtual const QWidget*	qwidget_() const	{ return qbar_; }
    QToolBar*			qbar_;
    uiToolBar&			tbar_;
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
    BufferStringSet		pmsrcs_;
    
    ObjectSet<uiObject>		objects_; 
    TypeSet<int>		butindex_;

};


uiToolBarBody::uiToolBarBody( uiToolBar& handle, QToolBar& bar )
    : uiParentBody("ToolBar")
    , qbar_(&bar)
    , tbar_(handle)
{
}


uiToolBarBody::~uiToolBarBody()	
{ 
    for ( int idx=0; idx<butindex_.size(); idx++ )
	delete objects_[butindex_[idx]];
}


int uiToolBarBody::addButton( const char* fnm, const CallBack& cb,
			      const char* nm, bool toggle )
{ return addButton( ioPixmap(fnm), cb, nm, toggle ); }


int uiToolBarBody::addButton( const ioPixmap& pm, const CallBack& cb,
			      const char* nm, bool toggle )
{
    uiToolButton* toolbarbut = new uiToolButton( &tbar_, nm, pm, cb );
    toolbarbut->setToggleButton(toggle);
    toolbarbut->setToolTip( nm );
    butindex_ += objects_.size();
    addObject( toolbarbut );
    pmsrcs_.add( pm.source() );

    return butindex_.size()-1;
}


void uiToolBarBody::addObject( uiObject* obj )
{
#ifndef USEQT3
    if ( obj && obj->body() )
    {
	qbar_->addWidget( obj->body()->qwidget() );
	objects_ += obj;
    }
#endif
}


#define mToolBarBut(idx) dynamic_cast<uiToolButton*>(objects_[butindex_[idx]])
#define mConstToolBarBut(idx) \
	dynamic_cast<const uiToolButton*>(objects_[butindex_[idx]])

void uiToolBarBody::turnOn( int idx, bool yn )
{ mToolBarBut(idx)->setOn( yn ); }


void uiToolBarBody::reLoadPixMaps()
{
    for ( int idx=0; idx<pmsrcs_.size(); idx++ )
    {
	const BufferString& pmsrc = pmsrcs_.get( idx );
	if ( pmsrc[0] == '[' )
	    continue;

	FileMultiString fms( pmsrc );
	const int len = fms.size();
	const BufferString fnm( fms[0] );
	const ioPixmap pm( fnm.buf(), len > 1 ? fms[1] : 0 );
	if ( pm.isEmpty() )
	    continue;

	mToolBarBut(idx)->setPixmap( pm );
    }
}


bool uiToolBarBody::isOn( int idx ) const
{ return mConstToolBarBut(idx)->isOn(); }


void uiToolBarBody::setSensitive( int idx, bool yn )
{ mToolBarBut(idx)->setSensitive( yn ); }


void uiToolBarBody::setPixmap( int idx, const char* fnm )
{ setPixmap( idx, ioPixmap(fnm) ); }


void uiToolBarBody::setPixmap( int idx, const ioPixmap& pm )
{ mToolBarBut(idx)->setPixmap( pm ); }


void uiToolBarBody::setToolTip( int idx, const char* txt )
{ mToolBarBut(idx)->setToolTip( txt ); }


void uiToolBarBody::setShortcut( int idx, const char* sc )
{
#ifndef USEQT3
    mToolBarBut(idx)->setShortcut( sc );
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
    , parent_(parnt)
{
    //TODO: impl preferred dock and newline
    Qt::ToolBarDock tbdock = uiToolBarBody::qdock(d);
    QWidget* qwidget = parnt && parnt->pbody() ? parnt->pbody()->qwidget() : 0;
    qtoolbar_ = new QToolBar( QString(nm), (QMainWindow*)qwidget );
    setBody( &mkbody(nm,*qtoolbar_) );
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
    qtoolbar_->setLabel( QString(lbl) );
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
	qtoolbar_->show();
    else
	qtoolbar_->hide();

    mDynamicCastGet( uiMainWin*, uimw, parent() )
    if ( uimw ) uimw->updateToolbarsMenu();
}


bool uiToolBar::isHidden() const
{ return qtoolbar_->isHidden(); }


bool uiToolBar::isVisible() const
{ return qtoolbar_->isVisible(); }


void uiToolBar::addSeparator()
{ qtoolbar_->addSeparator(); }


#ifdef USEQT3
void uiToolBar::dock()
{ qtoolbar_->dock(); }


void uiToolBar::undock()
{ qtoolbar_->undock(); }
#endif


void uiToolBar::setStretchableWidget( uiObject* obj )
{
    if ( !obj ) return;
#ifdef USEQT3
    qtoolbar_->setStretchableWidget( obj->body()->qwidget() );
#endif
}


void uiToolBar::reLoadPixMaps()
{ body_->reLoadPixMaps(); }


void uiToolBar::clear()
{ qtoolbar_->clear(); }


const ObjectSet<uiObject>& uiToolBar::objectList() const
{ return body_->objectList(); }


#define mSetFunc(func,var) \
void uiToolBar::func( var value ) \
{ \
    qtoolbar_->func( value ); \
}

#define mGetFunc(func,var) \
var uiToolBar::func() const \
{ \
    return qtoolbar_->func(); \
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
