/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          13/02/2002
 RCS:           $Id: uidockwin.cc,v 1.1 2002-02-13 16:33:37 arend Exp $
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
                                              uiObject* other, int margin )
                        {
                            if ( !child || initing ) return;

			    centralWidget_->attachChild( tp, child, other,
							margin); 
                        }
protected:

    virtual void	finalise();

    bool		initing;

    uiGroup*		centralWidget_;

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return centralWidget_->body()->managewidg();
			    return qwidget_();
			}
};



uiDockWinBody::uiDockWinBody( uiDockWin& handle__, uiParent* parnt, 
			      const char* nm )
	: uiParentBody()
	, UserIDObject( nm )
	, QDockWindow( InDock, parnt && parnt->body() ?  
					parnt->body()->qwidget() : 0, nm ) 
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
    delete centralWidget_;
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
{ delete body_; }


uiGroup* uiDockWin::topGroup()	    	   { return body_->uiCentralWidg(); }


uiObject* uiDockWin::uiObj()
    { return body_->uiCentralWidg()->uiObj(); }

const uiObject* uiDockWin::uiObj() const
    { return body_->uiCentralWidg()->uiObj(); }

