/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidockwin.h"

#include "uigroup.h"
#include "uimainwin.h"
#include "uiparentbody.h"

#include "perthreadrepos.h"

#include "q_uiimpl.h"

#include <QDockWidget>

mUseQtnamespace

class uiDockWinBody : public uiCentralWidgetBody, public QDockWidget
{
public:
			uiDockWinBody(uiDockWin&,uiParent*,
				      const uiString& caption);
    virtual		~uiDockWinBody();

    void		construct();

protected:
    const QWidget*	qwidget_() const override { return this; }
    void		finalize() override;

    uiDockWin&		handle_;
};



uiDockWinBody::uiDockWinBody( uiDockWin& uidw, uiParent* parnt,
			      const uiString& nm )
    : uiCentralWidgetBody(nm.getFullString())
    , QDockWidget(toQString(nm))
    , handle_(uidw)
{
    QDockWidget::setFeatures( QDockWidget::DockWidgetMovable |
			      QDockWidget::DockWidgetFloatable );
    setObjectName( toQString(nm) );
}


void uiDockWinBody::construct()
{
    centralwidget_ = new uiGroup( &handle_, "uiDockWin central widget" );
    setWidget( centralwidget_->body()->qwidget() );

    centralwidget_->setIsMain(true);
    centralwidget_->setBorder(0);
    centralwidget_->setStretch(2,2);

    initing_ = false;
}


uiDockWinBody::~uiDockWinBody( )
{
    delete centralwidget_;
}


void uiDockWinBody::finalize()
{
    handle_.preFinalize().trigger( handle_ );
    centralwidget_->finalize();
    finalizeChildren();
    handle_.postFinalize().trigger( handle_ );
}


// ----- uiDockWin -----
uiDockWin::uiDockWin( uiParent* parnt, const uiString& nm )
    : uiParent(nm.getFullString(),0)
    , body_(0)
    , parent_(parnt)
{
    body_= new uiDockWinBody( *this, parnt, nm );
    setBody( body_ );
    body_->construct();

    if ( parnt ) parnt->addChild( *this );
}


uiDockWin::~uiDockWin()
{ delete body_; }


void uiDockWin::setObject( uiObject* obj )
{
    if ( !obj ) return;
    body_->setWidget( obj->body()->qwidget() );
}


void uiDockWin::setGroup( uiGroup* grp )
{
    if ( !grp ) return;
    setObject( grp->attachObj() );
}


void uiDockWin::setVisible( bool yn )
{
    if ( !finalized() )
	display( yn );

    body_->setVisible( yn );
}


bool uiDockWin::isVisible() const
{
    return body_->isVisible();
}


uiString uiDockWin::getDockName() const
{
    uiString res;
    res.setFrom(  body_->qwidget()->objectName() );
    return res;
}

void uiDockWin::setDockName( const uiString& nm )
{ body_->qwidget()->setObjectName( toQString(nm) ); }

uiGroup* uiDockWin::topGroup()
{ return body_->uiCentralWidg(); }


uiMainWin* uiDockWin::mainwin()
{
    mDynamicCastGet(uiMainWin*,uimw,parent_);
    return uimw;
}


uiObject* uiDockWin::mainobject()
{ return body_->uiCentralWidg()->mainObject(); }

void uiDockWin::setFloating( bool yn )
{ body_->setFloating( yn ); }

bool uiDockWin::isFloating() const
{ return body_->isFloating(); }

QDockWidget* uiDockWin::qwidget()
{ return body_; }

void uiDockWin::setMinimumWidth( int width )
{ if ( body_ ) body_->setMinimumWidth( width ); }
