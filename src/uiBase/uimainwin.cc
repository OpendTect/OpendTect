/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.cc,v 1.85 2004-08-25 11:32:14 nanne Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uidialog.h"

#include "msgh.h"
#include "errh.h"
#include "helpview.h"
#include "uimsg.h"

#include "pixmap.h"
#include "uibody.h"
#include "uiobjbody.h"
#include "uiparentbody.h"

#include "uigroup.h"
#include "uibutton.h"
#include "uistatusbar.h"
#include "uitoolbar.h"
#include "uiseparator.h"
#include "uimenu.h"
#include "uilabel.h"
#include "uidockwin.h"

#include "timer.h"


#include <iostream>

#include <qmainwindow.h>
#include <qwidget.h>
#include <qstatusbar.h>
#include <qapplication.h>
#include <qdockwindow.h>
#include <qpixmap.h>

#ifdef SUPPORT_PERSISTENCE
// for positions of dock area's
#include <qdockarea.h>
#include <qfile.h>
#endif

#ifdef __msvc__
#include <qpopupmenu.h>
#endif


#ifdef __debug__
#include <qmenubar.h>
#endif

#include <qlayout.h>

#include "dtect.xpm"


class uiMainWinBody : public uiParentBody
		    , public QMainWindow
{
friend class		uiMainWin;
public:
			uiMainWinBody( uiMainWin& handle, uiParent* parnt,
				       const char* nm, bool modal );

    void		construct( int nrStatusFlds, bool wantMenuBar, 
				   bool wantToolBar );

    virtual		~uiMainWinBody();

#define mHANDLE_OBJ     uiMainWin
#define mQWIDGET_BASE   QMainWindow
#define mQWIDGET_BODY   QMainWindow
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

public:


    uiStatusBar* 	uistatusbar();
    uiMenuBar* 		uimenubar();
    uiToolBar*		uitoolbar();

    virtual void        polish()
                        {
			    CallBack msghcb = mCB(&uiMSG(),uiMsg,handleMsg);
			    MsgClass::theCB( &msghcb );
                            QMainWindow::polish();
                        }

    void		reDraw( bool deep )
			{
			    update();
			    centralWidget_->reDraw(deep);
			}

    void		go()			
			{ 
			    finalise(); 
			    show(); 
			}

    virtual void	show() 
			{
			    QMainWindow::show();

			    if( poptimer.isActive() )
				poptimer.stop();

			    popped_up = false;
			    poptimer.start( 100, true );

			    if( modal_ )	
				looplevel__ = qApp->enter_loop();
			    else 
				looplevel__ = -1;
			}

    void		close();

    bool		poppedUp() const { return popped_up; }
    bool		touch()
			{
			    if ( popped_up || !finalised() ) return false;

			    if( poptimer.isActive() )
				poptimer.stop();

			    if ( !popped_up )
				poptimer.start( 100, true );

			    return true;
			}

    static Qt::Dock	qdock(uiMainWin::Dock);

    void 		uimoveDockWindow(uiDockWin&,uiMainWin::Dock,int);

#ifdef SUPPORT_PERSISTENCE
    void		storePositions();
    void		restorePositions();
#endif

protected:

    virtual void	finalise( bool trigger_finalise_start_stop=true );
    void		closeEvent(QCloseEvent*);

    bool		exitapponclose_;

    uiStatusBar* 	statusbar;
    uiMenuBar* 		menubar;
    uiToolBar* 		toolbar;

private:

    bool		modal_;
    int			looplevel__;


    void 		popTimTick(CallBacker*);
    Timer		poptimer;
    bool		popped_up;


    ObjectSet<uiDockWin>	wins2move;
    TypeSet<uiMainWin::Dock>	docks4wins;
    void			moveDockWindows();
};



uiMainWinBody::uiMainWinBody( uiMainWin& handle__, uiParent* parnt, 
			      const char* nm, bool modal )
	: uiParentBody( nm )
//	, UserIDObject( nm )
	, QMainWindow( parnt && parnt->pbody() ? parnt->pbody()->qwidget() : 0, 
		       nm, 
		       (parnt && modal) ? 
				WType_TopLevel | WShowModal| WGroupLeader :
				WType_TopLevel )
	, handle_( handle__ )
	, initing( true )
	, centralWidget_( 0 )
	, statusbar(0), menubar(0), toolbar(0)  
	, modal_( parnt && modal )
	, poptimer("Popup timer")
	, popped_up( false )
	, exitapponclose_( false )
{
    if ( nm && *nm ) setCaption( nm );
    poptimer.tick.notify(mCB(this,uiMainWinBody,popTimTick));
}

void uiMainWinBody::construct(  int nrStatusFlds, bool wantMenuBar, 
				bool wantToolBar )
{
    centralWidget_ = new uiGroup( &handle(), "uiMainWin central widget" );
    setCentralWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);
    centralWidget_->setBorder(10);
    centralWidget_->setStretch(2,2);

    if( nrStatusFlds != 0 )
    {
	QStatusBar* mbar= statusBar();
	if( mbar )
	    statusbar = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    pErrMsg("No statusbar returned from Qt");

	if( nrStatusFlds > 0 )
	{
	    for( int idx=0; idx<nrStatusFlds; idx++ )
		statusbar->addMsgFld();
	}
    }
    if( wantMenuBar )
    {   
	QMenuBar* myBar =  menuBar();

	if( myBar )
	    menubar = new uiMenuBar( &handle(), "MainWindow MenuBar handle", 
				      *myBar);
	else
	    pErrMsg("No menubar returned from Qt");
    }
    if( wantToolBar )
    {
	toolbar = uiToolBar::getNew( *this );
    }

    initing = false;
}


uiMainWinBody::~uiMainWinBody( )
{
//    delete centralWidget_;
}

void uiMainWinBody::popTimTick(CallBacker*)
{
    if ( popped_up ) { pErrMsg( "huh?" ); }
	popped_up = true;
    moveDockWindows();
//    restorePositions();
}

#define mMwHandle static_cast<uiMainWin&>(handle_)

void uiMainWinBody::finalise( bool trigger_finalise_start_stop )
{
    if( trigger_finalise_start_stop )
	mMwHandle.finaliseStart.trigger(mMwHandle);

    centralWidget_->finalise();
    finaliseChildren();

    if( trigger_finalise_start_stop )
	mMwHandle.finaliseDone.trigger(mMwHandle);
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    mMwHandle.closeOK() ? ce->accept() : ce->ignore();
}


#define mHide() \
    if ( !mMwHandle.closeOK() ) return; \
    mMwHandle.windowClosed.trigger(mMwHandle); \
    if( modal_ )	qApp->exit_loop(); \
    QMainWindow::hide();


void uiMainWinBody::close()
{
    mHide();
    if ( exitapponclose_ )	qApp->quit();
}


uiStatusBar* uiMainWinBody::uistatusbar()
{
    return statusbar;
}


uiMenuBar* uiMainWinBody::uimenubar()
{
    if ( !menubar ) pErrMsg("No menuBar. See uiMainWinBody's constructor"); 
    return menubar;
}

uiToolBar* uiMainWinBody::uitoolbar()
{
    if ( !toolbar ) pErrMsg("No toolBar. See uiMainWinBody's constructor"); 
    return toolbar;
}

void uiMainWinBody::uimoveDockWindow( uiDockWin& dwin, uiMainWin::Dock dock,
				      int index )
{
    if ( index < 0 )
    {
	wins2move += &dwin;
	docks4wins += dock;
    }
    else
    {
	wins2move.insertAt( &dwin, index );
	docks4wins.insert( index, dock );
    }

    moveDockWindows();
}

void uiMainWinBody::moveDockWindows()
{
    if ( !poppedUp() ) return;

    for( int idx=0; idx<wins2move.size(); idx++ )
	moveDockWindow( wins2move[idx]->qwidget(), qdock(docks4wins[idx]) );

    wins2move.erase();
    docks4wins.erase();
}

#ifdef SUPPORT_PERSISTENCE
void uiMainWinBody::storePositions()
{
#ifdef USE_FILE
    QFile outfil( "/tmp/qpositions.txt");
    outfil.open( IO_WriteOnly );
    QTextStream ts( &outfil );
#else
    static QString str;
    str="";
    QTextStream ts( &str, IO_WriteOnly  );
#endif

    QDockArea* dck = leftDock();
    if ( dck ) { ts << *dck; }

    dck = rightDock();
    if ( dck ) { ts << *dck; }

    dck = topDock();
    if ( dck ) { ts << *dck; }

    dck = bottomDock();
    if ( dck ) { ts << *dck; }

#ifdef USE_FILE
    outfil.close();
#else
    cout << str;
#endif
}

void uiMainWinBody::restorePositions()
{
storePositions();

    QFile infil( "/tmp/qpositions.txt");
    infil.open( IO_ReadOnly );
    QTextStream ts( &infil );

    QDockArea* dck = leftDock();
    if ( dck ) { ts >> *dck; }

    dck = rightDock();
    if ( dck ) { ts >> *dck; }

    dck = topDock();
    if ( dck ) { ts >> *dck; }

    dck = bottomDock();
    if ( dck ) { ts >> *dck; }

    infil.close();
storePositions();
}
#endif

Qt::Dock uiMainWinBody::qdock( uiMainWin::Dock d )
{
    switch( d )
    {
        case uiMainWin::Top:            return Qt::DockTop;
        case uiMainWin::Bottom:         return Qt::DockBottom;
        case uiMainWin::Right:          return Qt::DockRight;
        case uiMainWin::Left:           return Qt::DockLeft;
        case uiMainWin::Minimized:      return Qt::DockMinimized;
        case uiMainWin::TornOff:  	return Qt::DockTornOff;
        case uiMainWin::Unmanaged:      return Qt::DockUnmanaged;
    }
    return (Qt::Dock) 0;
}


uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		  int nrstatusflds, bool wantMBar, bool wantTBar, bool modal )
    : uiParent( nm, 0 )
    , body_( 0 )
    , finaliseStart(this)
    , finaliseDone(this)
    , windowClosed(this)
{ 
    body_= new uiMainWinBody( *this, parnt, nm, modal ); 
    setBody( body_ );
    body_->construct( nrstatusflds, wantMBar, wantTBar );
    if ( !parnt )
	setIcon( dtect_xpm_data, "OpendTect" );
}


uiMainWin::uiMainWin( const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )			
    , finaliseStart(this)
    , finaliseDone(this)
    , windowClosed(this)
{}


uiMainWin::~uiMainWin()
{ delete body_; }


void uiMainWin::provideHelp( const char* winid )
{
    HelpViewer::use( HelpViewer::getURLForWinID(winid) );
}

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }
uiToolBar* uiMainWin::toolBar()			{ return body_->uitoolbar(); }
uiToolBar* uiMainWin::newToolBar( const char* nm )
    { return uiToolBar::getNew( *body_, nm ); }

void uiMainWin::show()				{ body_->go(); }
void uiMainWin::close()				{ body_->close(); }
void uiMainWin::setCaption( const char* txt )	{ body_->setCaption(txt); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::touch() 			{ return body_->touch(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }

void uiMainWin::moveDockWindow( uiDockWin& dwin, Dock d, int index )
    { body_->uimoveDockWindow(dwin,d,index); }

void uiMainWin::removeDockWindow( uiDockWin* dwin )
{
    if ( !dwin ) return;
    body_->removeDockWindow( dwin->qwidget() );
}


uiGroup* uiMainWin::topGroup()	    	   { return body_->uiCentralWidg(); }

void uiMainWin::setShrinkAllowed(bool yn)  
    { if( topGroup() ) topGroup()->setShrinkAllowed(yn); }

bool uiMainWin::shrinkAllowed()	 	   
    { return topGroup() ? topGroup()->shrinkAllowed() : false; }


uiObject* uiMainWin::mainobject()
    { return body_->uiCentralWidg()->mainObject(); }

void uiMainWin::toStatusBar( const char* txt, int fldidx, int msecs )
{
    if ( !txt ) txt = "";
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt, fldidx, msecs );
    else if ( *txt )
    	UsrMsg(txt);
}

uiMainWin* uiMainWin::activeWindow()
{
    QWidget* _aw = qApp->activeWindow();
    if( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if( !_awb )		return 0;

    return &_awb->handle();

}

uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if( !mwimpl )	return 0;

    uiMainWinBody* _mwb = dynamic_cast<uiMainWinBody*>( mwimpl );
    if( !_mwb )		return 0;

    return &_mwb->handle();
}


void uiMainWin::setIcon( const char* img[], const char* icntxt )
{
    if ( icntxt )
	body_->setIconText( icntxt );
    if ( img )
    {
	QPixmap imgpm( img );
	body_->setIcon( imgpm );
	if ( menuBar() )
	    menuBar()->setIcon( imgpm );
    }
}


/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/

#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public uiMainWinBody
{ 	
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);

    int			exec(); 

    void		reject( CallBacker* s )	
			{
			    mHandle.cancelpushed_ = s == cnclBut;
			    if ( mHandle.rejectOK(s) ) done_(0);
			}
                        //!< to be called by a 'cancel' button
    void		accept( CallBacker* s )	
			    { if ( mHandle.acceptOK(s) ) done_(1); }
                        //!< to be called by a 'ok' button
    void		done( int i )
			    { if ( mHandle.doneOK(i) ) done_(i); }

    void		uiSetResult( int v ) { reslt = v; }
    int			uiResult(){ return reslt; }

    void		setOkText( const char* txt );
			//!< OK button disabled when set to empty
    void		setCancelText( const char* txt );
			//!< cancel button disabled when set to empty
    void		enableSaveButton( const char* txt )
			    { 
				setup.savetext_ = txt; 
				setup.savebutton_ = uiDialog::Setup::CheckBox; 
			    }
    void		setSaveButtonChecked( bool yn )
			    { setup.savechecked_ = yn;
			      if ( saveBut_cb ) saveBut_cb->setChecked(yn); }
    void		setButtonSensitive( uiDialog::Button but, bool yn )
			    { 
				switch ( but )
				{
				case uiDialog::OK     :
				    if ( okBut ) okBut->setSensitive(yn); 
				break;
				case uiDialog::CANCEL :
				    if ( cnclBut ) cnclBut->setSensitive(yn); 
				break;
				case uiDialog::SAVE   : 
				    if ( saveBut_cb )
					saveBut_cb->setSensitive(yn); 
				    if ( saveBut_pb )
					saveBut_pb->setSensitive(yn); 
				break;
				case uiDialog::HELP   : 
				    if ( helpBut ) helpBut->setSensitive(yn); 
				break;
				}
			    }

    void		setTitleText( const char* txt );

    bool		saveButtonChecked() const;
    uiButton*		button( uiDialog::Button but ) 
			    { 
				switch ( but )
				{
				case uiDialog::OK     : return okBut;
				break;
				case uiDialog::CANCEL : return cnclBut;
				break;
				case uiDialog::SAVE   : 
				    return saveBut_cb
					? (uiButton*) saveBut_cb
					: (uiButton*) saveBut_pb;
				break;
				case uiDialog::HELP   : return helpBut;
				break;
				}
				return 0;
			    }

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )	{ setup.separator_ = yn; }
    bool		separator() const	{ return setup.separator_; }
    void		setHelpID( const char* id ) { setup.helpid_ = id; }
    const char*		helpID() const		{ return setup.helpid_; }

    void		setDlgGrp( uiGroup* cw )	{ dlgGroup=cw; }

    void		setHSpacing( int spc )	{ dlgGroup->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlgGroup->setVSpacing(spc); }
    void		setBorder( int b )	{ dlgGroup->setBorder( b ); }

    virtual void        addChild( uiObjHandle& child )
			{ 
			    if ( !initing ) 
				dlgGroup->addChild( child );
			    else
				uiMainWinBody::addChild( child );
			}

    virtual void        manageChld_( uiObjHandle& o, uiObjectBody& b )
			{ 
			    if ( !initing ) 
				dlgGroup->manageChld( o, b );
			}

    virtual void  	attachChild ( constraintType tp,
                                              uiObject* child,
                                              uiObject* other, int margin,
					      bool reciprocal )
                        {
                            if ( !child || initing ) return;
			    dlgGroup->attachChild( tp, child, other, margin,
						   reciprocal ); 
                        }
    void		provideHelp(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup; }

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return dlgGroup->pbody()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int 		reslt;
    bool		childrenInited;

    uiGroup*            dlgGroup;
    uiDialog::Setup	setup;

    uiPushButton*	okBut;
    uiPushButton*	cnclBut;
    uiToolButton*	helpBut;

    uiCheckBox*		saveBut_cb;
    uiPushButton*	saveBut_pb;

    uiSeparator*	horSepar;
    uiLabel*		title;

    void		done_(int);

    virtual void	finalise(bool);
    void		closeEvent(QCloseEvent*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);

};


uiDialogBody::uiDialogBody( uiDialog& handle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(handle,parnt,s.wintitle_,s.modal_)
    , dlgGroup( 0 )
    , setup( s )
    , okBut( 0 ), cnclBut( 0 ), saveBut_cb( 0 ),  saveBut_pb( 0 )
    , helpBut( 0 ), title( 0 ) , reslt( 0 )
    , childrenInited(false)
{
}

int uiDialogBody::exec()
{ 
    uiSetResult( 0 );

    if( setup.fixedsize_ )
    {
	setMaximumSize( QSize(0,0));
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );
    }

    go();

    return uiResult();
}



void uiDialogBody::setOkText( const char* txt )    
{ 
    setup.oktext_ = txt; 
    if( okBut ) okBut->setText(txt);
}


void uiDialogBody::setTitleText( const char* txt )    
{ 
    setup.dlgtitle_ = txt; 
    if( title ) 
    { 
	title->setText(txt); 
	uiObjectBody* tb = dynamic_cast<uiObjectBody*>( title->body() ); 
	if( tb && !tb->itemInited() )
	    title->setPrefWidthInChar( 
		    mMAX( tb->prefWidthInCharSet(), strlen(txt) + 2 )); 
    }
}

void uiDialogBody::setCancelText( const char* txt ) 
{ 
    setup.canceltext_ = txt; 
    if( cnclBut ) cnclBut->setText(txt);
}


bool uiDialogBody::saveButtonChecked() const
{ 
    return saveBut_cb ? saveBut_cb->isChecked() : false;
}


/*!
    Hides the box, which also exits the event loop in case of a modal box.
*/
void uiDialogBody::done_( int v )
{
    uiSetResult( v );
    close();
}


void uiDialogBody::closeEvent( QCloseEvent* ce )
{
    reject(0);
}


/*!
    Construct OK and Cancel buttons just before the first show.
    This gives chance not to construct them in case OKtext and CancelText have
    been set to ""
*/
void uiDialogBody::finalise(bool) 
{
    uiMainWinBody::finalise(false); 

    mMwHandle.finaliseStart.trigger(mMwHandle);

    dlgGroup->finalise();

    if ( !childrenInited ) 
	initChildren();

    finaliseChildren();

    mMwHandle.finaliseDone.trigger(mMwHandle);
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
    layoutChildren( lowestobject );

    if ( okBut )
    {
	okBut->activated.notify( mCB(this,uiDialogBody,accept) );
	okBut->setDefault();
    }
    if ( cnclBut )
    {
	cnclBut->activated.notify( mCB(this,uiDialogBody,reject) );
	if ( !okBut )
	    cnclBut->setDefault();
    }
    if ( helpBut )
	helpBut->activated.notify( mCB(this,uiDialogBody,provideHelp) );

    childrenInited = true;
}


uiObject* uiDialogBody::createChildren()
{
    if ( setup.oktext_ != "" )
	okBut = new uiPushButton( centralWidget_, setup.oktext_ );
    if ( setup.canceltext_ != "" )
	cnclBut = new uiPushButton( centralWidget_, setup.canceltext_ );

    if ( setup.savebutton_ && setup.savetext_ != "" )
    {
	if( setup.savebutton_ == uiDialog::Setup::PushButton )
	    saveBut_pb= new uiPushButton( centralWidget_, setup.savetext_);
	else
	{
	    saveBut_cb = new uiCheckBox( centralWidget_, setup.savetext_ );
	    saveBut_cb->setChecked( setup.savechecked_ );
	}
    }
    if ( setup.helpid_ != "" )
    {
	const ioPixmap pixmap( GetDataFileName("contexthelp.png") );
	helpBut = new uiToolButton( centralWidget_, "Help button", pixmap );
	helpBut->setPrefWidthInChar( 5 );
	bool shwhid = getenv( "DTECT_SHOW_HELP" );
#ifdef __debug__
	shwhid = true;
#endif
	helpBut->setToolTip( shwhid ? setup.helpid_.buf()
				    : "Help on this window" );
    }
    if ( !setup.menubar_ && setup.dlgtitle_ != "" )
    {
	title = new uiLabel( centralWidget_, setup.dlgtitle_ );

	uiObject* obj = setup.separator_ 
			    ? (uiObject*) new uiSeparator(centralWidget_)
			    : (uiObject*) title;

	if ( obj != title )
	{
	    title->attach( centeredAbove, obj );
	    obj->attach( stretchedBelow, title, -2 );
	}
	if ( setup.mainwidgcentered_ )
	    dlgGroup->attach( centeredBelow, obj );
	else
	    dlgGroup->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlgGroup->mainObject();
    if ( setup.separator_ && ( okBut || cnclBut || saveBut_cb || 
			       saveBut_pb || helpBut) )
    {
	horSepar = new uiSeparator( centralWidget_ );
	horSepar->attach( stretchedBelow, dlgGroup, -2 );
	lowestobj = horSepar;
    }

    return lowestobj;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = okBut;
    uiObject* rightbut = cnclBut;
    uiObject* centerbut = 0;
    uiObject* extrabut1 = 0; uiObject* extrabut2 = 0;
    uiObject* savebut = saveBut_cb ? (uiObject*)saveBut_cb
				   : (uiObject*)saveBut_pb;
    uiObject* nearokbut = saveBut_cb;

    if ( okBut && !cnclBut )	{ centerbut = okBut; leftbut = 0; }
    if ( cnclBut && !okBut )	{ centerbut = cnclBut; rightbut = 0; }

    if ( !centerbut )		centerbut = helpBut;
    else			extrabut1 = helpBut;

    if ( !centerbut )		centerbut = savebut;
    else if ( !extrabut1 )	extrabut1 = savebut;
    else 			extrabut2 = savebut;

    // Exception: save checkbox needs to be near OK button.
    if ( okBut && nearokbut )
    {
	if ( centerbut == nearokbut ) centerbut = 0;
	if ( extrabut1 == nearokbut ) extrabut1 = 0;
	if ( extrabut2 == nearokbut ) extrabut2 = 0;
    }

#	define mCommonLayout(but) \
	but->attach( ensureBelow, lowestobj ); \
	but->attach( bottomBorder, 0 )

    if ( leftbut )
    {
	mCommonLayout(leftbut);
	leftbut->attach( leftBorder );
    }
    if ( rightbut )
    {
	mCommonLayout(rightbut);
	rightbut->attach( rightBorder );
	if ( leftbut )
	    rightbut->attach( ensureRightOf, leftbut );
    }
    if ( centerbut )
    {
	mCommonLayout(centerbut);
	centerbut->attach( centeredBelow, horSepar
			? (uiObject*)horSepar
			: (uiObject*)centralWidget_->mainObject() );
	if ( leftbut )
	    centerbut->attach( ensureRightOf, leftbut );
	if ( rightbut )
	    centerbut->attach( ensureLeftOf, rightbut );
    }
    if ( extrabut1 )
    {
	mCommonLayout(extrabut1);
	extrabut1->attach( rightOf, centerbut );
	if ( rightbut )
	    extrabut1->attach( ensureLeftOf, rightbut );
    }
    if ( extrabut2 )
    {
	mCommonLayout(extrabut2);
	extrabut1->attach( leftOf, centerbut );
	if ( leftbut )
	    extrabut2->attach( ensureRightOf, leftbut );
    }
    if ( okBut && nearokbut )
    {
	mCommonLayout(nearokbut);
	nearokbut->attach( okBut == leftbut ? rightOf : leftOf, okBut );
	if ( centerbut != okBut )
	    centerbut->attach( ensureRightOf, nearokbut );
    }
}


void uiDialogBody::provideHelp( CallBacker* )
{
    uiMainWin::provideHelp( setup.helpid_ );
}


#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
	: uiMainWin(s.wintitle_)
    	, cancelpushed_(false)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.nrstatusflds_, s.menubar_, s.toolbar_ );
    uiGroup* cw= new uiGroup( body_->uiCentralWidg(), "Dialog box client area");

    cw->setStretch( 2, 2 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
}


int uiDialog::go()				{ return mBody->exec(); }
const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( int i )			{ mBody->done( i ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setCaption( const char* txt )	{ mBody->setCaption(txt); }
void uiDialog::setTitleText( const char* txt )	{ mBody->setTitleText(txt); }
void uiDialog::setOkText( const char* txt )	{ mBody->setOkText(txt); }
void uiDialog::setCancelText( const char* txt )	{ mBody->setCancelText(txt);}
void uiDialog::enableSaveButton(const char* t)  { mBody->enableSaveButton(t); }
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpID( const char* id )	{ mBody->setHelpID(id); }
const char* uiDialog::helpID() const		{ return mBody->helpID(); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setButtonSensitive(uiDialog::Button b, bool s ) 
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b) 
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
