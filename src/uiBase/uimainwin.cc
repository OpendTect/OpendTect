/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.cc,v 1.11 2001-09-27 16:17:04 arend Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "qmainwindow.h"
#include "qwidget.h"

#include "uistatusbar.h"
#include "qstatusbar.h"
#include "uimenu.h"
#include "uigroup.h"
#include "uimsg.h"
#include "uiobjbody.h"
#include "uibody.h"
#include "uiparentbody.h"

#include "msgh.h"
#include "errh.h"

#include <iostream>

#ifdef __msvc__
#include <qpopupmenu.h>
#endif


#ifdef __debug__
#include <qmenubar.h>
#endif

//class uiMainWinBody : public uiBodyIsaQthingImpl<uiMainWin,QMainWindow>
//		    , public uiParentBody
class uiMainWinBody : public uiParentBody, public UserIDObject
		    , public QMainWindow
{
public:
			uiMainWinBody( uiMainWin& handle, uiParent* parnt=0,
				   const char* nm="uiMainWinBody" );


    void		construct( bool wantStatusBar, bool wantMenuBar);
    virtual		~uiMainWinBody();

#define mHANDLE_OBJ     uiMainWin
#define mQWIDGET_BASE   QMainWindow
#define mQWIDGET_BODY   QMainWindow
#define UIBASEBODY_ONLY
#include                "i_uiobjqtbody.h"

public:


    uiStatusBar* 	uistatusbar();
    uiMenuBar* 		uimenubar();

    virtual void        polish()
                        {
			    MsgClass::theCB = mCB(&uiMSG(),uiMsg,handleMsg);
                            QMainWindow::polish();
                        }

    uiGroup*		uiCentralWidg()		{ return centralWidget_; }

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

    void		reDraw( bool deep )
			{
			    update();
			    centralWidget_->reDraw(deep);
			}

    virtual int		minTextWidgetHeight() const
			{ return centralWidget_->minTextWidgetHeight(); }


//protected:

    virtual void	finalise();

    bool		initing;

    uiGroup*		centralWidget_;

    uiStatusBar* 	mStatusBar;
    uiMenuBar* 		mMenuBar;

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return centralWidget_->body()->managewidg();
			    return qwidget_();
			}
};



uiMainWinBody::uiMainWinBody( uiMainWin& handle__, uiParent* parnt, 
			      const char* nm )
	//: uiBodyIsaQthingImpl<uiMainWin,QMainWindow> ( handle__, parnt )
	: uiParentBody()
	, UserIDObject( nm )
	, QMainWindow( parnt && parnt->body() ? 
			    parnt->body()->qwidget() : 0, nm )
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
	, mStatusBar( 0 ) , mMenuBar(0)  {}

void uiMainWinBody::construct(  bool wantStatusBar, bool wantMenuBar)
{ 
    centralWidget_ = new uiGroup( &handle(), "uiMainWin central widget" );
    setCentralWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);

    if( wantStatusBar )
    {
	QStatusBar* mbar= statusBar();
	if( mbar )
	    mStatusBar = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    pErrMsg("No statusbar returned from Qt");
    }
    if( wantMenuBar )
    {   
	QMenuBar* myBar =  menuBar();

	if( myBar )
	    mMenuBar = new uiMenuBar( &handle(), "MainWindow MenuBar handle", 
				      *myBar);
	else
	    pErrMsg("No menubar returned from Qt");
    }
    initing = false;
}


uiMainWinBody::~uiMainWinBody( )
{
    delete centralWidget_;
}


void uiMainWinBody::finalise()
    { centralWidget_->finalise();  finaliseChildren(); }



uiStatusBar* uiMainWinBody::uistatusbar()
{
    if ( !mStatusBar) pErrMsg("No statusbar. See uiMainWinBody's constructor"); 
    return mStatusBar;
}


uiMenuBar* uiMainWinBody::uimenubar()
{
    if ( !mMenuBar ) pErrMsg("No menuBar. See uiMainWinBody's constructor"); 
    return mMenuBar;
}



uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      bool wantSBar, bool wantMBar )
    : uiParent( nm, 0 )
    , body_( 0 )
{ 
    body_= new uiMainWinBody( *this, parnt, nm ); 
    setBody( body_ );
    body_->construct(wantSBar,wantMBar);
    body_->uiCentralWidg()->setBorder(10);
}

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }
void uiMainWin::show()				{ body_->uiShow(); }
void uiMainWin::setCaption( const char* txt )	{ body_->setCaption(txt); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }

uiObject* uiMainWin::uiObj()
    { return body_->uiCentralWidg()->uiObj(); }

const uiObject* uiMainWin::uiObj() const
    { return body_->uiCentralWidg()->uiObj(); }

