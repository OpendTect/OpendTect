/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.cc,v 1.15 2004-05-10 14:19:56 macman Exp $
________________________________________________________________________

-*/

#include "uidockwin.h"
#include "uigroup.h"
#include "uiparentbody.h"

#include <qdockwindow.h>

#ifdef __mac__
# define _machack_
# include "timer.h"
#endif

class uiDockWinBody : public uiParentBody
		    , public QDockWindow
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const char* nm );

    virtual		~uiDockWinBody();
    void		construct();

#define mHANDLE_OBJ     uiDockWin
#define mQWIDGET_BASE   QDockWindow
#define mQWIDGET_BODY   QDockWindow
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

protected:

    virtual void	finalise();

#ifdef _machack_

// the doc windows are not correctly drawn on the mac at pop-up

    virtual void        resizeEvent( QResizeEvent* ev )
    {
	QDockWindow::resizeEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

// not at moves...
    virtual void        moveEvent( QMoveEvent* ev )
    {
	QDockWindow::moveEvent(ev);

	if ( redrtimer.isActive() ) redrtimer.stop();
	redrtimer.start( 300, true );
    }

    void		redrTimTick( CallBacker* cb )
    {
	//TODO: check this out.  Are scenes deleted or 'just' hidden??
	if ( isHidden() ) return;

	hide();
	show();
    }

    static Timer	redrtimer;
    CallBack		mycallback_;
#endif

};

#ifdef _machack_
Timer uiDockWinBody::redrtimer;
#endif


uiDockWinBody::uiDockWinBody( uiDockWin& handle__, uiParent* parnt, 
			      const char* nm )
	: uiParentBody( nm )
	, QDockWindow( InDock, parnt && parnt->pbody() ?  
					parnt->pbody()->qwidget() : 0, nm ) 
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
#ifdef _machack_
	, mycallback_( mCB(this, uiDockWinBody, redrTimTick) )
#endif
{
    if ( *nm ) setCaption( nm );
#ifdef _machack_
    redrtimer.tick.notify( mycallback_ );
    redrtimer.start( 500, true );
#endif
}

void uiDockWinBody::construct()
{
    centralWidget_ = new uiGroup( &handle(), "uiDockWin central widget" );
    setWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);
    centralWidget_->setBorder(0);
    centralWidget_->setStretch(2,2);

    initing = false;
}


uiDockWinBody::~uiDockWinBody( )
{
#ifdef _machack_
    redrtimer.tick.remove( mycallback_ );
#endif
    //delete centralWidget_;
}

void uiDockWinBody::finalise()
    { centralWidget_->finalise();  finaliseChildren(); }


uiDockWin::uiDockWin( uiParent* parnt, const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )
{ 
    body_= new uiDockWinBody( *this, parnt, nm ); 
    setBody( body_ );
    body_->construct();
}


uiDockWin::~uiDockWin()
{ 
    delete body_; 
}


uiGroup* uiDockWin::topGroup()	    	   
{ 
    return body_->uiCentralWidg(); 
}


uiObject* uiDockWin::mainobject()
{ 
    return body_->uiCentralWidg()->mainObject(); 
}

void uiDockWin::setHorStretchable( bool yn )
{
    body_->setHorizontallyStretchable( yn );
}

bool uiDockWin::isHorStretchable() const
{
    return body_->isHorizontallyStretchable();
}


void uiDockWin::setVerStretchable( bool yn )
{
    body_->setVerticallyStretchable( yn );
}


bool uiDockWin::isVerStretchable() const
{
    return body_->isVerticallyStretchable();
}


void uiDockWin::setResizeEnabled( bool yn )
{
    body_->setResizeEnabled( yn );
}

bool uiDockWin::isResizeEnabled() const
{
    return body_->isResizeEnabled();
}

QDockWindow* uiDockWin::qwidget()
    { return body_; }
