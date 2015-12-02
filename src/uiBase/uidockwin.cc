/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          13/02/2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uidockwin.h"
#include "uigroup.h"
#include "uimainwin.h"
#include "uiparentbody.h"

#include "perthreadrepos.h"
#include <QDockWidget>

mUseQtnamespace

class uiDockWinBody : public uiParentBody, public QDockWidget
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const uiString& caption);

    virtual		~uiDockWinBody();
    void		construct();

#define mHANDLE_OBJ     uiDockWin
#define mQWIDGET_BASE   QDockWidget
#define mQWIDGET_BODY   QDockWidget
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

protected:

    virtual void	finalise();
};



uiDockWinBody::uiDockWinBody( uiDockWin& uidw, uiParent* parnt,
			      const uiString& nm )
    : uiParentBody( nm.getFullString() )
    , QDockWidget( nm.getQString() )
    , handle_( uidw )
    , initing_( true )
    , centralwidget_( 0 )

{
    QDockWidget::setFeatures( QDockWidget::DockWidgetMovable |
			      QDockWidget::DockWidgetFloatable );
    setObjectName( nm.getQString() );
}


void uiDockWinBody::construct()
{
    centralwidget_ = new uiGroup( &handle(), "uiDockWin central widget" );
    setWidget( centralwidget_->body()->qwidget() );

    centralwidget_->setIsMain(true);
    centralwidget_->setBorder(0);
    centralwidget_->setStretch(2,2);

    initing_ = false;
}


uiDockWinBody::~uiDockWinBody( )
{
    delete centralwidget_; centralwidget_ = 0;
}


void uiDockWinBody::finalise()
{
    handle_.preFinalise().trigger( handle_ );
    centralwidget_->finalise();
    finaliseChildren();
    handle_.postFinalise().trigger( handle_ );
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


uiString uiDockWin::getDockName() const
{
    uiString res;
    res.setFrom(  body_->qwidget()->objectName() );
    return res;
}

void uiDockWin::setDockName( const uiString& nm )
{ body_->qwidget()->setObjectName( nm.getQString() ); }

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
