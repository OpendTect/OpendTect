/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.cc,v 1.118 2007-03-22 12:47:42 cvsdgb Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uidialog.h"
#include "uibody.h"
#include "uiobjbody.h"
#include "uifont.h"

#include "uibutton.h"
#include "uidockwin.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiparentbody.h"
#include "uistatusbar.h"
#include "uiseparator.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "errh.h"
#include "helpview.h"
#include "msgh.h"
#include "pixmap.h"
#include "settings.h"
#include "timer.h"

#include <iostream>

#ifdef USEQT3
# define mQDockArea	QDockArea
# define mQDockWindow	QDockWindow
# define mFlagType	int
# define mDockType	Qt::Dock
# include <qmainwindow.h>
# include <qdockwindow.h>
#else
# define mQDockWindow	QDockWidget
# define mQDockArea	QDockArea
# define mFlagType	Qt::WFlags
# define mDockType	Qt::ToolBarDock
# include <QMainWindow>
# include <Q3PopupMenu>
# include <QDockWidget>
# include <QTextStream>
# include <QCloseEvent>
#endif

#include <qwidget.h>
#include <qstatusbar.h>
#include <qapplication.h>
#include <qpixmap.h>
#include <qlayout.h>

#ifdef SUPPORT_PERSISTENCE
// for positions of dock area's
#ifdef USEQT3
# include <qdockarea.h>
# else
#  include <Q3DockArea>
# endif
#include <qfile.h>
#endif

#include "dtect.xpm"

class uiMainWinBody : public uiParentBody
		    , public QMainWindow
{
friend class		uiMainWin;
public:
			uiMainWinBody( uiMainWin& handle, uiParent* parnt,
				       const char* nm, bool modal );

    void		construct( int nrstatusflds, bool wantmenubar );

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

    virtual void        polish() { QMainWindow::polish(); }

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

			    if ( modal_ )	
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

    static mDockType	qdock(uiMainWin::Dock);

    void 		uimoveDockWindow(uiDockWin&,uiMainWin::Dock,int);

#ifdef SUPPORT_PERSISTENCE
    void		storePositions();
    void		restorePositions();
#endif

    void		setModal( bool yn )	{ modal_ = yn; }
    bool		isModal() const		{ return modal_; }

protected:

    virtual void	finalise( bool trigger_finalise_start_stop=true );
    void		closeEvent(QCloseEvent*);

    bool		exitapponclose_;

    uiStatusBar* 	statusbar;
    uiMenuBar* 		menubar;

private:

    int			iconsz_;
    bool		modal_;
    int			looplevel__;
    mFlagType		getFlags(bool hasparent,bool modal) const;

    void 		popTimTick(CallBacker*);
    Timer		poptimer;
    bool		popped_up;


    ObjectSet<uiDockWin>	wins2move;
    TypeSet<uiMainWin::Dock>	docks4wins;
    void			moveDockWindows();
};



uiMainWinBody::uiMainWinBody( uiMainWin& handle__, uiParent* p, 
			      const char* nm, bool modal )
	: uiParentBody(nm)
	, QMainWindow(p && p->pbody() ? p->pbody()->qwidget() : 0,
		      nm,getFlags(p,modal) )
	, handle_(handle__)
	, initing( true )
	, centralWidget_( 0 )
	, statusbar(0)
	, menubar(0)
	, modal_(p && modal)
	, poptimer("Popup timer")
	, popped_up(false)
	, exitapponclose_(false)
{
    if ( nm && *nm ) setCaption( nm );
    poptimer.tick.notify( mCB(this,uiMainWinBody,popTimTick) );

#ifdef USEQT3
    setDockMenuEnabled( false );
#else
    iconsz_ = 24;
    Settings::common().get( "dTect.Icons.size", iconsz_ );
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( modal ? Qt::WindowModal : Qt::NonModal );
#endif
}


mFlagType uiMainWinBody::getFlags( bool hasparent, bool modal ) const
{
#ifdef USEQT3
    return hasparent && modal ? WType_TopLevel | WShowModal| WGroupLeader
				  : WType_TopLevel;
#else
    return  Qt::WindowFlags( hasparent ? Qt::Dialog : Qt::Window );
#endif
}


void uiMainWinBody::construct( int nrstatusflds, bool wantmenubar )
{
    centralWidget_ = new uiGroup( &handle(), "uiMainWin central widget" );
    setCentralWidget( centralWidget_->body()->qwidget() ); 

    centralWidget_->setIsMain(true);
    centralWidget_->setBorder(10);
    centralWidget_->setStretch(2,2);

    if ( nrstatusflds != 0 )
    {
	QStatusBar* mbar= statusBar();
	if ( mbar )
	    statusbar = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    pErrMsg("No statusbar returned from Qt");

	if ( nrstatusflds > 0 )
	{
	    for( int idx=0; idx<nrstatusflds; idx++ )
		statusbar->addMsgFld();
	}
    }
    if ( wantmenubar )
    {   
	QMenuBar* myBar =  menuBar();

	if ( myBar )
	    menubar = new uiMenuBar( &handle(), "MainWindow MenuBar handle", 
				      *myBar);
	else
	    pErrMsg("No menubar returned from Qt");
    }

    initing = false;
}


uiMainWinBody::~uiMainWinBody( )
{}


void uiMainWinBody::popTimTick( CallBacker* )
{
    if ( popped_up ) { pErrMsg( "huh?" ); }
	popped_up = true;
    moveDockWindows();
}


void uiMainWinBody::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
	handle_.finaliseStart.trigger( handle_ );

    centralWidget_->finalise();
    finaliseChildren();

    if ( trigger_finalise_start_stop )
	handle_.finaliseDone.trigger( handle_ );
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    if ( handle_.closeOK() )
    {
	handle_.windowClosed.trigger( handle_ );
	ce->accept();
    }
    else
	ce->ignore();
}


void uiMainWinBody::close()
{
    if ( !handle_.closeOK() ) return; 

    handle_.windowClosed.trigger( handle_ );

    if ( modal_ )
	qApp->exit_loop();

    QMainWindow::hide();

    if ( exitapponclose_ )
	qApp->quit();
}


uiStatusBar* uiMainWinBody::uistatusbar()
{
    return statusbar;
}


uiMenuBar* uiMainWinBody::uimenubar()
{
    return menubar;
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

//    for( int idx=0; idx<wins2move.size(); idx++ )
//	moveDockWindow( wins2move[idx]->qwidget(), qdock(docks4wins[idx]) );

    wins2move.erase();
    docks4wins.erase();
}

#ifdef SUPPORT_PERSISTENCE
void uiMainWinBody::storePositions()
{
#ifdef USE_FILE
    QFile outfil( "/tmp/qpositions.txt");
# ifdef USEQT3
    outfil.open( IO_WriteOnly );
# else
    outfil.open( QIODevice::WriteOnly );
# endif
    QTextStream ts( &outfil );
#else
    static QString str;
    str="";
# ifdef USEQT3
    QTextStream ts( &str, IO_WriteOnly  );
# else
    QTextStream ts( &str, QIODevice::WriteOnly  );
# endif
#endif

    mQDockArea* dck = leftDock();
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
    infil.open( QIODevice::ReadOnly );
    QTextStream ts( &infil );

    mQDockArea* dck = leftDock();
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

mDockType uiMainWinBody::qdock( uiMainWin::Dock d )
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
    return (mDockType) 0;
}


uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      int nrstatusflds, bool wantMBar, bool modal )
    : uiParent( nm, 0 )
    , body_( 0 )
    , windowClosed(this)
{ 
    body_= new uiMainWinBody( *this, parnt, nm, modal ); 
    setBody( body_ );
    body_->construct( nrstatusflds, wantMBar );
    if ( !parnt )
	setIcon( dtect_xpm_data, "OpendTect" );
}


uiMainWin::uiMainWin( const char* nm )
    : uiParent( nm, 0 )
    , body_( 0 )			
    , windowClosed(this)
{}


uiMainWin::~uiMainWin()
{ delete body_; }

QWidget* uiMainWin::qWidget() const
{ return body_; }

void uiMainWin::provideHelp( const char* winid )
{ HelpViewer::use( HelpViewer::getURLForWinID(winid) ); }

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }

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

#ifdef USEQT3
    body_->removeDockWindow( dwin->qwidget() );
#else
    body_->removeDockWidget( dwin->qwidget() );
#endif
}


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{
#ifdef USEQT3
    body_->addDockWindow( dwin.qwidget(), body_->qdock(d) );
#else
    body_->addDockWidget( Qt::LeftDockWidgetArea, dwin.qwidget() );
#endif
//    dwin.qwidget()->show();
}


void uiMainWin::addToolBar( uiToolBar* tb )
{
#ifndef USEQT3
    body_->addToolBar( tb->qwidget() );
#endif
}


void uiMainWin::removeToolBar( uiToolBar* tb )
{
#ifndef USEQT3
    body_->removeToolBar( tb->qwidget() );
#endif
}


void uiMainWin::addToolBarBreak()
{
#ifndef USEQT3
    body_->addToolBarBreak();
#endif 
}


uiGroup* uiMainWin::topGroup()	    	   { return body_->uiCentralWidg(); }

void uiMainWin::setShrinkAllowed(bool yn)  
    { if ( topGroup() ) topGroup()->setShrinkAllowed(yn); }

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
    if ( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if ( !_awb )		return 0;

    return &_awb->handle();

}


void uiMainWin::setSensitive( bool yn )
{
/*
#ifdef USEQT3
    QPtrList<QDockWindow> dws = body_->dockWindows();
#else
    QList<mQDockWindow*> dws = body_->dockWindows();
#endif
    for ( int idx=0; idx<dws.count(); idx++ )
    {
	mQDockWindow* qdwin = dws.at( idx );
	if ( qdwin ) qdwin->setEnabled( yn );
    }
*/
    if ( menuBar() ) menuBar()->setSensitive( yn );
    body_->setEnabled( yn );
}


uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if ( !mwimpl ) return 0;

    uiMainWinBody* _mwb = dynamic_cast<uiMainWinBody*>( mwimpl );
    if ( !_mwb ) return 0;

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

uiPopupMenu& uiMainWin::createDockWindowMenu()
{
#ifdef USEQT3
    QPopupMenu* qmnu = body_->createDockWindowMenu();
    return *new uiPopupMenu(this,qmnu,"Toolbars");
#else
    Q3PopupMenu* q3mnu = new Q3PopupMenu();
    q3mnu->addActions( body_->createPopupMenu()->actions() );
    return *new uiPopupMenu(this,q3mnu,"Toolbars");
#endif
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
				setup.savebutton_ = true;
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
    setContentsMargins( 10, 2, 10, 2 );
}


int uiDialogBody::exec()
{ 
    uiSetResult( 0 );

    if ( setup.fixedsize_ )
    {
	setMaximumSize( QSize(0,0) );
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );
    }

    go();

    return uiResult();
}



void uiDialogBody::setOkText( const char* txt )    
{ 
    setup.oktext_ = txt; 
    if ( okBut ) okBut->setText(txt);
}


void uiDialogBody::setTitleText( const char* txt )    
{ 
    setup.dlgtitle_ = txt; 
    if ( title ) 
    { 
	title->setText(txt); 
	uiObjectBody* tb = dynamic_cast<uiObjectBody*>( title->body() ); 
	if ( tb && !tb->itemInited() )
	    title->setPrefWidthInChar( 
		    mMAX( tb->prefWidthInCharSet(), strlen(txt) + 2 )); 
    }
}

void uiDialogBody::setCancelText( const char* txt ) 
{ 
    setup.canceltext_ = txt; 
    if ( cnclBut ) cnclBut->setText(txt);
}


bool uiDialogBody::saveButtonChecked() const
{ 
    return saveBut_cb ? saveBut_cb->isChecked() : false;
}


/*! Hides the box, which also exits the event loop in case of a modal box.  */
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
void uiDialogBody::finalise( bool ) 
{
    uiMainWinBody::finalise( false ); 

    handle_.finaliseStart.trigger( handle_ );

    dlgGroup->finalise();

    if ( !childrenInited ) 
	initChildren();

    finaliseChildren();

    handle_.finaliseDone.trigger( handle_ );
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
    if ( !setup.oktext_.isEmpty() )
	okBut = new uiPushButton( centralWidget_, setup.oktext_, true );
    if ( !setup.canceltext_.isEmpty() )
	cnclBut = new uiPushButton( centralWidget_, setup.canceltext_, true );

    if ( setup.savebutton_ && !setup.savetext_.isEmpty() )
    {
	if ( setup.savebutispush_ )
	    saveBut_pb= new uiPushButton( centralWidget_, setup.savetext_,
		    			  true);
	else
	{
	    saveBut_cb = new uiCheckBox( centralWidget_, setup.savetext_ );
	    saveBut_cb->setChecked( setup.savechecked_ );
	}
    }
    if ( !setup.helpid_.isEmpty() )
    {
	const ioPixmap pixmap( "contexthelp.png" );
	helpBut = new uiToolButton( centralWidget_, "&Help button", pixmap );
	helpBut->setPrefWidthInChar( 5 );
	static bool shwhid = GetEnvVarYN( "DTECT_SHOW_HELP" );
#ifdef __debug__
	shwhid = true;
#endif
	helpBut->setToolTip( shwhid ? setup.helpid_.buf()
				    : "Help on this window" );
    }
    if ( !setup.menubar_ && !setup.dlgtitle_.isEmpty() )
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


#define mPr(i) { std::cout << i << std::endl;

void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = okBut;
    uiObject* rightbut = cnclBut;
    uiObject* exitbut = okBut ? okBut : cnclBut;
    uiObject* centerbut = helpBut;
    uiObject* extrabut = saveBut_pb;

    if ( !okBut || !cnclBut )
    {
	leftbut = rightbut = 0;
	if ( exitbut )
	{
	    centerbut = exitbut;
	    extrabut = helpBut;
	    leftbut = saveBut_pb;
	}
    }

    if ( !centerbut )
    {
	centerbut = extrabut;
	extrabut = 0;
    }

    const int borderdist = 5;

#define mCommonLayout(but) \
    but->attach( ensureBelow, lowestobj ); \
    but->attach( bottomBorder, 0 )

    if ( leftbut )
    {
	mCommonLayout(leftbut);
	leftbut->attach( leftBorder, borderdist );
    }

    if ( rightbut )
    {
	mCommonLayout(rightbut);
	rightbut->attach( rightBorder, borderdist );
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

    if ( saveBut_cb )
    {
	mCommonLayout(saveBut_cb);
	saveBut_cb->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, saveBut_cb );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, saveBut_cb );
    }

    if ( extrabut )
    {
	mCommonLayout(extrabut);
	extrabut->attach( rightOf, centerbut );
	if ( rightbut )
	    extrabut->attach( ensureLeftOf, rightbut );
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
    body_->construct( s.nrstatusflds_, s.menubar_ );
    uiGroup* cw= new uiGroup( body_->uiCentralWidg(), "Dialog box client area");

    cw->setStretch( 2, 2 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
}


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    switch ( cs )
    {
    case DoAndLeave:
	setOkText( "&Ok" );
	setCancelText( "&Cancel" );
    break;
    case DoAndStay:
	setOkText( "&Go" );
	setCancelText( "&Dismiss" );
    break;
    case LeaveOnly:
	setOkText( mBody->finalised() ? "&Dismiss" : "" );
	setCancelText( "&Dismiss" );
    break;
    }
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
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }
