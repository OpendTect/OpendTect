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

#include <QDockWidget>

mUseQtnamespace

class uiDockWinBody : public uiParentBody, public QDockWidget
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const char* nm );

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
			      const char* nm )
    : uiParentBody( nm )
    , QDockWidget( nm )
    , handle_( uidw )
    , initing( true )
    , centralWidget_( 0 )

{
    QDockWidget::setFeatures( QDockWidget::DockWidgetMovable | 
	    		      QDockWidget::DockWidgetFloatable );
    setObjectName( nm );
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
    delete centralWidget_; centralWidget_ = 0;
}


void uiDockWinBody::finalise()
{
    handle_.preFinalise().trigger( handle_ );
    centralWidget_->finalise();
    finaliseChildren();
    handle_.postFinalise().trigger( handle_ );
}


// ----- uiDockWin -----
uiDockWin::uiDockWin( uiParent* parnt, const char* nm )
    : uiParent(nm,0)
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


const char* uiDockWin::getDockName() const
{
    static BufferString docknm;
    docknm = mQStringToConstChar( body_->qwidget()->objectName() );
    return docknm;
}

void uiDockWin::setDockName( const char* nm )
{ body_->qwidget()->setObjectName( nm ); }

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
