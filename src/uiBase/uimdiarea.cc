/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          August 2008
________________________________________________________________________

-*/

#include "uimdiarea.h"
#include "i_qmdiarea.h"

#include "uiicon.h"
#include "uimsg.h"
#include "uiobjbodyimpl.h"
#include "bufstringset.h"
#include "keystrs.h"
#include "perthreadrepos.h"

#include "q_uiimpl.h"

#include <QApplication>
#include <QCloseEvent>
#include <QIcon>
#include <QMdiSubWindow>

mUseQtnamespace

static bool sNoCloseMessage = false;

class uiMdiAreaBody : public uiObjBodyImpl<uiMdiArea,QMdiArea>
{
public:
			uiMdiAreaBody(uiMdiArea&,uiParent*,const char*);
			~uiMdiAreaBody();

protected:
    i_MdiAreaMessenger& messenger_;

    void		resizeEvent(QResizeEvent*) override;
};


uiMdiAreaBody::uiMdiAreaBody( uiMdiArea& hndle, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiMdiArea,QMdiArea>(hndle,p,nm)
    , messenger_(*new i_MdiAreaMessenger(this,&hndle))
{}

uiMdiAreaBody::~uiMdiAreaBody()
{ delete &messenger_; }


void uiMdiAreaBody::resizeEvent( QResizeEvent* ev )
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
{
    detachAllNotifiers();
    delete body_;
}


void uiMdiArea::cascade()
{ body_->cascadeSubWindows(); }


void uiMdiArea::tile()
{
    body_->tileSubWindows();

    // Fix case where QMdiArea::tileSubWindows() does not obey WindowOrder
    QList<QMdiSubWindow*> windows = body_->subWindowList();
    for ( int idx=0; idx<windows.count()-1; idx++ )
    {
	QMdiSubWindow* widget0 = windows.at( idx );
	for ( int idy=idx+1; idy<windows.count(); idy++ )
	{
	    QMdiSubWindow* widget1 = windows.at( idy );
	    const QRect rect0 = widget0->geometry();
	    const QRect rect1 = widget1->geometry();
	    if ( rect0.top()>rect1.top() ||
		 (rect0.top()==rect1.top() && rect0.left()>rect1.left()) )
	    {
		widget0->setGeometry( rect1 );
		widget1->setGeometry( rect0 );
	    }
	}
    }
}


void uiMdiArea::tileVertical()
{
    body_->tileSubWindows();
    QList<QMdiSubWindow*> windows = body_->subWindowList();

    int nrvisiblewindows = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	QMdiSubWindow* widget = windows.at( idx );
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
	QMdiSubWindow* widget = windows.at( idx );
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
    body_->tileSubWindows();
    QList<QMdiSubWindow*> windows = body_->subWindowList();

    int nrvisiblewindows = 0;
    for ( int idx=0; idx<windows.count(); idx++ )
    {
	QMdiSubWindow* widget = windows.at( idx );
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
	QMdiSubWindow* widget = windows.at( idx );
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
    mAttachCB( grp->closed(), uiMdiArea::grpClosed );
    mAttachCB( grp->changed, uiMdiArea::grpChanged );
    grps_ += grp;
    body_->setActiveSubWindow( grp->qWidget() );
    windowActivated.trigger();
}


uiMdiAreaWindow* uiMdiArea::getWindow( const char* nm )
{
    const BufferString nmbufstr( nm );
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( !strcmp(nmbufstr.buf(),grps_[idx]->getTitle().getFullString()) )
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
	grp->qWidget()->deleteLater();
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
    mDeclStaticString( nm );
    QWidget* widget = body_->activeSubWindow();

    nm = sKey::EmptyString();

    if ( widget )
    {
	for ( int idx=0; idx<grps_.size(); idx++ )
	{
	    if ( grps_[idx]->qWidget()==widget )
		nm = grps_[idx]->getTitle().getFullString();
	}
    }

    return nm.buf();
}


void uiMdiArea::getWindowNames( uiStringSet& nms ) const
{
    for ( int idx=0; idx<grps_.size(); idx++ )
	nms += grps_[idx]->getTitle();
}


bool uiMdiArea::paralyse( bool yn )
{
    const bool oldstate = !sensitive();
    if ( !yn ) QApplication::processEvents();
    setSensitive( !yn );
    return oldstate;
}


class ODMdiSubWindow : public QMdiSubWindow, public CallBacker
{
public:
ODMdiSubWindow( uiMdiAreaWindow& mdiareawin,
		QWidget* par=nullptr, Qt::WindowFlags flgs=Qt::WindowFlags() )
    : QMdiSubWindow( par, flgs )
    , mdiareawin_( mdiareawin )
    , windowShown(this)
    , windowHidden(this)
{
}

Notifier<ODMdiSubWindow>	windowShown;
Notifier<ODMdiSubWindow>	windowHidden;

protected:
void closeEvent( QCloseEvent* ev ) override
{
    const int closedsubwinidx =
	mdiArea()->subWindowList(QMdiArea::CreationOrder).indexOf( this );

    uiMdiArea& mdiarea = mdiareawin_.getMdiArea();
    BufferString cmdrecmsg( "Close " ); cmdrecmsg += closedsubwinidx;
    const int refnr = mdiarea.beginCmdRecEvent( cmdrecmsg );
    BufferString bfstr = windowTitle();
    uiString msg =
	od_static_tr("closeEvent","Do you want to close %1 ?").arg(bfstr);
    if ( sNoCloseMessage || uiMSG().askGoOn(msg) )
    {
	ev->accept();
	QMdiSubWindow::closeEvent( ev );
    }
    else
	ev->ignore();

    mdiarea.endCmdRecEvent( refnr, cmdrecmsg );
}


void showEvent( QShowEvent* ev ) override
{
    if ( !isMinimized() )
	windowShown.trigger();
}


void hideEvent( QHideEvent* ) override
{
    windowHidden.trigger();
}

    uiMdiAreaWindow&	mdiareawin_;
};


// uiMdiAreaWindow
uiMdiAreaWindow::uiMdiAreaWindow( uiMdiArea& mdiarea, const uiString& title )
    : uiGroup(0,title.getFullString())
    , mdiarea_(mdiarea)
    , changed(this)
{
    qmdisubwindow_ = new ODMdiSubWindow( *this );
    qmdisubwindow_->setWidget( attachObj()->body()->qwidget() );
    qmdisubwindow_->setAttribute( Qt::WA_DeleteOnClose );
    setTitle( title );
}


void uiMdiAreaWindow::setTitle( const uiString& title )
{
    qmdisubwindow_->setWindowTitle( toQString(title) );
    title_ = title;
    changed.trigger();
}


void uiMdiAreaWindow::setIcon( const char* img[] )
{
    if ( !img ) return;
    const QPixmap pixmap( img );
    qmdisubwindow_->setWindowIcon( QIcon(pixmap) );
}


void uiMdiAreaWindow::setIcon( const char* icnnm )
{
    const uiIcon icon( icnnm );
    qmdisubwindow_->setWindowIcon( icon.qicon() );
}


NotifierAccess& uiMdiAreaWindow::closed()
{ return mainObject()->closed; }


NotifierAccess& uiMdiAreaWindow::windowShown()
{
    mDynamicCastGet(ODMdiSubWindow*, odmdia, qmdisubwindow_);
    return odmdia->windowShown;
}


NotifierAccess& uiMdiAreaWindow::windowHidden()
{
    mDynamicCastGet(ODMdiSubWindow*, odmdia, qmdisubwindow_);
    return odmdia->windowHidden;
}


QMdiSubWindow* uiMdiAreaWindow::qWidget()
{ return qmdisubwindow_; }

const QMdiSubWindow* uiMdiAreaWindow::qWidget() const
{ return qmdisubwindow_; }



void uiMdiAreaWindow::show()		{ qmdisubwindow_->showNormal(); }
void uiMdiAreaWindow::close()		{ qmdisubwindow_->close(); }
void uiMdiAreaWindow::showMinimized()	{ qmdisubwindow_->showMinimized(); }
void uiMdiAreaWindow::showMaximized()	{ qmdisubwindow_->showMaximized(); }

bool uiMdiAreaWindow::isMinimized() const
{ return qmdisubwindow_->isMinimized(); }

bool uiMdiAreaWindow::isMaximized() const
{ return qmdisubwindow_->isMaximized(); }
