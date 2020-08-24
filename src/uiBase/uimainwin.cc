/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uidialog.h"

#include "uibody.h"
#include "uiclipboard.h"
#include "uidesktopservices.h"
#include "uidockwin.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "keyboardevent.h"
#include "msgh.h"
#include "oddirs.h"
#include "odver.h"
#include "settings.h"
#include "texttranslation.h"
#include "threadlock.h"
#include "timer.h"

#include "q_uiimpl.h"

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
#include <QPainter>
#include <QPrinter>
#include <QScreen>
#include <QSettings>
#include <QStatusBar>
#include <QWidget>
#include <QWindow>

#ifdef __win__
# include <QtPlatformHeaders/QWindowsWindowFunctions>
#endif

mUseQtnamespace

ObjectSet<uiMainWin>	uiMainWin::allwins_;
static Threads::Lock	winlistlock_;
static uiMainWin*	programmedactivewin_ = 0;



class uiMainWinBody : public uiCentralWidgetBody , public QMainWindow
{ mODTextTranslationClass(uiMainWinBody)
friend class		uiMainWin;
public:
			uiMainWinBody(uiMainWin& handle,uiParent* parnt,
				      const char* nm,bool modal);
    virtual		~uiMainWinBody();

    void		construct(int nrstatusflds,bool wantmenubar);

public:

    uiStatusBar*	uistatusbar()			{ return statusbar_; }
    uiMenuBar*		uimenubar()			{ return menubar_; }

    virtual void	polish();
    void		reDraw(bool deep);
    void		go(bool showminimized=false);
    virtual void	show()				{ doShow(); }
    void		doSetWindowFlags(Qt::WindowFlags,bool yn);

    void		move(uiMainWin::PopupArea);
    void		move(int,int);

    bool		handleAlive() const;
    void		close();
    bool		poppedUp() const		{ return poppedup_; }
    bool		resetPopupTimerIfNotPoppedUp();

    void		removeDockWin(uiDockWin*);
    void		addDockWin(uiDockWin&,uiMainWin::Dock);

    virtual QMenu*	createPopupMenu();
    void		addToolBar(uiToolBar*);
    uiToolBar*		findToolBar(const char*);
    uiToolBar*		removeToolBar(uiToolBar*);
    uiMenu&		getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const	{ return toolbars_; }
    const ObjectSet<uiDockWin>& dockWins() const	{ return dockwins_; }

    void		setModal(bool yn);
    bool		isModal() const			{ return modal_; }

    void		activateInGUIThread(const CallBack&,bool busywait);

    bool		force_finalise_;

    static QScreen*	primaryScreen();
    QScreen*		screen(bool usetoplevel=false) const;

protected:

    virtual const QWidget*	qwidget_() const { return this; }

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

    uiStatusBar*	statusbar_;
    uiMenuBar*		menubar_;
    uiMenu*		toolbarsmnu_;

    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiDockWin> dockwins_;
    uiString		windowtitle_;
    uiMainWin&		handle_;

private:

    QEventLoop		eventloop_;

    bool		modal_;

    void		popTimTick(CallBacker*);
    void		getPosForScreenMiddle(int& x,int& y);
    void		getPosForParentMiddle(int& x,int& y);
    Timer		poptimer_;
    bool		poppedup_;
    uiSize		prefsz_;
    uiPoint		prefpos_;
    bool		moved_;
    bool		createtbmenu_;

    bool		deletefrombody_;
    bool		deletefromod_;

    bool		hasguisettings_;
};


#define mParent p && p->pbody() ? p->pbody()->qwidget() : 0
uiMainWinBody::uiMainWinBody( uiMainWin& uimw, uiParent* p,
			      const char* nm, bool modal )
	: uiCentralWidgetBody(nm)
	, QMainWindow(mParent)
	, handle_(uimw)
	, statusbar_(0)
	, menubar_(0)
	, toolbarsmnu_(0)
	, modal_(p && modal)
	, poptimer_("Popup timer")
	, poppedup_(false)
	, exitapponclose_(false)
	, prefsz_(-1,-1)
	, prefpos_(uiPoint::udf())
	, nractivated_(0)
	, moved_(false)
	, createtbmenu_(false)
	, hasguisettings_(false)
	, force_finalise_(false)
	, deletefrombody_(false)
	, deletefromod_(false)
{
    if ( nm && *nm )
	setObjectName( nm );

    const int sz = uiObject::toolButtonSize();
    setIconSize( QSize(sz,sz) );
    setWindowModality( p && modal ? Qt::WindowModal
				  : Qt::NonModal );

#ifdef __mac__
    setUnifiedTitleAndToolBarOnMac( true );
#endif

    setDockOptions( VerticalTabs | AnimatedDocks );

    poptimer_.tick.notify( mCB(this,uiMainWinBody,popTimTick) );
}


uiMainWinBody::~uiMainWinBody()
{
    deleteAllChildren(); //delete them now to make sure all ui objects
			 //are deleted before their body counterparts

    if ( toolbarsmnu_ )
    {
	toolbarsmnu_->clear();
	if ( toolbarsmnu_->isStandAlone() )
	    delete toolbarsmnu_;
    }

    if ( !deletefromod_ )
    {
	deletefrombody_ = true;
	delete &handle_;
    }

    delete statusbar_;
    delete menubar_;
}


void uiMainWinBody::doSetWindowFlags( Qt::WindowFlags todoflag, bool yn )
{
    const Qt::WindowFlags flags = windowFlags();
    if ( yn )
	setWindowFlags( flags | todoflag );
    else
    {
	const od_uint32 newflagsi = (od_uint32)flags - (od_uint32)todoflag;
	const Qt::WindowFlags newflags( newflagsi );
	setWindowFlags( newflags );
    }
}


void uiMainWinBody::setModal( bool yn )
{
    modal_ = yn;
    setWindowModality( yn ? Qt::WindowModal
			  : Qt::NonModal );
}


void uiMainWinBody::doShow( bool minimized )
{
    if ( !finalised() || force_finalise_ )
	finalise( true );

    handle_.updateCaption();
    eventrefnr_ = handle_.beginCmdRecEvent("WinPopUp");
    managePopupPos();

    if ( minimized )
    {
	handle_.windowHidden.trigger( handle_ );
	QMainWindow::showMinimized();
    }
    else
    {
	if ( isMinimized() ) showNormal();
	raise();
	handle_.windowShown.trigger( handle_ );
	QMainWindow::show();
    }

    if( poptimer_.isActive() )
	poptimer_.stop();

    poppedup_ = false;
    poptimer_.start( 100, true );

    QEvent* ev = new QEvent( mUsrEvPopUpReady );
    QApplication::postEvent( this, ev );

#ifdef __debug__

/*
 We need a check on windows being too big for little (laptop) screens.
 But if we set the margins too tight we'll get a hit for many windows.
 Then we (programmers) will start ignoring pErrMsg's, which is _really_ bad.
 Notes:
 * It seems that windows can be a bit bigger than the screen.
 * Remember the actual size is dep on font size.
 * I asked Farrukh to come up with some data on our laptops, most notably the
   ones used for the courses, but it's inconclusive.

 The issue is the tension between: what would we like to support vs the ease
 of build and - last but not least - the convenience for the user to have
 a lot of info and tools on a single window.

 In any case: recurring pErrMsg's are *BAD*. They should never be ignored.
 Which means they have to indicate serious problems, not matters of taste.

*/

#   define mMinSupportedWidth 1920
#   define mMinSupportedHeight 1080

    QRect qrect = geometry();
    if ( !hasguisettings_ && qrect.height() > mMinSupportedHeight )
    {
	BufferString msg( "The window '", name(), "' is " );
	msg.add( qrect.width() ).add( "x" ).add( qrect.height() )
	    .add( ". That height may be a problem on many laptops.\nWe want to "
		    "support heights >= " ).add( mMinSupportedHeight )
	    .add( "and if possible keep width below " ).add(mMinSupportedWidth)
	    .add( ".\nSee comments in the .cc file." );
	pErrMsg( msg );
    }

#endif

    if ( !handle_.afterPopup.isEmpty() )
    {
	handle_.afterpopuptimer_ = new Timer( "After popup timer" );
	handle_.afterpopuptimer_->tick.notify(
				mCB(&handle_,uiMainWin,aftPopupCB) );
	handle_.afterpopuptimer_->start( 50, true );
    }

    move( handle_.popuparea_ );
    raise();

    if ( modal_ )
	eventloop_.exec();
}


void uiMainWinBody::construct( int nrstatusflds, bool wantmenubar )
{
    centralwidget_ = new uiGroup( &handle_, "OpendTect Main Window" );
    setCentralWidget( centralwidget_->body()->qwidget() );

    centralwidget_->setIsMain(true);
    centralwidget_->setBorder(10);
    centralwidget_->setStretch(2,2);

    if ( nrstatusflds != 0 )
    {
	QStatusBar* mbar= statusBar();
	if ( mbar )
	    statusbar_ = new uiStatusBar( &handle_,
					  "MainWindow StatusBar handle", *mbar);
	else
	    { pErrMsg("No statusbar returned from Qt"); }

	if ( nrstatusflds > 0 )
	{
	    for( int idx=0; idx<nrstatusflds; idx++ )
		statusbar_->addMsgFld();
	}
    }
    if ( wantmenubar )
    {
	QMenuBar* qmenubar = menuBar();
	if ( qmenubar )
	    menubar_ = new uiMenuBar( &handle_, "MenuBar", qmenubar );
	else
	    { pErrMsg("No menubar returned from Qt"); }

	toolbarsmnu_ = new uiMenu( &handle_, uiStrings::sToolBar(mPlural) );
    }

    initing_ = false;
}


void uiMainWinBody::getPosForScreenMiddle( int& xpos, int& ypos )
{
    const QScreen* primscreen = primaryScreen();
    if ( !primscreen ) return;
    const QRect geom = primscreen->availableGeometry();
    const int screenwidth = geom.width();
    const int screenheight = geom.height();
    const int mywidth = QMainWindow::width();
    const int myheight = QMainWindow::height();

    xpos = (screenwidth - mywidth)/2;
    ypos = (screenheight - myheight)/2;
}


static QWidget* getParentWidget( QWidget* qw )
{
    while ( qw && !qw->isWindow() )
	qw = qw->parentWidget();

    return qw;
}


QScreen* uiMainWinBody::screen( bool usetoplevel ) const
{
    QScreen* qscreen = primaryScreen();
    QWidget* parentwidget = getParentWidget( parentWidget() );
    if ( !parentwidget && usetoplevel )
    {
	uiMainWin* toplevel = uiMain::theMain().topLevel();
	if ( toplevel )
	    parentwidget = toplevel->getWidget(0);
    }

    if ( parentwidget )
    {
	const QWindow* qwindow = parentwidget->windowHandle();
	if ( qwindow )
	{
	    QScreen* winqscreen = qwindow->screen();
	    if ( winqscreen )
		qscreen = winqscreen;
	}
    }

    return qscreen;
}


QScreen* uiMainWinBody::primaryScreen()
{
    return QGuiApplication::primaryScreen();
}


void uiMainWinBody::getPosForParentMiddle( int& xpos, int& ypos )
{
    QWidget* parentwidget = getParentWidget( parentWidget() );
    if ( !parentwidget )
    {
	getPosForScreenMiddle( xpos, ypos );
	return;
    }

    const QScreen* qscreen = screen( false );
    if ( !qscreen ) return;
    const QRect screenrect = qscreen->availableGeometry();
    const int mywidth = frameGeometry().width();
    const int myheight = frameGeometry().height();
    const QPoint parentcenter = parentwidget->frameGeometry().center();
    xpos = parentcenter.x() - mywidth/2;
    ypos = parentcenter.y() - myheight/2;
    if ( xpos<screenrect.left() ) xpos = screenrect.left();
    if ( ypos<screenrect.top() ) ypos = screenrect.top();
    if ( xpos+mywidth > screenrect.right() )
	xpos = screenrect.right() - mywidth;
    if ( ypos+myheight > screenrect.bottom() )
	ypos = screenrect.bottom() - myheight;
}


void uiMainWinBody::move( uiMainWin::PopupArea pa )
{
    const QScreen* qscreen = screen( true );
    if ( !qscreen ) return;
    const QRect screenrect = qscreen->availableGeometry();
    const int mywidth = frameGeometry().width();
    const int myheight = frameGeometry().height();
    int xpos = 0, ypos = 0;
    switch( pa )
    {
	case uiMainWin::TopLeft :
	    move( screenrect.left(), screenrect.top() ); break;
	case uiMainWin::TopRight :
	    move( screenrect.left()-mywidth, screenrect.top() ); break;
	case uiMainWin::BottomLeft :
	    move( screenrect.left(), screenrect.bottom()-myheight ); break;
	case uiMainWin::BottomRight :
	    move( screenrect.right()-mywidth, screenrect.bottom()-myheight );
	    break;
	case uiMainWin::Middle :
	    getPosForParentMiddle( xpos, ypos ); move( xpos, ypos ); break;
	case uiMainWin::Auto :
	    getPosForScreenMiddle( xpos, ypos ); move( xpos, ypos ); break;
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
    centralwidget_->reDraw( deep );
}


void uiMainWinBody::go( bool showminimized )
{
    doShow( showminimized );
    for ( int idx=0; idx<toolbars_.size(); idx++ )
	toolbars_[idx]->handleFinalise( false );
}


bool uiMainWinBody::resetPopupTimerIfNotPoppedUp()
{
    if ( poppedup_ || !finalised() )
	return false;

    if ( poptimer_.isActive() )
	poptimer_.stop();

    if ( !poppedup_ )
	poptimer_.start( 100, true );

    return true;
}


QMenu* uiMainWinBody::createPopupMenu()
{ return createtbmenu_ ? QMainWindow::createPopupMenu() : 0; }


void uiMainWinBody::popTimTick( CallBacker* )
{
    if ( poppedup_ )
	{ pErrMsg( "huh?" ); return; }
    poppedup_ = true;

// TODO: Remove when we can get rid of the popTimTick
    if ( prefsz_.hNrPics()>0 && prefsz_.vNrPics()>0 )
	resize( prefsz_.hNrPics(), prefsz_.vNrPics() );
    if ( !prefpos_.isUdf() )
	move( prefpos_.x_, prefpos_.y_ );
}


void uiMainWinBody::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
    {
	handle_.preFinalise().trigger( handle_ );

	for ( int idx=0; idx<toolbars_.size(); idx++ )
	    toolbars_[idx]->handleFinalise( true );
    }

    centralwidget_->finalise();
    finaliseChildren();

    if ( trigger_finalise_start_stop )
	handle_.postFinalise().trigger( handle_ );
}


bool uiMainWinBody::handleAlive() const
{
    Threads::Locker locker( winlistlock_ );
    for ( int idx=0; idx<uiMainWin::allwins_.size(); idx++ )
	if ( uiMainWin::allwins_[idx] == &handle_ )
	    return true;
    return false;
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    // refuse if we have any modal window on the screen
    Threads::Locker locker( winlistlock_ );
    for ( int idx=0; idx<uiMainWin::allwins_.size(); idx++ )
	if ( uiMainWin::allwins_[idx]->isModal() )
	    { ce->ignore(); return; }
    locker.unlockNow();

    const int refnr = handle_.beginCmdRecEvent( "Close" );

    if ( !handle_.closeOK() )
	{ ce->ignore(); return; }

    handle_.endCmdRecEvent( refnr, "Close" );
    ce->accept();
    if ( modal_ )
	eventloop_.exit();

    // this may delete this object, *must* be last statement
    handle_.windowClosed.trigger( handle_ );
}


void uiMainWinBody::close()
{
    if ( !handleAlive() || !handle_.closeOK() )
	return;

    handle_.windowClosed.trigger( handle_ );

    if ( testAttribute(Qt::WA_DeleteOnClose) )
	{ QMainWindow::close(); return; }

    if ( modal_ )
	eventloop_.exit();

    handle_.windowHidden.trigger( handle_ );
    QMainWindow::hide();

    if ( exitapponclose_ )
	qApp->quit();
}


void uiMainWinBody::removeDockWin( uiDockWin* dwin )
{
    if ( !dwin )
	return;

    removeDockWidget( dwin->qwidget() );
    dockwins_ -= dwin;
}


void uiMainWinBody::addDockWin( uiDockWin& dwin, uiMainWin::Dock dock )
{
    Qt::DockWidgetArea dwa = Qt::LeftDockWidgetArea;
    if ( dock == uiMainWin::Right )
	dwa = Qt::RightDockWidgetArea;
    else if ( dock == uiMainWin::Top )
	dwa = Qt::TopDockWidgetArea;
    else if ( dock == uiMainWin::Bottom )
	dwa = Qt::BottomDockWidgetArea;
    addDockWidget( dwa, dwin.qwidget() );
    if ( dock == uiMainWin::TornOff )
	dwin.setFloating( true );
    dockwins_ += &dwin;
}


void uiMainWinBody::toggleToolbar( CallBacker* cb )
{
    mDynamicCastGet( uiAction*, action, cb );
    if ( !action ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	if ( tb.hasName( toString(action->text()) ) )
	    tb.display( tb.isHidden() );
    }
}


void uiMainWinBody::updateToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    const ObjectSet<uiAction>& items = toolbarsmnu_->actions();

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiAction& action = *const_cast<uiAction*>( items[idx] );
	if ( tb.name()==tb.name() )
	    action.setChecked( !tb.isHidden() );
    }
}


void uiMainWinBody::addToolBar( uiToolBar* tb )
{
    if ( toolbars_.isPresent(tb) )
	{ pErrMsg("Toolbar is already added"); return; }
    QMainWindow::addToolBar( (Qt::ToolBarArea)tb->prefArea(),
			      tb->getQToolbar() );
    toolbars_ += tb;
    renewToolbarsMenu();
}


uiToolBar* uiMainWinBody::findToolBar( const char* nm )
{
    for ( int idx=0; idx<toolbars_.size(); idx++ )
	if ( toolbars_[idx]->hasName(nm) )
	    return toolbars_[idx];

    return 0;
}


uiToolBar* uiMainWinBody::removeToolBar( uiToolBar* tb )
{
    if ( !toolbars_.isPresent(tb) )
	return 0;

    QMainWindow::removeToolBar( tb->getQToolbar() );
    toolbars_ -= tb;
    renewToolbarsMenu();
    return tb;
}


void uiMainWinBody::renewToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
	toolbars_[idx]->setToolBarMenuAction( 0 );

    toolbarsmnu_->clear();
    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	uiAction* itm =
	    new uiAction( toUiString(tb.name()),
	    mCB(this,uiMainWinBody,toggleToolbar) );
	toolbarsmnu_->insertAction( itm );
	tb.setToolBarMenuAction( itm );
	itm->setCheckable( true );
	itm->setChecked( !tb.isHidden() );
    }
}


static BufferString getSettingsFileName()
{
    BufferString fnm( "qtsettings_", (int)mODVersion );
    File::Path fp( GetSettingsDir(), fnm );
    const char* swusr = GetSoftwareUser();
    if ( swusr )
	fp.setExtension( swusr, false );
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
    QSize qsz( settings.value("size", QSize(200,200)).toSize() );
    prefsz_ = uiSize( qsz.width(), qsz.height() );
    QPoint qpt( settings.value("pos", QPoint(200,200)).toPoint() );
    prefpos_.setXY( qpt.x(), qpt.y() );
    restoreState( settings.value("state").toByteArray() );
    settings.endGroup();

    updateToolbarsMenu();
    hasguisettings_ = true;
}


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

    if ( key == OD::KB_C && modifier == OD::ControlButton )
	handle_.ctrlCPressed.trigger();

    return QMainWindow::keyPressEvent( ev );
}


bool uiMainWinBody::event( QEvent* ev )
{
    const QEvent::Type qtyp = ev->type();
    if ( qtyp == mUsrEvGuiThread )
    {
	mExecMutex( CallBack* actcb = activatecbs_[nractivated_++] );
	actcb->doCall( this );
	handle_.activatedone.trigger( actcb->cbObj() );
	mExecMutex( activatecbs_ -= actcb; nractivated_-- );
	delete actcb;
    }
    else if ( qtyp == mUsrEvPopUpReady )
    {
	handle_.endCmdRecEvent( eventrefnr_, "WinPopUp" );
    }
    else
    {
	const bool res = QMainWindow::event( ev );
	if ( qtyp == QEvent::Show )
	    handle_.windowShown.trigger( handle_ );
	else if ( qtyp == QEvent::Hide )
	    handle_.windowHidden.trigger( handle_ );
	return res;
    }

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


//===== uiMainWin ============================================================


uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(toString(setup.caption_),0)
    , parent_(p)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(setup.caption_)
    , languagechangecount_( TrMgr().changeCount() )
{
    const BufferString bodynm( toString(setup.caption_) );
    body_ = new uiMainWinBody( *this, p, bodynm, setup.modal_ );
    setBody( body_ );
    body_->construct( setup.nrstatusflds_, setup.withmenubar_ );
    body_->setWindowIconText( setup.caption_.isEmpty()
		? QString("OpendTect") : toQString(setup.caption_) );
    body_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );

    finishConstruction();
}


uiMainWin::uiMainWin( uiParent* parnt, const uiString& cpt,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(toString(cpt),0)
    , parent_(parnt)
    , popuparea_(Middle)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(cpt)
    , languagechangecount_( TrMgr().changeCount() )
{
    body_ = new uiMainWinBody( *this, parnt, toString(caption_), modal );
    setBody( body_ );
    body_->construct( nrstatusflds, withmenubar );
    body_->setWindowIconText( caption_.isEmpty()
			     ? QString("OpendTect") : toQString(caption_) );
    finishConstruction();
}


uiMainWin::uiMainWin( const uiString& captn, uiParent* parnt )
    : uiParent(toString(captn),0)
    , parent_(parnt)
    , popuparea_(Middle)
    , windowShown(this)
    , windowHidden(this)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , afterPopup(this)
    , runScriptRequest(this)
    , caption_(captn)
{
    // no finishConstruction() - this constructor is for subclasses that need
    // to call it explicitly at end-of-constructor
}


uiMainWin::~uiMainWin()
{
    detachAllNotifiers();

    Threads::Locker locker( winlistlock_ );
    const auto idxof = allwins_.indexOf( this );
    if ( idxof < 0 )
	{ pErrMsg("Huh"); return; }
    allwins_.removeSingle( idxof, false );
    locker.unlockNow();

    if ( !body_->deletefrombody_ )
    {
	body_->deletefromod_ = true;
	delete body_;
    }

    if ( programmedactivewin_ == this )
	programmedactivewin_ = parent() ? parent()->mainwin() : 0;

    delete afterpopuptimer_;
}


void uiMainWin::finishConstruction()
{
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
    mAttachCB( TrMgr().languageChange, uiMainWin::languageChangeCB );
    Threads::Locker locker( winlistlock_ );
    allwins_ += this;
}


QWidget* uiMainWin::getWidget( int )
{ return body_; }


uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }


void uiMainWin::show()
{
    body_->go();
}




void uiMainWin::close()				{ body_->close(); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::resetPopupTimerIfNotPoppedUp()	{ return body_->
					    resetPopupTimerIfNotPoppedUp(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ body_->showMaximized(); }
void uiMainWin::showMinimized()			{ body_->showMinimized(); }
void uiMainWin::showNormal()			{ body_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return body_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return body_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }
void uiMainWin::setForceFinalise( bool yn )	{ body_->force_finalise_ = yn; }


void uiMainWin::setCaption( const uiString& txt )
{
    caption_ = txt;
    updateCaption();

}


void uiMainWin::updateCaption()
{
    uniquecaption_ = uniqueWinTitle(caption_,body_,0);
    body_->setWindowTitle( toQString(uniquecaption_) );
}


const uiString& uiMainWin::caption( bool unique ) const
{
    return unique ? uniquecaption_ : caption_;
}


void uiMainWin::setDeleteOnClose( bool yn )
{ body_->setAttribute( Qt::WA_DeleteOnClose, yn ); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ body_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ body_->addDockWin( dwin, d ); }


void uiMainWin::addToolBar( uiToolBar* tb )
{ body_->addToolBar( tb ); }

uiToolBar* uiMainWin::findToolBar( const char* nm )
{ return body_->findToolBar( nm ); }

uiToolBar* uiMainWin::removeToolBar( uiToolBar* tb )
{ return body_->removeToolBar( tb ); }


void uiMainWin::addToolBarBreak()
{ body_->addToolBarBreak(); }


uiMenu& uiMainWin::getToolbarsMenu() const
{ return body_->getToolbarsMenu(); }


const ObjectSet<uiToolBar>& uiMainWin::toolBars() const
{ return body_->toolBars(); }


const ObjectSet<uiDockWin>& uiMainWin::dockWins() const
{ return body_->dockWins(); }


uiGroup* uiMainWin::topGroup()
{ return body_->uiCentralWidg(); }


void uiMainWin::setShrinkAllowed(bool yn)
    { if ( topGroup() ) topGroup()->setShrinkAllowed(yn); }


bool uiMainWin::shrinkAllowed()
    { return topGroup() ? topGroup()->shrinkAllowed() : false; }


uiObject* uiMainWin::mainobject()
    { return body_->uiCentralWidg()->mainObject(); }


void uiMainWin::toStatusBar( const uiString& txt, int fldidx, int msecs )
{
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt, fldidx, msecs );
    else if ( !txt.isEmpty() )
	UsrMsg( txt );
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

    return &_mwb->handle_;
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


bool uiMainWin::doSetWindowFlags( od_uint32 todoflagi, bool setyn )
{
    const Qt::WindowFlags todoflag( todoflagi );
    const Qt::WindowFlags flags = body_->windowFlags();
    if ( ( setyn &&  (flags & todoflag)) ||
         (!setyn && !(flags & todoflag)) )
	return false;

    if ( !isMinimized() && !isHidden() )
	pErrMsg( "Setting window flags on a displayed widget "
		 "hides it (see Qt doc)" );

    const bool isfinalised = finalised();
    body_->doSetWindowFlags( todoflag, setyn );
    if ( isfinalised )
	Threads::sleep( 0.3 );
    return true;
}


bool uiMainWin::showMinMaxButtons( bool yn )
{ return doSetWindowFlags( Qt::WindowMinMaxButtonsHint, yn ); }

bool uiMainWin::showAlwaysOnTop( bool yn )
{ return doSetWindowFlags( Qt::WindowStaysOnTopHint, yn ); }


void uiMainWin::showAndActivate()
{
    if ( isMinimized() || isHidden() )
	showNormal();
    activate();
}


void uiMainWin::activate()
{
    if ( !finalised() )
	return;

#ifdef __win__
# if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
    QWindowsWindowFunctions::setWindowActivationBehavior(
			    QWindowsWindowFunctions::AlwaysActivateWindow );
# endif
#endif

    body_->activateWindow();
}


void uiMainWin::setIcon( const uiPixmap& pm )
{ body_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIcon( const char* iconid )
{ setIcon( uiPixmap(iconid) ); }

void uiMainWin::setIconText( const uiString& txt)
{ body_->setWindowIconText( toQString(txt) ); }

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


void uiMainWin::runScript( const char* filename )
{
    scripttorun_ = filename;
    runScriptRequest.trigger();
}


const char* uiMainWin::getScriptToRun() const
{ return scripttorun_; }


uiMainWin* uiMainWin::activeWindow()
{
    if ( programmedactivewin_ )
	return programmedactivewin_;

    QWidget* _aw = qApp->activeWindow();
    if ( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if ( !_awb )	return 0;

    return &_awb->handle_;
}


uiMainWin::ActModalTyp uiMainWin::activeModalType()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )					return None;

    if ( dynamic_cast<uiMainWinBody*>(amw) )	return Main;
    if ( dynamic_cast<QMessageBox*>(amw) )	return Message;
    if ( dynamic_cast<QFileDialog*>(amw) )	return File;
    if ( dynamic_cast<QColorDialog*>(amw) )	return Colour;
    if ( dynamic_cast<QFontDialog*>(amw) )	return Font;

    return Unknown;
}


uiMainWin* uiMainWin::activeModalWindow()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )	return 0;

    uiMainWinBody* mwb = dynamic_cast<uiMainWinBody*>( amw );
    if ( !mwb )	return 0;

    return &mwb->handle_;
}


BufferString uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    return amw ? BufferString( amw->windowTitle() ) : BufferString();
}


static QMessageBox::StandardButton getStandardButton( const QMessageBox* qmb,
						      int buttonnr )
{
    int stdbutcount = 0;

    for ( unsigned int idx=QMessageBox::Ok;
	  qmb && idx<=QMessageBox::RestoreDefaults; idx+=idx )
    {
	if ( !qmb->button((QMessageBox::StandardButton) idx) )
	    continue;

	if ( stdbutcount == buttonnr )
	    return (QMessageBox::StandardButton) idx;

	stdbutcount++;
    }

    return QMessageBox::NoButton;
}


BufferString uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    BufferString ret;
    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );

	const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );
	if ( stdbut )
	    ret = qmb->button(stdbut)->text();
	    // TODO: get original text if button text is translation
    }
    else if ( typ==Colour || typ==Font || typ==File )
    {
	if ( buttonnr == 0 )
	    ret = "Cancel";
	else if ( buttonnr == 1 )
	    ret = typ==File ? "Ok" : "OK";
    }

    return ret;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    QWidget* amw = qApp->activeModalWidget();
    const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
    const QMessageBox::StandardButton stdbut =
					getStandardButton( qmb, buttonnr );

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
    Threads::Locker locker( winlistlock_ );
    for ( int idx=0; idx<allwins_.size(); idx++ )
    {
	auto* win = allwins_[idx];
	if ( !visibleonly || !win->isHidden() )
	    windowlist += win;
    }
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
	    sprintf( qwptrstr.getCStr(), "%p", qw );
	    signatures.add( qwptrstr );
	}
    }
}


uiString uiMainWin::uniqueWinTitle( const uiString& txt,
				    QWidget* forwindow,
				    BufferString* outputaddendum )
{
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    uiString res;
    for ( int count=1; true; count++ )
    {
	bool unique = true;
	uiString beginning = txt.isEmpty() ? tr("<no title>") : txt;

	if ( count>1 )
	{
	    res = toUiString( "%1 {%2}").arg( beginning )
					.arg( toUiString(count) );
	    BufferString addendum( "  {", toString(count), "}" );
	    if ( outputaddendum ) *outputaddendum = addendum;
	}
	else
	{
	    res = beginning;
	}

	QString wintitle = toQString( res );

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( qw->windowTitle() == wintitle )
	    {
		unique = false;
		break;
	    }
	}

	if ( unique ) break;
    }

    return res;
}


std::string OD_Win_GetSnapShotFile(const std::string&);

bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
#ifdef __win__

    std::string snapshotfile = OD_Win_GetSnapShotFile( filenm );
    if ( snapshotfile.empty() )
	return false;

    QPixmap desktopsnapshot;

    if ( !desktopsnapshot.load( snapshotfile.c_str() ) )
	{ ErrMsg( "Generated GDI+ image does not load in Qt" ); return false; }

    File::remove( snapshotfile.c_str() );

#else

    QScreen* qscreen = body_ ? body_->screen() : uiMainWinBody::primaryScreen();
    if ( !qscreen ) return false;
    const WId desktopwinid = QApplication::desktop()->winId();
    QPixmap desktopsnapshot = qscreen->grabWindow( desktopwinid );

#endif

    if ( zoom > 0 )
    {
	QWidget* qwin = qApp->activeModalWidget();
	if ( !qwin || zoom==1 )
	    qwin = body_;

#ifdef __win__

	RECT rect = {};
	GetWindowRect( (HWND)qwin->winId() , &rect );
	const int width  = rect.right - rect.left;
	const int height = rect.bottom - rect.top;

#else

	const int width = qwin->frameGeometry().width();
	/*on windows, it gets width till end of monitor and not entire widget*/
	const int height = qwin->frameGeometry().height();

#endif
	desktopsnapshot = desktopsnapshot.copy( qwin->x(), qwin->y(),
						width, height );
    }

    return desktopsnapshot.save( QString(filenm), format, quality );
}


bool uiMainWin::grabScreen( const char* filenm, const char* format, int quality,
			    int screenidx )
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.isEmpty() ) return false;

    const int nrscreens = screens.size();
    QScreen* qscreen = 0;
    if ( screenidx==-1 )
	qscreen = QGuiApplication::primaryScreen();
    else if ( screenidx>=0 && screenidx<nrscreens )
	qscreen = screens.at( screenidx );
    else
	qscreen = screens.first();

    if ( !qscreen ) return false;

    const QRect geom = qscreen->geometry();
    QPixmap snapshot = qscreen->grabWindow( 0, geom.left(), geom.top(),
					    geom.width(), geom.height() );
    return snapshot.save( QString(filenm), format, quality );
}


void uiMainWin::activateInGUIThread( const CallBack& cb, bool busywait )
{ body_->activateInGUIThread( cb, busywait ); }


void uiMainWin::translateText()
{
    uiParent::translateText();

    for ( int idx=0; idx<body_->toolbars_.size(); idx++ )
	body_->toolbars_[idx]->translateText();

    //Don't know if anything special needs to be done here.
}


void uiMainWin::copyToClipBoardCB( CallBacker* )
{
    copyToClipBoard();
}


void uiMainWin::languageChangeCB( CallBacker* )
{
    if ( languagechangecount_<TrMgr().changeCount() )
    {
	translateText();
	languagechangecount_ = TrMgr().changeCount();
    }
}


void uiMainWin::aftPopupCB( CallBacker* )
{
    afterPopup.trigger();
}



class ImageSaver : public CallBacker
{ mODTextTranslationClass(ImageSaver)
public:

ImageSaver()
{
    timer_.tick.notify( mCB(this,ImageSaver,shootImageCB) );
}


void setImageProp( WId qwid, int w, int h, int r )
{
    qwinid_ = qwid;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = true;
    timer_.start( 500, true );
}


void setImageProp( WId qwid, const char* fnm, int w, int h, int r )
{
    qwinid_ = qwid;
    fname_ = fnm;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = false;
    timer_.start( 500, true );
}


protected:
void shootImageCB( CallBacker* )
{
    QScreen* qscreen = uiMainWinBody::primaryScreen();
    if ( !qscreen ) return;
    const QPixmap snapshot = qscreen->grabWindow( qwinid_ );

    QImage image = snapshot.toImage();
    image = image.scaledToWidth( width_ );
    image = image.scaledToHeight( height_ );
    image.setDotsPerMeterX( (int)(res_/0.0254) );
    image.setDotsPerMeterY( (int)(res_/0.0254) );
    if ( copytoclipboard_ )
	uiClipboard::setImage( image );
    else
	image.save( fname_ );

    timer_.stop();
}

    int		width_;
    int		height_;
    int		res_;
    bool	copytoclipboard_;
    QString	fname_;
    WId		qwinid_;
    Timer	timer_;
};


void uiMainWin::copyToClipBoard()
{
    QWidget* qwin = getWidget(0);
    if ( !qwin )
	qwin = body_;
    WId wid = qwin->winId();
    const int width = qwin->frameGeometry().width();
    const int height = qwin->frameGeometry().height();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, width, height, uiMain::getMinDPI() );
}


void uiMainWin::saveImage( const char* fnm, int width, int height, int res )
{
    QWidget* qwin = getWidget(0);
    if ( !qwin )
	qwin = body_;
    WId wid = qwin->winId();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, fnm, width, height, res );
}


void uiMainWin::saveAsPDF_PS( const char* filename, bool aspdf, int w,
				    int h, int res )
{
    QString fileName( filename );
    QPrinter* pdfprinter = new QPrinter();
    pdfprinter->setOutputFormat( QPrinter::PdfFormat );
    pdfprinter->setPaperSize( QSizeF(w,h), QPrinter::Point );
    pdfprinter->setFullPage( false );
    pdfprinter->setOutputFileName( filename );
    pdfprinter->setResolution( res );

    QPainter* pdfpainter = new QPainter();
    pdfpainter->begin( pdfprinter );
    QWidget* qwin = getWidget(0);
    qwin->render( pdfpainter, pdfprinter->pageRect().topLeft(), qwin->rect() );
    pdfpainter->end();
    delete pdfpainter;
    delete pdfprinter;
}


//============================================================================

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/

#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public uiMainWinBody
{ mODTextTranslationClass(uiDialogBody);
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);
			~uiDialogBody();

    bool		exec(bool showminimized);

    void		reject( CallBacker* s )
			{
			    mHandle.cancelpushed_ = s == cnclbut_;
			    if ( mHandle.rejectOK() )
				_done( 0 );
			}
    void		accept( CallBacker* s )
			{
			    if ( mHandle.acceptOK() )
				_done( 1 );
			}
    void		done( int res )
			{
			    _done( res );
			}

    void		uiSetResult( int v )	{ result_ = v; }
    int			uiResult()		{ return result_; }

    void		setTitleText(const uiString& txt);
    void		setOkCancelText(const uiString&,const uiString&);
    void		setOkText(const uiString&);
			//!< OK button disabled when set to empty
    void		setCancelText(const uiString&);
			//!< Cancel button disabled when set to empty
    void		setApplyText(const uiString&);
    void		enableSaveButton(const uiString& txt);
    void		setSaveButtonChecked(bool yn);
    void		setButtonSensitive(uiDialog::Button,bool yn);
    bool		saveButtonChecked() const;
    bool		hasSaveButton() const;
    uiButton*		button(uiDialog::Button);

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )	{ setup_.separator_ = yn; }
    bool		separator() const	{ return setup_.separator_; }
    void		setHelpKey(const HelpKey& key) { setup_.helpkey_ = key;}
    HelpKey		helpKey() const	{ return setup_.helpkey_; }

    void		setDlgGrp( uiGroup* cw )	{ dlggrp_=cw; }

    void		setHSpacing( int spc )	{ dlggrp_->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlggrp_->setVSpacing(spc); }
    void		setBorder( int b )	{ dlggrp_->setBorder( b ); }

    virtual void        addChild(uiBaseObject& child);
    virtual void        manageChld_(uiBaseObject&,uiObjectBody&);
    virtual void	attachChild(ConstraintType,uiObject* child,
				    uiObject* other,int margin,bool reciprocal);
    void		provideHelp(CallBacker*);
    void		showCredits(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup_; }

protected:

    virtual const QWidget* managewidg_() const
			{
			    if ( !initing_ )
				return dlggrp_->pbody()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int			result_;
    bool		initchildrendone_;

    uiGroup*            dlggrp_;
    uiDialog::Setup	setup_;

    uiButton*		okbut_;
    uiButton*		cnclbut_;
    uiButton*		applybut_;
    uiButton*		helpbut_;
    uiToolButton*	creditsbut_;

    uiCheckBox*		savebutcb_;
    uiToolButton*	savebuttb_;

    uiLabel*		titlelbl_;

    void		_done(int);

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool);
    void		closeEvent(QCloseEvent*);
    void		applyCB(CallBacker*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);
    void		layoutChildrenOld(uiObject*);

};


uiDialogBody::uiDialogBody( uiDialog& hndle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(hndle,parnt,toString(s.wintitle_),s.modal_)
    , dlggrp_(0)
    , setup_(s)
    , okbut_(0), cnclbut_(0), applybut_(0)
    , savebutcb_(0),  savebuttb_(0)
    , helpbut_(0), creditsbut_(0)
    , titlelbl_(0), result_(-1)
    , initchildrendone_(false)
{
    setContentsMargins( 10, 2, 10, 2 );

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Dialog;
    setWindowFlags( flags );
}


uiDialogBody::~uiDialogBody()
{
    if ( okbut_ )
	okbut_->activated.remove( mCB(this,uiDialogBody,accept) );

    if ( cnclbut_ )
	cnclbut_->activated.remove( mCB(this,uiDialogBody,reject) );
}


bool uiDialogBody::exec( bool showminimized )
{
    uiSetResult( -1 );

    if ( setup_.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go( showminimized );
    return uiResult();
}


void uiDialogBody::setTitleText( const uiString& txt )
{
    setup_.dlgtitle_ = txt;
    if ( titlelbl_ )
	titlelbl_->setText( txt );
}


void uiDialogBody::setOkCancelText( const uiString& oktxt,
				    const uiString& cncltxt )
{
    setOkText( oktxt );
    setCancelText( cncltxt );
}


void uiDialogBody::setOkText( const uiString& txt )
{
    setup_.oktext_ = txt;
    if ( okbut_ ) okbut_->setText(txt);
}


void uiDialogBody::setCancelText( const uiString& txt )
{
    setup_.canceltext_ = txt;
    if ( cnclbut_ ) cnclbut_->setText( txt );
}


void uiDialogBody::setApplyText( const uiString& txt )
{
    if ( applybut_ )
	applybut_->setText( txt );
}


bool uiDialogBody::hasSaveButton() const
{ return savebutcb_; }


bool uiDialogBody::saveButtonChecked() const
{ return savebutcb_ ? savebutcb_->isChecked() : false; }


/*! Hides the box, which also exits the event loop in case of a modal box.  */
void uiDialogBody::_done( int res )
{
    uiSetResult( res );
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


void uiDialogBody::enableSaveButton( const uiString& txt )
{ setup_.savetext_ = txt; setup_.savebutton_ = true; }

void uiDialogBody::setSaveButtonChecked( bool yn )
{
    setup_.savechecked_ = yn;
    if ( savebutcb_ ) savebutcb_->setChecked(yn);
}


void uiDialogBody::setButtonSensitive( uiDialog::Button but, bool yn )
{
    switch ( but )
    {
    case uiDialog::OK:		if ( okbut_ ) okbut_->setSensitive(yn);
    break;
    case uiDialog::CANCEL:	if ( cnclbut_ ) cnclbut_->setSensitive(yn);
    break;
    case uiDialog::APPLY:	if ( applybut_ ) applybut_->setSensitive(yn);
    break;
    case uiDialog::HELP:	if ( helpbut_ ) helpbut_->setSensitive(yn);
    break;
    case uiDialog::SAVE:
	if ( savebutcb_ ) savebutcb_->setSensitive(yn);
	if ( savebuttb_ ) savebuttb_->setSensitive(yn);
    break;
    case uiDialog::CREDITS:
	if ( creditsbut_ ) creditsbut_->setSensitive(yn);
    break;
    }
}


uiButton* uiDialogBody::button( uiDialog::Button but )
{
    switch ( but )
    {
    case uiDialog::OK:		return okbut_; break;
    case uiDialog::CANCEL:	return cnclbut_; break;
    case uiDialog::APPLY:	return applybut_; break;
    case uiDialog::HELP:	return helpbut_; break;
    case uiDialog::SAVE:
	return savebutcb_
	    ? (uiButton*)savebutcb_ : (uiButton*)savebuttb_;
    break;
    case uiDialog::CREDITS:	return creditsbut_; break;
    }

    return 0;
}


void uiDialogBody::addChild( uiBaseObject& child )
{
    if ( !initing_ )
	dlggrp_->addChild( child );
    else
	uiMainWinBody::addChild( child );
}


void uiDialogBody::manageChld_( uiBaseObject& o, uiObjectBody& b )
{
    if ( !initing_ )
	dlggrp_->manageChld( o, b );
}


void uiDialogBody::attachChild( ConstraintType tp, uiObject* child,
				uiObject* other, int margin, bool reciprocal )
{
    if ( !child || initing_ ) return;

    dlggrp_->attachChild( tp, child, other, margin, reciprocal );
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

    dlggrp_->finalise();

    if ( !initchildrendone_ )
	initChildren();

    finaliseChildren();

    handle_.postFinalise().trigger( handle_ );
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
    if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
	layoutChildrenOld( lowestobject );
    else
	layoutChildren( lowestobject );

    if ( okbut_ )
    {
	okbut_->activated.notify( mCB(this,uiDialogBody,accept) );
	mDynamicCastGet(uiPushButton*,pb,okbut_)
	if ( pb )
	    pb->setDefault();
    }
    if ( cnclbut_ )
    {
	cnclbut_->activated.notify( mCB(this,uiDialogBody,reject) );
	if ( !okbut_ )
	{
	    mDynamicCastGet(uiPushButton*,pb,cnclbut_)
	    if ( pb )
		pb->setDefault();
	}
    }

    initchildrendone_ = true;
}


uiObject* uiDialogBody::createChildren()
{
    if ( !setup_.oktext_.isEmpty() )
	okbut_ = uiButton::getStd( centralwidget_, OD::Ok, CallBack(),
				   true, setup_.oktext_ );
    if ( !setup_.canceltext_.isEmpty() )
	cnclbut_ = uiButton::getStd( centralwidget_, OD::Cancel,
				     CallBack(), true, setup_.canceltext_ );
    if ( setup_.applybutton_ )
	applybut_ = uiButton::getStd( centralwidget_, OD::Apply,
				mCB(this,uiDialogBody,applyCB), true,
				   setup_.applytext_ );

    if ( setup_.savebutton_ && !setup_.savetext_.isEmpty() )
    {
	if ( setup_.savebutispush_ )
	    savebuttb_ = new uiToolButton( centralwidget_, "save",
			  setup_.savetext_, CallBack() );
	else
	{
	    savebutcb_ = new uiCheckBox( centralwidget_, setup_.savetext_ );
	    savebutcb_->setChecked( setup_.savechecked_ );
	}
    }
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    if ( !dlg.helpKey().isEmpty() )
    {
	mDefineStaticLocalObject( bool, shwhid,
				  = GetEnvVarYN("DTECT_SHOW_HELP") );
#ifdef __debug__
	shwhid = true;
#endif

	helpbut_ = uiButton::getStd( centralwidget_, OD::Help,
				mCB(this,uiDialogBody,provideHelp), true );
	if ( shwhid )
	    helpbut_->setToolTip( toUiString("%1 %2")
			    .arg(toUiString(dlg.helpKey().providername_))
			    .arg(toUiString(dlg.helpKey().argument_)) );
	else
	    helpbut_->setToolTip( tr("Help on this window") );
    }

    if ( !setup_.menubar_ && !setup_.dlgtitle_.isEmpty() )
    {
	titlelbl_ = new uiLabel( centralwidget_, setup_.dlgtitle_ );
	titlelbl_->setHSzPol( uiObject::WideVar );
	uiObject* obj = setup_.separator_
			    ? (uiObject*)new uiSeparator(centralwidget_)
			    : (uiObject*)titlelbl_;

	if ( obj != titlelbl_ )
	{
	    if ( uiDialog::titlePos() == uiDialog::CenterWin )
		titlelbl_->attach( centeredAbove, obj );
	    else if ( uiDialog::titlePos() == uiDialog::RightSide )
		titlelbl_->attach( rightBorder );
	    else if ( uiDialog::titlePos() == uiDialog::LeftSide )
		titlelbl_->attach( leftBorder );
	    obj->attach( stretchedBelow, titlelbl_, -2 );
	}
	if ( setup_.mainwidgcentered_ )
	    dlggrp_->attach( centeredBelow, obj );
	else
	    dlggrp_->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlggrp_->mainObject();
    if ( setup_.separator_ && ( okbut_ || cnclbut_ || savebutcb_ ||
			       savebuttb_ || helpbut_) )
    {
	uiSeparator* horSepar = new uiSeparator( centralwidget_ );
	horSepar->attach( stretchedBelow, dlggrp_, -2 );
	lowestobj = horSepar;
    }

    return lowestobj;
}


static const int hborderdist = 1;
static const int vborderdist = 5;

static void attachButton( uiObject* but, uiObject*& prevbut,
			  uiObject* lowestobj )
{
    if ( !but ) return;

    but->attach( ensureBelow, lowestobj );
    but->attach( bottomBorder, vborderdist );
    if ( prevbut )
	but->attach( leftOf, prevbut );
    else
	but->attach( rightBorder, hborderdist );

    prevbut = but;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;

    uiObject* prevbut = 0;
    attachButton( helpbut_, prevbut, lowestobj );
    attachButton( applybut_, prevbut, lowestobj );
    attachButton( rightbut, prevbut, lowestobj );
    attachButton( leftbut, prevbut, lowestobj );
    attachButton( creditsbut_, prevbut, lowestobj );

    uiObject* savebut = savebutcb_;
    if ( !savebut ) savebut = savebuttb_;
    if ( savebut )
    {
	savebut->attach( ensureBelow, lowestobj );
	savebut->attach( bottomBorder, vborderdist );
	savebut->attach( leftBorder, hborderdist );
	if ( prevbut )
	    savebut->attach( ensureLeftOf, prevbut );
    }
}


void uiDialogBody::layoutChildrenOld( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;
    uiObject* exitbut = okbut_ ? okbut_ : cnclbut_;
    uiObject* centerbut = helpbut_;
    uiObject* extrabut = savebuttb_;

    if ( !okbut_ || !cnclbut_ )
    {
	leftbut = rightbut = 0;
	if ( exitbut )
	{
	    centerbut = exitbut;
	    extrabut = helpbut_;
	    leftbut = savebuttb_;
	}
    }

    if ( !centerbut )
    {
	centerbut = extrabut;
	extrabut = 0;
    }

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

    if ( savebutcb_ )
    {
	savebutcb_->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, savebutcb_ );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, savebutcb_ );
    }

    if ( extrabut )
	extrabut->attach( rightOf, centerbut );
}


void uiDialogBody::provideHelp( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    HelpProvider::provideHelp( dlg.helpKey() );
}


void uiDialogBody::applyCB( CallBacker* cb )
{
    mDynamicCastGet(uiDialog&,dlg,handle_);
    if ( dlg.applyOK() )
	dlg.applyPushed.trigger( cb );
}


//====== uiDialog =============================================================


#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
    : uiMainWin( s.wintitle_, p )
    , ctrlstyle_(OkAndCancel)
    , cancelpushed_(false)
    , applyPushed(this)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.nrstatusflds_, s.menubar_ );

    auto* centralwidget = new uiGroup( body_->uiCentralWidg(),
				       "uiDialog's Central Widget group" );
    centralwidget->setStretch( 2, 2 );
    mBody->setDlgGrp( centralwidget );

    setTitleText( s.dlgtitle_ );

    finishConstruction();
}


void uiDialog::setButtonText( Button but, const uiString& txt )
{
    switch ( but )
    {
	case OK		: setOkText( txt ); break;
	case CANCEL	: setCancelText( txt ); break;
	case APPLY	: mBody->setApplyText( txt ); break;
	case HELP	: pErrMsg("set help txt but"); break;
	case SAVE	: enableSaveButton( txt ); break;
	case CREDITS	: pErrMsg("set credits txt but"); break;
    }
}


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    uiString oktext = toUiString("Run");
    uiString canceltext = uiStrings::sClose();
    if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
    {
	oktext = uiStrings::sGo();
	canceltext = uiStrings::sCancel();
    }

    switch ( cs )
    {
    case OkAndCancel:
	setOkCancelText( uiStrings::sOk(), uiStrings::sCancel() );
    break;
    case RunAndClose:
	setOkCancelText( oktext, canceltext );
    break;
    case CloseOnly:
	    setOkCancelText(
		mBody->finalised() ? canceltext : uiString::empty(),
		canceltext );
    break;
    }

    ctrlstyle_ = cs;
}


bool uiDialog::go()
{
    return mBody->exec( false );
}


bool uiDialog::goMinimized()
{
    return mBody->exec( true );
}


void uiDialog::done( DoneResult dr )
{
    mBody->done( dr == Rejected ? 0 : 1 );
}


const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setTitleText(const uiString& txt){ mBody->setTitleText(txt); }
void uiDialog::setOkText( const uiString& txt ) { mBody->setOkText(txt); }
void uiDialog::setCancelText(const uiString& t) { mBody->setCancelText(t);}
void uiDialog::enableSaveButton(const uiString& t){ mBody->enableSaveButton(t);}
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpKey( const HelpKey& key ) { mBody->setHelpKey(key); }
HelpKey uiDialog::helpKey() const		{ return mBody->helpKey(); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setOkCancelText( const uiString& oktxt, const uiString& cncltxt )
{ mBody->setOkCancelText( oktxt, cncltxt ); }

void uiDialog::setButtonSensitive(uiDialog::Button b, bool s )
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b)
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }

uiDialog::TitlePos uiDialog::titlepos_		= LeftSide;
uiDialog::TitlePos uiDialog::titlePos()		{ return titlepos_; }
void uiDialog::setTitlePos( TitlePos p )	{ titlepos_ = p; }
