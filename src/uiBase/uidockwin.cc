/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.cc,v 1.8 2002-11-01 12:29:32 arend Exp $
________________________________________________________________________

-*/

#include "uidockwin.h"
#include "uigroup.h"
#include "uiparentbody.h"


#include <qdockwindow.h>

class uiDockWinBody : public uiParentBody, public UserIDObject
		    , public QDockWindow
{
public:
			uiDockWinBody( uiDockWin& handle, uiParent* parnt,
				       const char* nm );

    void		construct();

    virtual		~uiDockWinBody();

#define mHANDLE_OBJ     uiDockWin
#define mQWIDGET_BASE   QDockWindow
#define mQWIDGET_BODY   QDockWindow
#define UIBASEBODY_ONLY
#include                "i_uiobjqtbody.h"

public:

    uiGroup*		uiCentralWidg()		{ return centralWidget_; }


    virtual void        addChild( uiObjHandle& child )
			{ 
			    if ( !initing && centralWidget_ ) 
				centralWidget_->addChild( child );
			    else
				uiParentBody::addChild( child );
			}

    virtual void        manageChld_( uiObjHandle& o, uiObjectBody& b )
			{ 
			    if ( !initing && centralWidget_ ) 
				centralWidget_->manageChld( o, b );

			}

    virtual void  	attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin,
					      bool reciprocal )
                        {
                            if ( !child || initing ) return;

			    centralWidget_->attachChild( tp, child, other,
							margin, reciprocal); 
                        }
protected:

    virtual void	finalise();

    bool		initing;

    uiGroup*		centralWidget_;

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return centralWidget_->pbody()->managewidg();
			    return qwidget_();
			}
};



uiDockWinBody::uiDockWinBody( uiDockWin& handle__, uiParent* parnt, 
			      const char* nm )
	: uiParentBody()
	, UserIDObject( nm )
	, QDockWindow( InDock, parnt && parnt->pbody() ?  
					parnt->pbody()->qwidget() : 0, nm ) 
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
{
    if ( *nm ) setCaption( nm );
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


uiObject* uiDockWin::uiObj()
{ 
    return body_->uiCentralWidg()->uiObj(); 
}


const uiObject* uiDockWin::uiObj() const
{ 
    return body_->uiCentralWidg()->uiObj(); 
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


void uiDockWin::display( bool yn, bool shrink )
{ 
    finalise();
    uiObj()->display(yn,shrink);
}



QDockWindow* uiDockWin::qwidget()
    { return body_; }
