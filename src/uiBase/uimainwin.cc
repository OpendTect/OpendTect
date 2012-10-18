/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uimainwin.h"
#include "uidialog.h"

#include "uibody.h"
#include "uidesktopservices.h"
#include "uidockwin.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "uistatusbar.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "errh.h"
#include "filepath.h"
#include "helpview.h"
#include "msgh.h"
#include "keyboardevent.h"
#include "oddirs.h"
#include "odver.h"
#include "pixmap.h"
#include "settings.h"
#include "strmprov.h"
#include "texttranslator.h"
#include "thread.h"
#include "timer.h"
#include "iopar.h"
#include "thread.h"

#include <iostream>

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QStatusBar>
#include <QWidget>


class uiMainWinBody : public uiParentBody
		    , public QMainWindow
{
friend class		uiMainWin;
public:
			uiMainWinBody(uiMainWin& handle,uiParent* parnt,
				      const char* nm,bool modal);

    void		construct(int nrstatusflds,bool wantmenubar);

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

    virtual void        polish();
    void		reDraw(bool deep);
    void		go(bool showminimized=false);
    virtual void	show()				{ doShow(); }

    void		move(uiMainWin::PopupArea);
    void		move(int,int);

    void		close();
    bool		poppedUp() const		{ return popped_up; }
    bool		touch();

    void		removeDockWin(uiDockWin*);
    void		addDockWin(uiDockWin&,uiMainWin::Dock);

    virtual QMenu*	createPopupMenu();
    void		addToolBar(uiToolBar*);
    void		removeToolBar(uiToolBar*);
    uiPopupMenu&	getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const	{ return toolbars_; }
    const ObjectSet<uiDockWin>& dockWins() const	{ return dockwins_; }

    void		setModal(bool yn);
    bool		isModal() const			{ return modal_; }

    void		setWindowTitle(const char*);

    void		activateInGUIThread(const CallBack&,bool busywait);

protected:

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool trigger_finalise_start_stop);
    void		closeEvent(QCloseEvent*);
    bool		event(QEvent*);  

    void		keyPressEvent(QKeyEvent*);

    void		doShow(bool minimized=false);
    void		managePopupPos();


    void		renewToolbarsMenu();
    void		toggleToolbar(CallBacker*);

    void		saveSettings();
    void		readSettings();

    bool		exitapponclose_;

    Threads::Mutex	activatemutex_;
    ObjectSet<CallBack>	activatecbs_;
    int			nractivated_;

    int			eventrefnr_;

    uiStatusBar* 	statusbar;
    uiMenuBar* 		menubar;
    uiPopupMenu*	toolbarsmnu_;
    
    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiDockWin> dockwins_;

private:

    QEventLoop		eventloop_;

    int			iconsz_;
    bool		modal_;
    int			looplevel__;
    Qt::WFlags		getFlags(bool hasparent,bool modal) const;

    void 		popTimTick(CallBacker*);
    Timer		poptimer;
    bool		popped_up;
    uiSize		prefsz_;
    uiPoint		prefpos_;
    bool		moved_;
    bool		createtbmenu_;

    bool		deletefrombody_;
    bool		deletefromod_;
};


uiMainWinBody::uiMainWinBody( uiMainWin& uimw, uiParent* p, 
			      const char* nm, bool modal )
	: uiParentBody(nm)
	, QMainWindow(p && p->pbody() ? p->pbody()->qwidget() : 0,
		      getFlags(p,modal) )
	, handle_(uimw)
	, initing(true)
	, centralWidget_(0)
	, statusbar(0)
	, menubar(0)
	, toolbarsmnu_(0)
	, modal_(p && modal)
	, poptimer("Popup timer")
	, popped_up(false)
	, exitapponclose_(false)
        , prefsz_(-1,-1)
	, prefpos_(uiPoint::udf())
	, nractivated_(0)
	, moved_(false)
	, createtbmenu_(false)
{
    if ( nm && *nm )
	setObjectName( nm );

    poptimer.tick.notify( mCB(this,uiMainWinBody,popTimTick) );

    iconsz_ = uiObject::iconSize();
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( p && modal ? Qt::WindowModal : Qt::NonModal );

    setDockOptions( VerticalTabs | AnimatedDocks );

    deletefrombody_ = deletefromod_ = false;
}


uiMainWinBody::~uiMainWinBody()
{
    deleteAllChildren(); //delete them now to make sure all ui objects
    			 //are deleted before their body counterparts

    while ( toolbars_.size() )
	delete toolbars_[0];

    if ( toolbarsmnu_ ) toolbarsmnu_->clear();
    delete toolbarsmnu_;

    if ( !deletefromod_ )
    {
	deletefrombody_ = true;
	delete &handle_;
    }

    delete statusbar;
    delete menubar;
}


void uiMainWinBody::setModal( bool yn )
{
    modal_ = yn;
    setWindowModality( yn ? Qt::WindowModal : Qt::NonModal );
}


Qt::WFlags uiMainWinBody::getFlags( bool hasparent, bool modal ) const
{
    return  Qt::WindowFlags( hasparent ? Qt::Dialog : Qt::Window );
}


void uiMainWinBody::doShow( bool minimized )
{
    setWindowTitle( handle_.caption(false) );
    eventrefnr_ = handle_.beginCmdRecEvent("WinPopUp");
    managePopupPos();

    if ( minimized )
	QMainWindow::showMinimized();
    else
    {
	if ( isMinimized() ) showNormal();
	QMainWindow::show();
    }

    if( poptimer.isActive() )
	poptimer.stop();

    popped_up = false;
    poptimer.start( 100, true );

    QEvent* ev = new QEvent( mUsrEvPopUpReady );
    QApplication::postEvent( this, ev );

    if ( modal_ )
	eventloop_.exec();
}


void uiMainWinBody::construct( int nrstatusflds, bool wantmenubar )
{
    centralWidget_ = new uiGroup( &handle(), "OpendTect Main Window" );
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

	toolbarsmnu_ = new uiPopupMenu( &handle(), "Toolbars" );
    }

    initing = false;
}


void uiMainWinBody::move( uiMainWin::PopupArea pa )
{
    QDesktopWidget wgt;
    const int xpos = wgt.screen()->width() - QMainWindow::width();
    const int ypos = wgt.screen()->height() - QMainWindow::height();
   
    switch( pa )
    {
	case uiMainWin::TopLeft :
	    move( 0, 0 ); break;
	case uiMainWin::TopRight :
	    move( xpos, 0 ); break;
	case uiMainWin::BottomLeft :
	    move( 0, ypos ); break;
	case uiMainWin::BottomRight :
	    move( xpos, ypos ); break;
	case uiMainWin::Middle :
	    move( mNINT32(((float) xpos)/2), mNINT32(((float) ypos) / 2)); break;
	default:
	    break;
    }
}


void uiMainWinBody::move( int xdir, int ydir )
{
    QWidget::move( xdir, ydir );
    moved_ = true;
}


void uiMainWinBody::polish()
{ QMainWindow::ensurePolished(); }


void uiMainWinBody::reDraw( bool deep )
{
    update();
    centralWidget_->reDraw( deep );
}


void uiMainWinBody::go( bool showminimized )
{
    finalise( true );
    doShow( showminimized );
    move( handle_.popuparea_ );
}


bool uiMainWinBody::touch()
{
    if ( popped_up || !finalised() )
	return false;

    if ( poptimer.isActive() )
	poptimer.stop();

    if ( !popped_up )
	poptimer.start( 100, true );

    return true;
}


QMenu* uiMainWinBody::createPopupMenu()
{ return createtbmenu_ ? QMainWindow::createPopupMenu() : 0; }


void uiMainWinBody::popTimTick( CallBacker* )
{
    if ( popped_up ) { pErrMsg( "huh?" );  return; }
	popped_up = true;

// TODO: Remove when we can get rid of the popTimTick
    if ( prefsz_.hNrPics()>0 && prefsz_.vNrPics()>0 )
	resize( prefsz_.hNrPics(), prefsz_.vNrPics() );
    if ( prefpos_ != uiPoint::udf() )
	move( prefpos_.x, prefpos_.y );
}


void uiMainWinBody::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
	handle_.preFinalise().trigger( handle_ );

    centralWidget_->finalise();
    finaliseChildren();

    if ( trigger_finalise_start_stop )
	handle_.postFinalise().trigger( handle_ );
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    if ( handle_.closeOK() )
    {
	handle_.windowClosed.trigger( handle_ );
	ce->accept();

	if ( modal_ )
	    eventloop_.exit();
    }
    else
	ce->ignore();

     handle_.endCmdRecEvent( refnr, "Close" );
}


void uiMainWinBody::close()
{
    if ( !handle_.closeOK() ) return; 

    handle_.windowClosed.trigger( handle_ );

    if ( modal_ )
	eventloop_.exit();

    QMainWindow::hide();

    if ( exitapponclose_ )
	qApp->quit();
}


uiStatusBar* uiMainWinBody::uistatusbar()
{ return statusbar; }

uiMenuBar* uiMainWinBody::uimenubar()
{ return menubar; }


void uiMainWinBody::removeDockWin( uiDockWin* dwin )
{
    if ( !dwin ) return;

    removeDockWidget( dwin->qwidget() );
    dockwins_ -= dwin;
}


void uiMainWinBody::addDockWin( uiDockWin& dwin, uiMainWin::Dock dock )
{
    Qt::DockWidgetArea dwa = Qt::LeftDockWidgetArea;
    if ( dock == uiMainWin::Right ) dwa = Qt::RightDockWidgetArea;
    else if ( dock == uiMainWin::Top ) dwa = Qt::TopDockWidgetArea;
    else if ( dock == uiMainWin::Bottom ) dwa = Qt::BottomDockWidgetArea;
    addDockWidget( dwa, dwin.qwidget() );
    if ( dock == uiMainWin::TornOff )
	dwin.setFloating( true );
    dockwins_ += &dwin;
}


void uiMainWinBody::toggleToolbar( CallBacker* cb )
{
    mDynamicCastGet( uiMenuItem*, itm, cb );
    if ( !itm ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	if ( tb.name()==itm->name() )
	    tb.display( tb.isHidden() );
    }
}


void uiMainWinBody::updateToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    const ObjectSet<uiMenuItem>& items = toolbarsmnu_->items();

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiMenuItem& itm = *const_cast<uiMenuItem*>( items[idx] );
	if ( itm.name()==tb.name() )
	    itm.setChecked( !tb.isHidden() );
    }
}


void uiMainWinBody::addToolBar( uiToolBar* tb )
{
    QMainWindow::addToolBar( (Qt::ToolBarArea)tb->prefArea(), tb->qwidget() );
    toolbars_ += tb;
    renewToolbarsMenu();
}


void uiMainWinBody::removeToolBar( uiToolBar* tb )
{
    QMainWindow::removeToolBar( tb->qwidget() );
    toolbars_ -= tb;
    renewToolbarsMenu();
}


void uiMainWinBody::renewToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    toolbarsmnu_->clear();
    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiMenuItem* itm =
	    new uiMenuItem( tb.name(), mCB(this,uiMainWinBody,toggleToolbar) );
	toolbarsmnu_->insertItem( itm );
	itm->setCheckable( true );
	itm->setChecked( !tb.isHidden() );
    }
}


static BufferString getSettingsFileName()
{
    BufferString fnm( "qtsettings_", (int)mODVersion );
    FilePath fp( GetSettingsDir(), fnm );
    const char* swusr = GetSoftwareUser();
    if ( swusr )
	fp.setExtension( swusr );
    return fp.fullPath();
}


void uiMainWinBody::saveSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    settings.setValue( "size", size() );
    settings.setValue( "pos", pos() );
    settings.setValue( "state", saveState() );
    settings.endGroup();
}


void uiMainWinBody::readSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    QSize qsz( settings.value("size",QSize(200,200)).toSize() );
    prefsz_ = uiSize( qsz.width(), qsz.height() );
    QPoint qpt( settings.value("pos",QPoint(200,200)).toPoint() );
    prefpos_.setXY( qpt.x(), qpt.y() );
    restoreState( settings.value("state").toByteArray() );
    settings.endGroup();

    updateToolbarsMenu();
}


void uiMainWinBody::setWindowTitle( const char* txt )
{ QMainWindow::setWindowTitle( uiMainWin::uniqueWinTitle(txt,this) ); }


#define mExecMutex( statements ) \
    activatemutex_.lock(); statements; activatemutex_.unLock();


void uiMainWinBody::activateInGUIThread( const CallBack& cb, bool busywait )
{
    CallBack* actcb = new CallBack( cb );
    mExecMutex( activatecbs_ += actcb );

    QEvent* guithreadev = new QEvent( mUsrEvGuiThread );
    QApplication::postEvent( this, guithreadev );

    float sleeptime = 0.01;
    while ( busywait )
    {
	mExecMutex( const int idx = activatecbs_.indexOf(actcb) );
	if ( idx < 0 )
	    break;

	Threads::sleep( sleeptime ); 
	if ( sleeptime < 1.28 )
	    sleeptime *= 2;
    }
}


void uiMainWinBody::keyPressEvent( QKeyEvent* ev )
{
    OD::KeyboardKey key = OD::KeyboardKey( ev->key() );
    OD::ButtonState modifier = OD::ButtonState( (int)ev->modifiers() );

    if ( ev && ev->key() == Qt::Key_F12 )
	handle_.translate();

    if ( key == OD::C && modifier == OD::ControlButton )
	handle_.ctrlCPressed.trigger();

    return QMainWindow::keyPressEvent( ev );
}


bool uiMainWinBody::event( QEvent* ev )
{
    if ( ev->type() == mUsrEvGuiThread )
    {
	mExecMutex( CallBack* actcb = activatecbs_[nractivated_++] );
	actcb->doCall( this );
	handle_.activatedone.trigger( actcb->cbObj() );
	mExecMutex( activatecbs_ -= actcb; nractivated_-- );
	delete actcb;
    }
    else if ( ev->type() == mUsrEvPopUpReady )
    {
	handle_.endCmdRecEvent( eventrefnr_, "WinPopUp" );
    }
    else
	return QMainWindow::event( ev );
    
    return true; 
}


void uiMainWinBody::managePopupPos()
{
    uiParent* myparent = handle_.parent();
    uiMainWin* myparentsmw = myparent ? myparent->mainwin() : 0;
    if ( myparentsmw && !myparentsmw->isHidden() )
	return;

    uiMainWin* parwin = handle_.programmedActiveWindow();
    while ( parwin && parwin->isHidden() )
	parwin = parwin->parent() ? parwin->parent()->mainwin() : 0;

    if ( !parwin || moved_ )
	return;

    const uiRect pwrect = parwin->geometry( false );
    handle_.setCornerPos( pwrect.get(uiRect::Left), pwrect.get(uiRect::Top) );
    moved_ = false;
}


// ----- uiMainWin -----


uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(setup.caption_,0)
    , body_(0)
    , parent_(p)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , caption_(setup.caption_)
{ 
    body_ = new uiMainWinBody( *this, p, setup.caption_, setup.modal_ ); 
    setBody( body_ );
    body_->construct( setup.nrstatusflds_, setup.withmenubar_ );
    body_->setWindowIconText(
	    setup.caption_.isEmpty() ? "OpendTect" : setup.caption_.buf() );
    body_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoard) );
}


uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(nm,0)
    , body_(0)
    , parent_(parnt)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , caption_(nm)
{ 
    body_ = new uiMainWinBody( *this, parnt, nm, modal ); 
    setBody( body_ );
    body_->construct( nrstatusflds, withmenubar );
    body_->setWindowIconText( nm && *nm ? nm : "OpendTect" );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoard) );
}


uiMainWin::uiMainWin( const char* nm, uiParent* parnt )
    : uiParent(nm,0)
    , body_(0)			
    , parent_(parnt)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , caption_(nm)
{
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoard) );
}


static Threads::Mutex		winlistmutex_;
static ObjectSet<uiMainWin>	orderedwinlist_;
static uiMainWin*		programmedactivewin_ = 0;

uiMainWin::~uiMainWin()
{
    if ( !body_->deletefrombody_ )
    {
	body_->deletefromod_ = true;
	delete body_;
    }

    winlistmutex_.lock();
    orderedwinlist_ -= this;

    if ( programmedactivewin_ == this )
	programmedactivewin_ = parent() ? parent()->mainwin() : 0;

    winlistmutex_.unLock();
}


QWidget* uiMainWin::qWidget() const
{ return body_; }

void uiMainWin::provideHelp( const char* winid )
{
    const BufferString fnm = HelpViewer::getURLForWinID( winid );
    static bool shwonly = GetEnvVarYN("DTECT_SHOW_HELPINFO_ONLY");
    if ( shwonly ) return;

    BufferString browser;
    Settings::common().get( "dTect.Browser", browser );
    if ( browser.isEmpty() )
	uiDesktopServices::openUrl( fnm );
    else
    {
	BufferString cmd( browser, " ", fnm );
	ExecOSCmd( cmd, true );
    }
}


void uiMainWin::showCredits( const char* winid )
{
    const BufferString fnm = HelpViewer::getCreditsURLForWinID( winid );
    uiDesktopServices::openUrl( fnm );
}

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }


#define mAddToOrderedWinList( uimw ) \
    winlistmutex_.lock(); \
    orderedwinlist_ -= uimw; \
    orderedwinlist_ += uimw; \
    winlistmutex_.unLock();

void uiMainWin::show()
{
    mAddToOrderedWinList( this );
    body_->go();
}


void uiMainWin::close()				{ body_->close(); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::touch() 			{ return body_->touch(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ body_->showMaximized(); }
void uiMainWin::showMinimized()			{ body_->showMinimized(); }
void uiMainWin::showNormal()			{ body_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return body_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return body_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }

void uiMainWin::setCaption( const char* txt )
{
    caption_ = txt;
    body_->setWindowTitle( txt );
}


const char* uiMainWin::caption( bool unique ) const
{
    static BufferString capt;
    capt = unique ? mQStringToConstChar(body_->windowTitle()) : caption_.buf();
    return capt;
}


void uiMainWin::setDeleteOnClose( bool yn )
{ body_->setAttribute( Qt::WA_DeleteOnClose, yn ); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ body_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ body_->addDockWin( dwin, d ); }


void uiMainWin::addToolBar( uiToolBar* tb )
{ body_->addToolBar( tb ); }


void uiMainWin::removeToolBar( uiToolBar* tb )
{ body_->removeToolBar( tb ); }


void uiMainWin::addToolBarBreak()
{ body_->addToolBarBreak(); } 

uiPopupMenu& uiMainWin::getToolbarsMenu() const
{ return body_->getToolbarsMenu(); }


void uiMainWin::updateToolbarsMenu()
{ body_->updateToolbarsMenu(); }


const ObjectSet<uiToolBar>& uiMainWin::toolBars() const
{ return body_->toolBars(); } 
    

const ObjectSet<uiDockWin>& uiMainWin::dockWins() const
{ return body_->dockWins(); } 


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


void uiMainWin::setSensitive( bool yn )
{
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


void uiMainWin::setCornerPos( int x, int y )
{ body_->move( x, y ); }


uiRect uiMainWin::geometry( bool frame ) const
{
    // Workaround for Qt-bug: top left of area sometimes translates to origin!
    QRect qarea = body_->geometry();
    QRect qframe = body_->frameGeometry();
    QPoint correction = body_->mapToGlobal(QPoint(0,0)) - qarea.topLeft();
    qframe.translate( correction );
    qarea.translate( correction ); 
    QRect qrect = frame ? qframe : qarea;

    //QRect qrect = frame ? body_->frameGeometry() : body_->geometry();
    uiRect rect( qrect.left(), qrect.top(), qrect.right(), qrect.bottom() );
    return rect;
}


void uiMainWin::setIcon( const ioPixmap& pm )
{ body_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIconText( const char* txt)
{ body_->setWindowIconText( txt ); }

void uiMainWin::saveSettings()
{ body_->saveSettings(); }

void uiMainWin::readSettings()
{ body_->readSettings(); }

void uiMainWin::raise()
{ body_->raise(); }


void uiMainWin::programActiveWindow( uiMainWin* mw )
{ programmedactivewin_ = mw; }


uiMainWin* uiMainWin::programmedActiveWindow()
{ return programmedactivewin_; }

uiMainWin* uiMainWin::activeWindow()
{
    if ( programmedactivewin_ )
	return programmedactivewin_;

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


const char* uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )
	return 0;

    static BufferString title;
    title = mQStringToConstChar( amw->windowTitle() );
    return title;
}


#define mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut ) \
\
    int stdbutcount = 0; \
    QMessageBox::StandardButton stdbut = QMessageBox::NoButton; \
    for ( unsigned int idx=QMessageBox::Ok; \
	  qmb && idx<=QMessageBox::RestoreDefaults; idx+=idx ) \
    { \
	const QAbstractButton* abstrbut; \
        abstrbut = qmb->button( (QMessageBox::StandardButton) idx ); \
	if ( !abstrbut ) \
	    continue; \
	if ( stdbutcount == buttonnr ) \
	    stdbut = (QMessageBox::StandardButton) idx; \
	stdbutcount++; \
    }

// buttons() function to get all buttons only available from Qt4.5 :-(

const char* uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw ); 
	mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut );

	static BufferString buttext;
        if ( stdbut )
	    buttext = mQStringToConstChar( qmb->button(stdbut)->text() );
	else if ( !stdbutcount )
	    buttext = mQStringToConstChar( qmb->buttonText(buttonnr) );
	else 
	    buttext = "";

	return buttext;
    }

    if ( typ==Colour || typ==Font || typ==File )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 ) return typ==File ? "Ok" : "OK";
	return "";
    }

    return 0;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    QWidget* amw = qApp->activeModalWidget();
    const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw ); 
    mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut );

    return stdbut ? ((int) stdbut) : buttonnr;
}


void uiMainWin::closeActiveModalQDlg( int retval )
{
    if ( activeModalWindow() )
	return;

    QWidget* _amw = qApp->activeModalWidget();
    if ( !_amw ) 
	return;

    QDialog* _qdlg = dynamic_cast<QDialog*>(_amw);
    if ( !_qdlg ) 
	return;

    _qdlg->done( retval );
}


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& windowlist,
				    bool visibleonly )
{
    windowlist.erase();
    winlistmutex_.lock();
    for ( int idx=0; idx<orderedwinlist_.size(); idx++ )
    {
	if ( !visibleonly || !orderedwinlist_[idx]->isHidden() )
	    windowlist += orderedwinlist_[idx];
    }
    winlistmutex_.unLock();
}


void uiMainWin::getModalSignatures( BufferStringSet& signatures )
{
    signatures.erase();
    QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int idx=0; idx<toplevelwigs.count(); idx++ )
    {
	const QWidget* qw = toplevelwigs.at( idx );
	if ( qw->isWindow() && !qw->isHidden() && qw->isModal() )
	{
	    BufferString qwptrstr;
	    sprintf( qwptrstr.buf(), "%p", qw );
	    signatures.add( qwptrstr );
	}
    }
}


const char* uiMainWin::uniqueWinTitle( const char* txt, QWidget* forwindow )
{
    static BufferString wintitle;
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int count=1; true; count++ )
    {
	bool unique = true;
	wintitle = txt;
	if ( wintitle.isEmpty() )
	    wintitle = "<no title>";

	if ( count>1 )
	{
	    wintitle += "  {"; wintitle += count ; wintitle += "}" ;
	}

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( wintitle==mQStringToConstChar(qw->windowTitle())  )
		unique = false;
	}

	if ( unique ) break;
    }
    return wintitle;
}


bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
    const WId desktopwinid = QApplication::desktop()->winId();
    const QPixmap desktopsnapshot = QPixmap::grabWindow( desktopwinid );

    QPixmap snapshot = desktopsnapshot;
    if ( zoom > 0 )
    {
	QWidget* qwin = qApp->activeModalWidget();
	if ( !qwin || zoom==1 )
	    qwin = body_;

	const int width = qwin->frameGeometry().width();
	const int height = qwin->frameGeometry().height();
	snapshot = desktopsnapshot.copy( qwin->x(), qwin->y(), width, height );
    }

    return snapshot.save( QString(filenm), format, quality );
}


void uiMainWin::activateInGUIThread( const CallBack& cb, bool busywait )
{ body_->activateInGUIThread( cb, busywait ); }


static void doTranslate( const uiBaseObject* obj )
{
    uiBaseObject* baseobj = const_cast<uiBaseObject*>( obj );
    mDynamicCastGet(uiObject*,uiobj,baseobj);
    mDynamicCastGet(uiGroupObj*,uigrpobj,baseobj);
    if ( !uigrpobj && uiobj ) uiobj->translate();

    if ( uigrpobj )
    {
	const ObjectSet<uiBaseObject>* children = uigrpobj->childList();
	for ( int idx=0; idx<children->size(); idx++ )
	    doTranslate( (*children)[idx] );
    }

    mDynamicCastGet(uiParent*,uipar,baseobj);
    if ( !uipar ) return;

    const ObjectSet<uiBaseObject>* children = uipar->childList();
    for ( int idx=0; idx<children->size(); idx++ )
	doTranslate( (*children)[idx] );
}


void uiMainWin::translate()
{
    doTranslate( body_->centralWidget_ );

    for ( int idx=0; idx<body_->toolbars_.size(); idx++ )
    {
	const ObjectSet<uiObject>& objs = body_->toolbars_[idx]->objectList();
	for ( int idy=0; idy<objs.size(); idy++ )
	    doTranslate( objs[idy] );
    }

    for ( int idx=0; idx<body_->dockwins_.size(); idx++ )
	doTranslate( body_->dockwins_[idx] );

    if ( menuBar() )
	menuBar()->translate();
}


void uiMainWin::copyToClipBoard( CallBacker* )
{
    const WId desktopwinid = QApplication::desktop()->winId();
    const QPixmap desktopsnapshot = QPixmap::grabWindow( desktopwinid );

    QWidget* qwin = qApp->activeModalWidget();
    if ( !qwin )
	qwin = body_;

    const int width = qwin->frameGeometry().width();
    const int height = qwin->frameGeometry().height();
    QPixmap snapshot = desktopsnapshot.copy(qwin->x(),qwin->y(),width,height);
    QImage image = snapshot.toImage();

    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setImage( image );
}


/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/

int uiDialog::titlepos_ = 0; // default is centered.
int uiDialog::titlePos()
{ return titlepos_; }

void uiDialog::setTitlePos( int p )
{ titlepos_ = p; }

#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public uiMainWinBody
{ 	
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);
			~uiDialogBody();

    int			exec( bool showminimized ); 

    void		reject( CallBacker* s )	
			{
			    mHandle.cancelpushed_ = s == cnclbut;
			    if ( mHandle.rejectOK(s) )
				done_(0);
			    else
				uiSetResult( -1 );
			}
                        //!< to be called by a 'cancel' button
    void		accept( CallBacker* s )	
			    { if ( mHandle.acceptOK(s) ) done_(1); }
                        //!< to be called by a 'ok' button
    void		done( int i )
			    { if ( mHandle.doneOK(i) ) done_(i); }

    void		uiSetResult( int v )	{ result_ = v; }
    int			uiResult()		{ return result_; }

    void		setTitleText(const char* txt);
    void		setOkText(const char*);
			//!< OK button disabled when set to empty
    void		setCancelText(const char*);
			//!< Cancel button disabled when set to empty
    void		enableSaveButton(const char* txt);
    void		setSaveButtonChecked(bool yn);
    void		setButtonSensitive(uiDialog::Button,bool yn);
    bool		saveButtonChecked() const;
    bool		hasSaveButton() const;
    uiButton*		button(uiDialog::Button);

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )	{ setup.separator_ = yn; }
    bool		separator() const	{ return setup.separator_; }
    void		setHelpID( const char* id ) { setup.helpid_ = id; }
    const char*		helpID() const		{ return setup.helpid_; }

    void		setDlgGrp( uiGroup* cw )	{ dlgGroup=cw; }

    void		setHSpacing( int spc )	{ dlgGroup->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlgGroup->setVSpacing(spc); }
    void		setBorder( int b )	{ dlgGroup->setBorder( b ); }

    virtual void        addChild(uiBaseObject& child);
    virtual void        manageChld_(uiBaseObject&,uiObjectBody&);
    virtual void  	attachChild(constraintType,uiObject* child,
				    uiObject* other,int margin,bool reciprocal);
    void		provideHelp(CallBacker*);
    void		showCredits(CallBacker*);
    void		doTranslate(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup; }

protected:

    virtual const QWidget* managewidg_() const 
			{ 
			    if ( !initing ) 
				return dlgGroup->pbody()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int 		result_;
    bool		childrenInited;

    uiGroup*            dlgGroup;
    uiDialog::Setup	setup;

    uiPushButton*	okbut;
    uiPushButton*	cnclbut;
    uiToolButton*	helpbut;
    uiToolButton*	creditsbut;
    uiToolButton*	translatebut;

    uiCheckBox*		savebut_cb;
    uiToolButton*	savebut_tb;

    uiSeparator*	horSepar;
    uiLabel*		title;

    void		done_(int);

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool);
    void		closeEvent(QCloseEvent*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);

};


uiDialogBody::uiDialogBody( uiDialog& hndle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(hndle,parnt,s.wintitle_,s.modal_)
    , dlgGroup(0)
    , setup(s)
    , okbut(0), cnclbut(0), savebut_cb(0),  savebut_tb(0)
    , helpbut(0), creditsbut(0), translatebut(0)
    , title(0), result_(0)
    , childrenInited(false)
{
    setContentsMargins( 10, 2, 10, 2 );
}



uiDialogBody::~uiDialogBody()
{
    if ( okbut )
	okbut->activated.remove( mCB(this,uiDialogBody,accept) );

    if ( cnclbut )
	cnclbut->activated.remove( mCB(this,uiDialogBody,reject) );
}


int uiDialogBody::exec( bool showminimized )
{ 
    uiSetResult( 0 );

    if ( setup.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go( showminimized );

    return uiResult();
}



void uiDialogBody::setOkText( const char* txt )    
{ 
    setup.oktext_ = txt; 
    if ( okbut ) okbut->setText(txt);
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
    if ( cnclbut ) cnclbut->setText(txt);
}


bool uiDialogBody::hasSaveButton() const
{
    return savebut_cb;
}


bool uiDialogBody::saveButtonChecked() const
{ 
    return savebut_cb ? savebut_cb->isChecked() : false;
}


/*! Hides the box, which also exits the event loop in case of a modal box.  */
void uiDialogBody::done_( int v )
{
    uiSetResult( v );
    close();
}


void uiDialogBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    reject(0);
    if ( result_ == -1 )
	ce->ignore();
    else
	ce->accept();

    handle_.endCmdRecEvent( refnr, "Close" );
}


void uiDialogBody::enableSaveButton( const char* txt )
{ setup.savetext_ = txt; setup.savebutton_ = true; }

void uiDialogBody::setSaveButtonChecked( bool yn )
{
    setup.savechecked_ = yn;
    if ( savebut_cb ) savebut_cb->setChecked(yn);
}


void uiDialogBody::setButtonSensitive( uiDialog::Button but, bool yn )
{ 
    switch ( but )
    {
    case uiDialog::OK:		if ( okbut ) okbut->setSensitive(yn); 
    break;
    case uiDialog::CANCEL:	if ( cnclbut ) cnclbut->setSensitive(yn); 
    break;
    case uiDialog::SAVE: 
	if ( savebut_cb ) savebut_cb->setSensitive(yn); 
	if ( savebut_tb ) savebut_tb->setSensitive(yn); 
    break;
    case uiDialog::HELP:	if ( helpbut ) helpbut->setSensitive(yn); 
    break;
    case uiDialog::CREDITS:	if ( creditsbut ) creditsbut->setSensitive(yn); 
    break;
    case uiDialog::TRANSLATE:
	if ( translatebut ) translatebut->setSensitive(yn);
    break;
    }
}


uiButton* uiDialogBody::button( uiDialog::Button but ) 
{ 
    switch ( but )
    {
    case uiDialog::OK:		return okbut; break;
    case uiDialog::CANCEL:	return cnclbut; break;
    case uiDialog::SAVE: 
	return savebut_cb
	    ? (uiButton*)savebut_cb : (uiButton*)savebut_tb;
    break;
    case uiDialog::HELP:	return helpbut; break;
    case uiDialog::CREDITS:	return creditsbut; break;
    case uiDialog::TRANSLATE:	return translatebut; break;
    }

    return 0;
}


void uiDialogBody::addChild( uiBaseObject& child )
{ 
    if ( !initing ) 
	dlgGroup->addChild( child );
    else
	uiMainWinBody::addChild( child );
}


void uiDialogBody::manageChld_( uiBaseObject& o, uiObjectBody& b )
{ 
    if ( !initing ) 
	dlgGroup->manageChld( o, b );
}


void uiDialogBody::attachChild( constraintType tp, uiObject* child,
				uiObject* other, int margin, bool reciprocal )
{
    if ( !child || initing ) return;

    dlgGroup->attachChild( tp, child, other, margin, reciprocal ); 
}


/*!
    Construct OK and Cancel buttons just before the first show.
    This gives chance not to construct them in case OKtext and CancelText have
    been set to ""
*/
void uiDialogBody::finalise( bool ) 
{
    uiMainWinBody::finalise( false ); 

    handle_.preFinalise().trigger( handle_ );

    dlgGroup->finalise();

    if ( !childrenInited ) 
	initChildren();

    finaliseChildren();

    handle_.postFinalise().trigger( handle_ );
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
    layoutChildren( lowestobject );

    if ( okbut )
    {
	okbut->activated.notify( mCB(this,uiDialogBody,accept) );
	okbut->setDefault();
    }
    if ( cnclbut )
    {
	cnclbut->activated.notify( mCB(this,uiDialogBody,reject) );
	if ( !okbut )
	    cnclbut->setDefault();
    }

    childrenInited = true;
}


uiObject* uiDialogBody::createChildren()
{
    if ( !setup.oktext_.isEmpty() )
	okbut = new uiPushButton( centralWidget_, setup.oktext_, true );
    if ( !setup.canceltext_.isEmpty() )
	cnclbut = new uiPushButton( centralWidget_, setup.canceltext_, true );

    if ( setup.savebutton_ && !setup.savetext_.isEmpty() )
    {
	if ( setup.savebutispush_ )
	    savebut_tb = new uiToolButton( centralWidget_, "save.png",
			  setup.savetext_, CallBack() );
	else
	{
	    savebut_cb = new uiCheckBox( centralWidget_, setup.savetext_ );
	    savebut_cb->setChecked( setup.savechecked_ );
	}
    }
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    const BufferString hid( dlg.helpID() );
    if ( !hid.isEmpty() && hid != "-" )
    {
	static bool shwhid = GetEnvVarYN( "DTECT_SHOW_HELP" );
#ifdef __debug__
	shwhid = true;
#endif
	helpbut = new uiToolButton( centralWidget_, "contexthelp.png",
			shwhid ? hid.buf() : "Help on this window",
	       		mCB(this,uiDialogBody,provideHelp) );
	helpbut->setPrefWidthInChar( 5 );
	if ( TrMgr().tr() && TrMgr().tr()->enabled() )
	{
	    translatebut = new uiToolButton( centralWidget_,
		TrMgr().tr()->getIcon(),
		"Translate", mCB(this,uiDialogBody,doTranslate) );
	    translatebut->attach( rightOf, helpbut );
	}
	if ( dlg.haveCredits() )
	{
	    const ioPixmap pixmap( "credits.png" );
	    creditsbut = new uiToolButton( centralWidget_, "credits.png",
		    "Show credits", mCB(this,uiDialogBody,showCredits) );
	    creditsbut->setPrefWidthInChar( 5 );
	    creditsbut->attach( rightOf, translatebut ? translatebut : helpbut );
	}
    }

    if ( !setup.menubar_ && !setup.dlgtitle_.isEmpty() )
    {
	title = new uiLabel( centralWidget_, setup.dlgtitle_ );

	uiObject* obj = setup.separator_ 
			    ? (uiObject*) new uiSeparator(centralWidget_)
			    : (uiObject*) title;

	if ( obj != title )
	{
	    if ( uiDialog::titlePos() == 0 )
		title->attach( centeredAbove, obj );
	    else if ( uiDialog::titlePos() > 0 )
		title->attach( rightBorder );
	    obj->attach( stretchedBelow, title, -2 );
	}
	if ( setup.mainwidgcentered_ )
	    dlgGroup->attach( centeredBelow, obj );
	else
	    dlgGroup->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlgGroup->mainObject();
    if ( setup.separator_ && ( okbut || cnclbut || savebut_cb || 
			       savebut_tb || helpbut) )
    {
	horSepar = new uiSeparator( centralWidget_ );
	horSepar->attach( stretchedBelow, dlgGroup, -2 );
	lowestobj = horSepar;
    }

    return lowestobj;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = setup.okcancelrev_ ? cnclbut : okbut;
    uiObject* rightbut = setup.okcancelrev_ ? okbut : cnclbut;
    uiObject* exitbut = okbut ? okbut : cnclbut;
    uiObject* centerbut = helpbut;
    uiObject* extrabut = savebut_tb;

    if ( !okbut || !cnclbut )
    {
	leftbut = rightbut = 0;
	if ( exitbut )
	{
	    centerbut = exitbut;
	    extrabut = helpbut;
	    leftbut = savebut_tb;
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

    if ( savebut_cb )
    {
	savebut_cb->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, savebut_cb );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, savebut_cb );
    }

    if ( extrabut )
	extrabut->attach( rightOf, centerbut );
}


void uiDialogBody::provideHelp( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    uiMainWin::provideHelp( dlg.helpID() );
}


void uiDialogBody::doTranslate( CallBacker* )
{
    setButtonSensitive( uiDialog::TRANSLATE, false );
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    dlg.translate();
    setButtonSensitive( uiDialog::TRANSLATE, true );
}


void uiDialogBody::showCredits( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    uiMainWin::showCredits( dlg.helpID() );
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
    ctrlstyle_ = DoAndLeave;
}


void uiDialog::setButtonText( Button but, const char* txt )
{
    switch ( but )
    {
        case OK	: setOkText( txt ); break;
        case CANCEL	: setCancelText( txt ); break;
        case SAVE	: enableSaveButton( txt ); break;
        case HELP	: pErrMsg("set help txt but"); break;
        case CREDITS: pErrMsg("set credits txt but");
        case TRANSLATE: pErrMsg("set transl txt but");
    }
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
    case DoAndProceed:
	setOkText( "&Proceed" );
	setCancelText( "&Dismiss" );
    break;
    }

    ctrlstyle_ = cs;
}


void uiDialog::showMinMaxButtons()
{
    Qt::WindowFlags flags = body_->windowFlags();
    flags |= Qt::WindowMinMaxButtonsHint;
    body_->setWindowFlags( flags );
}


void uiDialog::showAlwaysOnTop()
{
    Qt::WindowFlags flags = body_->windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    body_->setWindowFlags( flags );
}


bool uiDialog::haveCredits() const
{
    return HelpViewer::hasSpecificCredits( helpID() );
}


int uiDialog::go()
{ 
    mAddToOrderedWinList( this );
    return mBody->exec( false );
}


int uiDialog::goMinimized()
{ 
    mAddToOrderedWinList( this );
    return mBody->exec( true );
}


const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( int i )			{ mBody->done( i ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
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
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setButtonSensitive(uiDialog::Button b, bool s ) 
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b) 
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }
void uiDialog::setCaption( const char* txt )
    { caption_ = txt; mBody->setWindowTitle( txt ); }
