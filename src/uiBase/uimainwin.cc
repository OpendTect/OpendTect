/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          31/05/2000
 RCS:           $Id: uimainwin.cc,v 1.132 2007-09-19 17:18:50 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uidialog.h"
#include "uibody.h"
#include "uiobjbody.h"
#include "uifont.h"

#include "uibutton.h"
#include "uidesktopservices.h"
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
# include <QDockWidget>
# include <QTextStream>
# include <QCloseEvent>
#endif

#include <qapplication.h>
#include <qcolordialog.h>
#include <qdialog.h>
#include <qfiledialog.h>
#include <qfontdialog.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qpixmap.h>
#include <qstatusbar.h>
#include <qwidget.h>

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

    void		activateClose();  //! force activation in GUI thread
    void		activateQDlg( int retval ); 

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
    virtual QMenu*	createPopupMenu()		{ return 0; }

    uiPopupMenu&	getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    void		setModal( bool yn )		{ modal_ = yn; }
    bool		isModal() const			{ return modal_; }

protected:

    virtual void	finalise( bool trigger_finalise_start_stop=true );
    void		closeEvent(QCloseEvent*);
    void 		closeQDlgChild(int retval,bool parentcheck=true);
    bool		event(QEvent*);

    void		renewToolbarsMenu();
    void		toggleToolbar(CallBacker*);

    bool		exitapponclose_;
    int			qdlgretval_;

    uiStatusBar* 	statusbar;
    uiMenuBar* 		menubar;
    uiPopupMenu*	toolbarsmnu_;

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
	, initing(true)
	, centralWidget_(0)
	, statusbar(0)
	, menubar(0)
	, toolbarsmnu_(0)
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
    iconsz_ = uiObject::iconSize();
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

    toolbarsmnu_ = new uiPopupMenu( &handle_, "Toolbars" );
    initing = false;
}


uiMainWinBody::~uiMainWinBody( )
{
    toolbarsmnu_->clear();
    delete toolbarsmnu_;
}


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


void uiMainWinBody::closeQDlgChild( int retval, bool parentcheck )
{
    QWidget* _amw = qApp->activeModalWidget();
    if ( !_amw ) 
	return;

    QDialog* _qdlg = dynamic_cast<QDialog*>(_amw);
    if ( !_qdlg ) 
	return;
    if ( parentcheck && _qdlg->parent()!=this )
	return;

    _qdlg->done( retval );
}


static const QEvent::Type sQEventActClose = (QEvent::Type) (QEvent::User+0);
static const QEvent::Type sQEventActQDlg  = (QEvent::Type) (QEvent::User+1);

bool uiMainWinBody::event( QEvent* ev )
{
    if ( ev->type() == sQEventActClose )
	close(); 
    else if ( ev->type() == sQEventActQDlg )
	closeQDlgChild( qdlgretval_, false );
	// Using parentcheck=true would be neat, but it turns out that
	// QDialogs not always have their parent set correctly (yet).
    else
	return QMainWindow::event( ev );
    
    handle_.activatedone.trigger(handle_);
    return true; 
}


void uiMainWinBody::activateClose()
{
    QEvent* actcloseevent = new QEvent( sQEventActClose );
    QApplication::postEvent( this, actcloseevent );
}


void uiMainWinBody::activateQDlg( int retval )
{
    qdlgretval_ = retval;
    QEvent* actqdlgevent = new QEvent( sQEventActQDlg );
    QApplication::postEvent( this, actqdlgevent );
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

    wins2move.erase();
    docks4wins.erase();
}


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


void uiMainWinBody::toggleToolbar( CallBacker* cb )
{
    mDynamicCastGet( uiMenuItem*, itm, cb );
    if ( !itm ) return;

    ObjectSet<uiToolBar>& toolbars = uiToolBar::toolBars();
    for ( int tbidx=0; tbidx<toolbars.size(); tbidx++ )
    {
	uiToolBar& tb = *toolbars[tbidx];
	if ( tb.name()==itm->name() && tb.parent()==&handle_ )
	    tb.display( tb.isHidden() );
    }
}


void uiMainWinBody::updateToolbarsMenu()
{
    const ObjectSet<uiMenuItem>& items = toolbarsmnu_->items();
    const ObjectSet<uiToolBar>& toolbars = uiToolBar::toolBars();

    for ( int tbidx=0; tbidx<toolbars.size(); tbidx++ )
    {
	const uiToolBar& tb = *toolbars[tbidx];
	for ( int itmidx=0; itmidx<items.size(); itmidx++ )
	{
	    uiMenuItem& itm = *const_cast<uiMenuItem*>( items[itmidx] );
	    if ( itm.name()==tb.name() && tb.parent()==&handle_ )
		itm.setChecked( !tb.isHidden() );
	}
    }
}


void uiMainWinBody::renewToolbarsMenu()
{
    toolbarsmnu_->clear();
    const ObjectSet<uiToolBar>& toolbars = uiToolBar::toolBars();

    for ( int tbidx=0; tbidx<toolbars.size(); tbidx++ )
    {
	const uiToolBar& tb = *toolbars[tbidx];
	if ( tb.parent() != &handle_ )
	    continue;

	uiMenuItem* itm =
	    new uiMenuItem( tb.name(), mCB(this,uiMainWinBody,toggleToolbar) );
	toolbarsmnu_->insertItem( itm );
	itm->setCheckable( true );
	itm->setChecked( !tb.isHidden() );
    }
}


uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      int nrstatusflds, bool wantMBar, bool modal )
    : uiParent(nm,0)
    , body_(0)
    , parent_(parnt)			
    , windowClosed(this)
    , activatedone(this)
{ 
    body_= new uiMainWinBody( *this, parnt, nm, modal ); 
    setBody( body_ );
    body_->construct( nrstatusflds, wantMBar );
    if ( !parnt )
	setIcon( dtect_xpm_data, "OpendTect" );
}


uiMainWin::uiMainWin( const char* nm, uiParent* parnt )
    : uiParent(nm,0)
    , body_(0)			
    , parent_(parnt)
    , windowClosed(this)
    , activatedone(this)
{}


uiMainWin::~uiMainWin()
{ delete body_; }

QWidget* uiMainWin::qWidget() const
{ return body_; }

void uiMainWin::provideHelp( const char* winid )
{
    BufferString fnm = HelpViewer::getURLForWinID( winid );
#ifdef USEQT3
    HelpViewer::use( fnm );
#else
    uiDesktopServices::openUrl( fnm );
#endif
}

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }

void uiMainWin::show()				{ body_->go(); }
void uiMainWin::close()				{ body_->close(); }
void uiMainWin::activateClose()			{ body_->activateClose(); }
void uiMainWin::activateQDlg( int retval )	{ body_->activateQDlg(retval); }
void uiMainWin::setCaption( const char* txt )	{ body_->setCaption(txt); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::touch() 			{ return body_->touch(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }

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
    body_->renewToolbarsMenu();
#endif
}


void uiMainWin::removeToolBar( uiToolBar* tb )
{
#ifndef USEQT3
    body_->removeToolBar( tb->qwidget() );
    body_->renewToolbarsMenu();
#endif
}


void uiMainWin::addToolBarBreak()
{
#ifndef USEQT3
    body_->addToolBarBreak();
#endif 
}


uiPopupMenu& uiMainWin::getToolbarsMenu() const
{ return body_->getToolbarsMenu(); }


void uiMainWin::updateToolbarsMenu()
{ body_->updateToolbarsMenu(); }
    

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
    if ( !_awb )	return 0;

    return &_awb->handle();
}


uiMainWin::ActModalTyp uiMainWin::activeModalType()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )					return None;

    if ( dynamic_cast<uiMainWinBody*>(amw) ) 	return Main;
    if ( dynamic_cast<QMessageBox*>(amw) ) 	return Message;
    if ( dynamic_cast<QFileDialog*>(amw) ) 	return File;
    if ( dynamic_cast<QColorDialog*>(amw) ) 	return Colour;
    if ( dynamic_cast<QFontDialog*>(amw) ) 	return Font;

    return Unknown;
}
    

uiMainWin* uiMainWin::activeModalWindow()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )	return 0;

    uiMainWinBody* mwb = dynamic_cast<uiMainWinBody*>( amw );
    if ( !mwb )	return 0;

    return &mwb->handle();
}


char* uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    if ( typ == Message )
    {
        const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
	const char* buttext = qmb->buttonText( buttonnr );
	return const_cast<char*>( buttext );
    }

    if ( typ==Colour || typ==Font )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 ) return "OK";
	return "";
    }

    if ( typ == File )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 )
	{
	    const QFileDialog* qfd = dynamic_cast<QFileDialog*>( amw );

	    if ( qfd->acceptMode() == QFileDialog::AcceptOpen ) return "Open";
	    if ( qfd->acceptMode() == QFileDialog::AcceptSave ) return "Save";
	}
	return "";
    }
    
    return 0;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    return buttonnr;
}


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& windowlist )
{
    windowlist.erase();
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();
    for ( int idx=0; idx<toplevelwigs.count(); idx++ )
    {
	QWidget* widget = toplevelwigs.at( idx );
	if ( widget && !widget->isHidden() )
	{
	    uiMainWinBody* uimwb = dynamic_cast<uiMainWinBody*>(widget);
	    if ( uimwb )
		windowlist += &uimwb->handle();
	}
    }
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

    const int hborderdist = 1;
    const int vborderdist = 5;

#define mCommonLayout(but) \
    but->attach( ensureBelow, lowestobj ); \
    but->attach( bottomBorder, vborderdist )

    if ( leftbut )
    {
	mCommonLayout(leftbut);
	leftbut->attach( leftBorder, hborderdist );
    }

    if ( rightbut )
    {
	mCommonLayout(rightbut);
	rightbut->attach( rightBorder, hborderdist );
	if ( leftbut )
	    rightbut->attach( ensureRightOf, leftbut );
    }

    if ( centerbut )
    {
	mCommonLayout(centerbut);
	centerbut->attach( hCentered );
	if ( leftbut )
	    centerbut->attach( ensureRightOf, leftbut );
	if ( rightbut )
	    centerbut->attach( ensureLeftOf, rightbut );
    }

    if ( saveBut_cb )
    {
	saveBut_cb->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, saveBut_cb );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, saveBut_cb );
    }

    if ( extrabut )
    {
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
	: uiMainWin( s.wintitle_, p )
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
