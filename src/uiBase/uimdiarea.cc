/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uimdiarea.h"
#include "i_qmdiarea.h"

#include "uimsg.h"
#include "uiobjbody.h"
#include "bufstringset.h"

#include <QApplication>
#include <QCloseEvent>
#include <QIcon>
#include <QMdiSubWindow>

static bool sNoCloseMessage = false;

class uiMdiAreaBody : public uiObjBodyImpl<uiMdiArea,mQtclass(QMdiArea)>
{ 	
public:
    			uiMdiAreaBody(uiMdiArea&,uiParent*,const char*);
			~uiMdiAreaBody();

protected:
    i_MdiAreaMessenger& messenger_;

    void		resizeEvent(mQtclass(QResizeEvent*));
};


uiMdiAreaBody::uiMdiAreaBody( uiMdiArea& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiMdiArea,mQtclass(QMdiArea)>(hndle,p,nm)
    , messenger_(*new i_MdiAreaMessenger(this,&hndle))
{}

uiMdiAreaBody::~uiMdiAreaBody()
{ delete &messenger_; }


void uiMdiAreaBody::resizeEvent( mQtclass(QResizeEvent*) ev )
{
    // TODO: Resize subwindows
    QMdiArea::resizeEvent( ev );
}


uiMdiArea::uiMdiArea( uiParent* p, const char* nm )
    : uiObject(p,nm,mkbody(p,nm))
    , windowActivated(this)
{
    setStretch( 2, 2 );
}


uiMdiAreaBody& uiMdiArea::mkbody( uiParent* p, const char* nm )
{
    body_ = new uiMdiAreaBody( *this, p, nm );
    return *body_;
}


uiMdiArea::~uiMdiArea()
{ delete body_; }


void uiMdiArea::tile()		{ body_->tileSubWindows(); }
void uiMdiArea::cascade()	{ body_->cascadeSubWindows(); }


void uiMdiArea::tileVertical()
{
    tile();
    mQtclass(QList)<mQtclass(QMdiSubWindow*)> windows = body_->subWindowList();

    int nrvisiblewindows = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	mQtclass(QMdiSubWindow*) widget = windows.at( idx );
	if ( widget && !widget->isHidden() )
	    nrvisiblewindows++;
    }

    if ( nrvisiblewindows == 0 ) return;

    const int wswidth = body_->frameSize().width();
    const int wsheight = body_->frameSize().height();
    const int avgheight = wsheight / nrvisiblewindows;
    int y = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	mQtclass(QMdiSubWindow*) widget = windows.at( idx );
	if ( widget->isHidden() ) continue;

	widget->showNormal();
	const int prefheight = widget->minimumHeight() + 
			       widget->parentWidget()->baseSize().height();
	const int actheight = mMAX( avgheight, prefheight );
	widget->setGeometry( 0, y, wswidth, actheight );
	y += actheight;
    }
}


void uiMdiArea::tileHorizontal()
{
    tile();
    mQtclass(QList)<mQtclass(QMdiSubWindow*)> windows = body_->subWindowList();

    int nrvisiblewindows = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	mQtclass(QMdiSubWindow*) widget = windows.at( idx );
	if ( widget && !widget->isHidden() )
	    nrvisiblewindows++;
    }

    if ( nrvisiblewindows == 0 ) return;

    const int wswidth = body_->frameSize().width();
    const int wsheight = body_->frameSize().height();
    const int avgwidth = wswidth / nrvisiblewindows;
    int x = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	mQtclass(QMdiSubWindow*) widget = windows.at( idx );
	if ( widget->isHidden() ) continue;

	widget->showNormal();
	const int prefwidth = widget->minimumWidth() + 
			      widget->parentWidget()->baseSize().width();
	const int actwidth = mMAX( avgwidth, prefwidth );
	widget->setGeometry( x, 0, actwidth, wsheight );
	x += actwidth;
    }
}


void uiMdiArea::addWindow( uiMdiAreaWindow* grp )
{
    if ( !grp || !grp->pbody() ) return;

    body_->addSubWindow( grp->qWidget() );
    grp->closed().notify( mCB(this,uiMdiArea,grpClosed) );
    grp->changed.notify( mCB(this,uiMdiArea,grpChanged) );
    grps_ += grp;
    body_->setActiveSubWindow( grp->qWidget() );
    windowActivated.trigger();
}


uiMdiAreaWindow* uiMdiArea::getWindow( const char* nm )
{
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( !strcmp(nm,grps_[idx]->getTitle()) )
	    return grps_[idx];
    }

    return 0;
}


const uiMdiAreaWindow* uiMdiArea::getWindow( const char* nm ) const
{ return const_cast<uiMdiArea*>(this)->getWindow( nm ); }


void uiMdiArea::grpClosed( CallBacker* cb )
{
    mDynamicCastGet(uiObject*,uiobj,cb)
    if ( !uiobj ) return;

    uiMdiAreaWindow* grp = 0;
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( grps_[idx]->mainObject() != uiobj )
	    continue;

	grp = grps_[idx];
	break;
    }

    if ( grp )
    {
	body_->removeSubWindow( grp->qWidget() );
	grp->closed().remove( mCB(this,uiMdiArea,grpClosed) );
	grp->changed.remove( mCB(this,uiMdiArea,grpChanged) );
	grps_ -= grp;
    }
    windowActivated.trigger();
}


void uiMdiArea::grpChanged( CallBacker* )
{ windowActivated.trigger(); }


void uiMdiArea::closeAll()
{
    sNoCloseMessage = true;
    body_->closeAllSubWindows();
    windowActivated.trigger();
    sNoCloseMessage = false;
}


void uiMdiArea::setActiveWin( uiMdiAreaWindow* grp )
{
    if ( !grp ) return;
    body_->setActiveSubWindow( grp->qWidget() );
}


void uiMdiArea::setActiveWin( const char* nm )
{ setActiveWin( getWindow(nm) ); }


const char* uiMdiArea::getActiveWin() const
{
    static BufferString nm;
    mQtclass(QWidget*) widget = body_->activeSubWindow();
    nm = widget ? mQStringToConstChar(widget->windowTitle()) : "";
    return nm.buf();
}


void uiMdiArea::getWindowNames( BufferStringSet& nms ) const
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	nms.add( grps_[idx]->getTitle() );
}


bool uiMdiArea::paralyse( bool yn )
{
    const bool oldstate = !sensitive();
    if ( !yn ) mQtclass(QApplication)::processEvents();
    setSensitive( !yn );
    return oldstate;
}


class ODMdiSubWindow : public mQtclass(QMdiSubWindow)
{
public:
ODMdiSubWindow( mQtclass(QWidget*) par=0, mQtclass(Qt)::WindowFlags flgs=0 )
    : mQtclass(QMdiSubWindow)( par, flgs )
{}

protected:
void closeEvent( mQtclass(QCloseEvent*) ev )
{
    const BufferString msg( "Do you want to close ",\
			    mQStringToConstChar(windowTitle()), "?" );
    if ( sNoCloseMessage || uiMSG().askGoOn(msg) )
    {
	ev->accept();
	mQtclass(QMdiSubWindow)::closeEvent( ev );
    }
    else
	ev->ignore();
}

};

// uiMdiAreaWindow
uiMdiAreaWindow::uiMdiAreaWindow( const char* nm )
    : uiGroup(0,nm)
    , changed(this)
{
    qmdisubwindow_ = new ODMdiSubWindow();
    qmdisubwindow_->setWidget( attachObj()->body()->qwidget() );
    setTitle( nm );
}


void uiMdiAreaWindow::setTitle( const char* nm )
{
    qmdisubwindow_->setWindowTitle( nm );
    changed.trigger();
}


const char* uiMdiAreaWindow::getTitle() const
{
    static BufferString title;
    title = mQStringToConstChar( qmdisubwindow_->windowTitle() );
    return title.buf();
}


void uiMdiAreaWindow::setIcon( const char* img[] )
{
    if ( !img ) return;
    qmdisubwindow_->setWindowIcon( mQtclass(QIcon)(img) );
}


NotifierAccess& uiMdiAreaWindow::closed()
{ return mainObject()->closed; }

mQtclass(QMdiSubWindow*) uiMdiAreaWindow::qWidget()
{ return qmdisubwindow_; }

void uiMdiAreaWindow::show()		{ qmdisubwindow_->showNormal(); }
void uiMdiAreaWindow::close()		{ qmdisubwindow_->close(); }
void uiMdiAreaWindow::showMinimized()	{ qmdisubwindow_->showMinimized(); }
void uiMdiAreaWindow::showMaximized()	{ qmdisubwindow_->showMaximized(); }

bool uiMdiAreaWindow::isMinimized() const
{ return qmdisubwindow_->isMinimized(); }

bool uiMdiAreaWindow::isMaximized() const
{ return qmdisubwindow_->isMaximized(); }

