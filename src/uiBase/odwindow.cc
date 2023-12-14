/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "odwindow.h"

#include "uibutton.h"
#include "uidockwin.h"
#include "uihelpview.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiparentbody.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "keyboardevent.h"
#include "od_ostream.h"
#include "oddirs.h"

#include <QApplication>
#include <QCloseEvent>
#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QWindow>

mUseQtnamespace


static Threads::Mutex		winlistmutex_;
static ObjectSet<uiMainWin>	orderedwinlist_;

#ifdef __debug__
static bool			debugmw = false;
#endif

static void addToOrderedWinList( uiMainWin* uimw )
{
    winlistmutex_.lock();
    orderedwinlist_ -= uimw;
    orderedwinlist_ += uimw;
    winlistmutex_.unLock();
}


static bool isInOrderedWinList( const uiMainWin* uimw )
{
    winlistmutex_.lock();
    const bool res = orderedwinlist_.isPresent( uimw );
    winlistmutex_.unLock();
    return res;
}


static bool hasChildWindows( uiMainWin& curwin )
{
    winlistmutex_.lock();
    for ( int idx=0; idx<orderedwinlist_.size(); idx++ )
    {
	uiMainWin* mw = orderedwinlist_[idx];
	if ( mw->parent()==&curwin && mw->isModal() )
	{
	    mw->raise();
	    winlistmutex_.unLock();
	    return true;
	}
    }

    winlistmutex_.unLock();
    return false;
}


// uiMainWinBody
#define mParent p && p->pbody() ? p->pbody()->qwidget() : 0
uiMainWinBody::uiMainWinBody( uiMainWin& uimw, uiParent* p,
			      const char* nm, bool modal )
	: uiCentralWidgetBody(nm)
	, QMainWindow(mParent)
	, handle_(uimw)
	, statusbar_(nullptr)
	, menubar_(nullptr)
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
	, force_finalize_(false)
{
    if ( nm && *nm )
	setObjectName( nm );

    mAttachCB( poptimer_.tick, uiMainWinBody::popTimTick );

    iconsz_ = uiObject::iconSize();
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( p && modal ? Qt::WindowModal
				  : Qt::NonModal );

#ifdef __mac__
    setUnifiedTitleAndToolBarOnMac( true );
#endif

    setDockOptions( VerticalTabs | AnimatedDocks );

    deletefrombody_ = deletefromod_ = false;

#ifdef __debug__
    static bool debugmw_loc = GetEnvVarYN("DTECT_DEBUG_MAINWIN");
    debugmw = debugmw_loc;

    if ( debugmw )
	od_cout() << "uiMainWinBody: " << name() << od_endl;
#endif
}


uiMainWinBody::~uiMainWinBody()
{
#ifdef __debug__
    if ( debugmw )
    {
	const char* src = deletefromod_ ? " (from od)" : " (from qt)";
	od_cout() << "~uiMainWinBody: " << name() << src << od_endl;
    }
#endif

    winlistmutex_.lock();
    orderedwinlist_ -= &handle_;
    winlistmutex_.unLock();

    deleteAllChildren(); //delete them now to make sure all ui objects
			 //are deleted before their body counterparts

    deepErase( toolbars_ );

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


bool uiMainWinBody::isDeleteOnClose() const
{
    return testAttribute( Qt::WA_DeleteOnClose );
}


void uiMainWinBody::getTopLevelWindows( ObjectSet<uiMainWin>& list,
					bool visibleonly )
{
    list.erase();
    winlistmutex_.lock();
    for ( int idx=0; idx<orderedwinlist_.size(); idx++ )
    {
	if ( !visibleonly || !orderedwinlist_[idx]->isHidden() )
	    list += orderedwinlist_[idx];
    }
    winlistmutex_.unLock();
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
    bool domove = false;
    if ( !finalized() || force_finalize_ )
    {
	finalize( true );
	domove = true;
    }

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
	if ( isMinimized() )
	    showNormal();

	if ( isHidden() )
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
    if ( !hasguisettings_ && (qrect.width() > mMinSupportedWidth
			   || qrect.height() > mMinSupportedHeight) )
    {
	BufferString msg( "The window '", name(), "' is " );
	msg.add( qrect.width() ).add( "x" ).add( qrect.height() )
	    .add( ". That won't fit on many laptops.\nWe want to support >= " )
	    .add( mMinSupportedWidth ).add( "x" ).add( mMinSupportedHeight )
	    .add( ", see comments in the .cc file." );
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

    if ( domove )
	move( handle_.popuparea_ );

    raise();
    if ( uiMainWin::getActivateBehaviour() == OD::AlwaysActivateWindow )
    {
	activateWindow();
	uiMainWin::setActivateBehaviour( OD::DefaultActivateWindow );
    }

    if ( modal_ )
	eventloop_.exec();
}


void uiMainWinBody::doDisplay( bool yn )
{
    if ( !intray_ )
	doShow( yn );

    QMainWindow::setVisible( yn );
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

	toolbarsmnu_ = new uiMenu( &handle_, tr("Toolbars") );
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
	uiMainWin* toplevel = uiMain::instance().topLevel();
	if ( toplevel )
	    parentwidget = toplevel->qWidget();
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
    addToOrderedWinList( &handle_ );
    doShow( showminimized );
}


bool uiMainWinBody::touch()
{
    if ( poppedup_ || !finalized() )
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
    if ( prefpos_ != uiPoint::udf() )
	move( prefpos_.x, prefpos_.y );
}


void uiMainWinBody::finalize( bool trigger_finalize_start_stop )
{
    if ( trigger_finalize_start_stop )
    {
	handle_.preFinalize().trigger( handle_ );

	for ( int idx=0; idx<toolbars_.size(); idx++ )
	    toolbars_[idx]->handleFinalize( true );
    }

    centralwidget_->finalize();
    finalizeChildren();

    if ( trigger_finalize_start_stop )
	handle_.postFinalize().trigger( handle_ );
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    if ( hasChildWindows(handle_) )
    {
	ce->ignore();
	return;
    }

    if ( intray_ )
    {
	hide();
	ce->ignore();
	return;
    }

    const int refnr = handle_.beginCmdRecEvent( "Close" );

    if ( handle_.closeOK() )
    {
	handle_.windowClosed.trigger( handle_ );
	ce->accept();

	if ( isInOrderedWinList(&handle_) && modal_ )
	    eventloop_.exit();
    }
    else
	ce->ignore();

     handle_.endCmdRecEvent( refnr, "Close" );
}


void uiMainWinBody::close()
{
    if ( !handle_.closeOK() )
	return;

    handle_.windowClosed.trigger( handle_ );

    if ( !isInOrderedWinList(&handle_) )
	return;

    if ( testAttribute(Qt::WA_DeleteOnClose) )
    {
	QMainWindow::close();
	return;
    }

    if ( modal_ )
	eventloop_.exit();

    handle_.windowHidden.trigger( handle_ );
    QMainWindow::hide();

    if ( exitapponclose_ && !intray_ )
	qApp->quit();
}


uiStatusBar* uiMainWinBody::uistatusbar()
{ return statusbar_; }

uiMenuBar* uiMainWinBody::uimenubar()
{ return menubar_; }


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
    mDynamicCastGet( uiAction*, action, cb );
    if ( !action ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	if ( tb.name()==action->text().getFullString() )
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
    QMainWindow::addToolBar( (Qt::ToolBarArea)tb->prefArea(), tb->qwidget() );
    toolbars_ += tb;
    renewToolbarsMenu();
}


uiToolBar* uiMainWinBody::findToolBar( const char* nm )
{
    for ( int idx=0; idx<toolbars_.size(); idx++ )
	if ( toolbars_[idx]->name() == nm )
	    return toolbars_[idx];

    return 0;
}


uiToolBar* uiMainWinBody::removeToolBar( uiToolBar* tb )
{
    if ( !toolbars_.isPresent(tb) )
	return 0;

    QMainWindow::removeToolBar( tb->qwidget() );
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
	uiString strnm = tb.getDispNm();
	uiAction* itm =
	    new uiAction( tb.getDispNm(),
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
    if ( !File::exists(fnm) )
    {
	restoreDefaultState();
	return;
    }

    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    QSize qwinsz( settings.value("size", QSize(200,200)).toSize() );
    prefsz_ = uiSize( qwinsz.width(), qwinsz.height() );
    QPoint qwinpos( settings.value("pos", QPoint(200,200)).toPoint() );
    QRect qwinrect( qwinpos, qwinsz );

    QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.size() > 0 ) // resize and reposition window when needed
    {
	const QScreen* scrn = screens[0];
	const QRect vrect = scrn->availableVirtualGeometry();
	if ( qwinrect.width() > vrect.width() )
	    prefsz_.setWidth( vrect.width() );
	if ( qwinrect.height() > vrect.height() )
	    prefsz_.setHeight( vrect.height() );

	if ( qwinrect.left() < vrect.left() )
	    qwinpos.setX( vrect.left() );
	if ( qwinrect.top() < vrect.top() )
	    qwinpos.setY( 0 );
	if ( qwinrect.right() > vrect.right() )
	    qwinpos.setX( vrect.right()-prefsz_.width() );
	if ( qwinrect.bottom() > vrect.bottom() )
	    qwinpos.setY( vrect.bottom()-prefsz_.height() );
    }

    prefpos_.setXY( qwinpos.x(), qwinpos.y() );

    restoreState( settings.value("state").toByteArray() );
    settings.endGroup();

    updateToolbarsMenu();
    hasguisettings_ = true;
}


void uiMainWinBody::restoreDefaultState()
{
    const char* deffnm = "qtdefaultstate.ini";
    const BufferString fp =
	    GetSetupDataFileName(ODSetupLoc_ApplSetupPref,deffnm,true);
    if ( !File::exists(fp) )
	return;

    QSettings defsetts( fp.buf(), QSettings::IniFormat );
    defsetts.beginGroup( NamedObject::name().buf() );
    QVariant defstate = defsetts.value("state");
    restoreState( defstate.toByteArray() );
    defsetts.endGroup();
    updateToolbarsMenu();
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


void uiMainWinBody::resizeEvent( QResizeEvent* ev )
{
    QMainWindow::resizeEvent( ev );
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


// uiDialogBody
uiDialogBody::uiDialogBody( uiDialog& hndle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(hndle,parnt,s.wintitle_.getFullString(),s.modal_)
    , result_(0)
    , initchildrendone_(false)
    , dlggrp_(nullptr)
    , setup_(s)
    , okbut_(nullptr)
    , cnclbut_(nullptr)
    , applybut_(nullptr)
    , helpbut_(nullptr)
    , videobut_(nullptr)
    , creditsbut_(nullptr)
    , savebutcb_(nullptr)
    , savebuttb_(nullptr)
    , titlelbl_(nullptr)
    , dlghandle_(hndle)
{
    setContentsMargins( 10, 2, 10, 2 );

    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::Dialog;
    setWindowFlags( flags );

    if ( !s.videokey_.isEmpty() )
	setVideoKey( s.videokey_ );
}


uiDialogBody::~uiDialogBody()
{
    if ( okbut_ )
	okbut_->activated.remove( mCB(this,uiDialogBody,accept) );

    if ( cnclbut_ )
	cnclbut_->activated.remove( mCB(this,uiDialogBody,reject) );
}


int uiDialogBody::exec( bool showminimized )
{
    uiSetResult( 0 );

    if ( setup_.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go( showminimized );
    return uiResult();
}


void uiDialogBody::reject( CallBacker* cb )
{
    dlghandle_.cancelpushed_ = cb == cnclbut_;
    if ( dlghandle_.rejectOK(cb) )
	_done(0);
    else
	uiSetResult( -1 );
}


void uiDialogBody::accept( CallBacker* cb )
{
    if ( dlghandle_.acceptOK(cb) )
	_done(1);
}


void uiDialogBody::done( int i )
{
    if ( dlghandle_.doneOK(i) )
	_done(i);
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
void uiDialogBody::_done( int v )
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


void uiDialogBody::enableSaveButton( const uiString& txt )
{ setup_.savetext_ = txt; setup_.savebutton_ = true; }

void uiDialogBody::setSaveButtonChecked( bool yn )
{
    setup_.savechecked_ = yn;
    if ( savebutcb_ ) savebutcb_->setChecked(yn);
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

    return nullptr;
}


const uiButton* uiDialogBody::button( uiDialog::Button but ) const
{
    return mSelf().button( but );
}


void uiDialogBody::setButtonSensitive( uiDialog::Button but, bool yn )
{
    uiButton* butobj = button( but );
    if ( butobj )
	butobj->setSensitive( yn );
}


bool uiDialogBody::isButtonSensitive( uiDialog::Button but ) const
{
    const uiButton* butobj = button( but );
    return butobj ? butobj->isSensitive() : false;
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
	dlggrp_->manageChild( o, b );
}


void uiDialogBody::attachChild( constraintType tp, uiObject* child,
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
void uiDialogBody::finalize( bool )
{
    uiMainWinBody::finalize( false );

    handle_.preFinalize().trigger( handle_ );

    dlggrp_->finalize();

    if ( !initchildrendone_ )
	initChildren();

    finalizeChildren();

    handle_.postFinalize().trigger( handle_ );
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
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
    uiGroup* butgrp = new uiGroup( centralwidget_, "OK-Cancel" );
    if ( !setup_.oktext_.isEmpty() )
	okbut_ = uiButton::getStd( butgrp, OD::Ok, CallBack(),
				   true, setup_.oktext_ );
    if ( !setup_.canceltext_.isEmpty() )
	cnclbut_ = uiButton::getStd( butgrp, OD::Cancel,
				     CallBack(), true, setup_.canceltext_ );
    if ( setup_.applybutton_ )
	applybut_ = uiButton::getStd( butgrp, OD::Apply,
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

    mDynamicCastGet(uiDialog&,dlg,handle_)
    const HelpKey helpkey = dlg.helpKey();
    if ( !helpkey.isEmpty() )
    {
	helpbut_ = uiButton::getStd( butgrp, OD::Help,
				mCB(this,uiDialogBody,provideHelp), true,
				uiString::emptyString() );
	helpbut_->setToolTip( HelpProvider::description(helpkey) );
    }

    const HelpKey videokey = videoKey(0);
    if ( !videokey.isEmpty() && HelpProvider::hasHelp(videokey) )
    {
	const CallBack cb = mCB(this,uiDialogBody,showVideo);
	videobut_ = uiButton::getStd( butgrp, OD::Video, cb, true,
				uiString::emptyString() );
	if ( videokeys_.size()==1 )
	    videobut_->setToolTip( HelpProvider::description( videokey ) );
	else
	{
	    auto* menu = new uiMenu();
	    for ( int idx=0; idx<videokeys_.size(); idx++ )
	    {
		const HelpKey& curkey = videokeys_[idx];
		uiString txt = HelpProvider::description( curkey );
		menu->insertAction( new uiAction(txt,cb), idx );
	    }
	    mDynamicCastGet(uiToolButton*,vb,videobut_)
	    if ( vb )
		vb->setMenu( menu, uiToolButton::InstantPopup );
	}
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
	    if ( uiDialog::titlePos() == 0 )
		titlelbl_->attach( centeredAbove, obj );
	    else if ( uiDialog::titlePos() > 0 )
		titlelbl_->attach( rightBorder );
	    obj->attach( stretchedBelow, titlelbl_, -2 );
	}

	if ( setup_.mainwidgcentered_ )
	    dlggrp_->attach( centeredBelow, obj );
	else
	    dlggrp_->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlggrp_->mainObject();
    if ( setup_.separator_ && (okbut_ || cnclbut_ || savebutcb_ ||
			       savebuttb_ || helpbut_ || videobut_) )
    {
	uiSeparator* horSepar = new uiSeparator( centralwidget_ );
	horSepar->attach( stretchedBelow, dlggrp_, -2 );
	lowestobj = horSepar;
    }

    butgrp->attach( ensureBelow, lowestobj );
    butgrp->attach( rightBorder, 1 );

    return lowestobj;
}


static const int hborderdist = 1;
static const int vborderdist = 5;

static void attachButton( uiObject* but, uiObject*& prevbut )
{
    if ( !but ) return;

    if ( prevbut )
	but->attach( leftOf, prevbut );

    prevbut = but;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;

    uiObject* prevbut = 0;
    attachButton( videobut_, prevbut );
    attachButton( helpbut_, prevbut );
    attachButton( applybut_, prevbut );
    attachButton( rightbut, prevbut );
    attachButton( leftbut, prevbut );
    attachButton( creditsbut_, prevbut );

    uiObject* savebut = savebutcb_;
    if ( !savebut ) savebut = savebuttb_;
    if ( savebut )
    {
	savebut->attach( ensureBelow, lowestobj );
	savebut->attach( bottomBorder, vborderdist );
	savebut->attach( leftBorder, hborderdist );
    }
}


void uiDialogBody::provideHelp( CallBacker* )
{
    HelpProvider::provideHelp( helpKey() );
}


void uiDialogBody::showVideo( CallBacker* cb )
{
    int videoidx = 0;
    mDynamicCastGet(uiAction*,action,cb)
    if ( action )
	videoidx = action->getID();

    HelpProvider::provideHelp( videoKey(videoidx) );
}


void uiDialogBody::applyCB( CallBacker* )
{
    mDynamicCastGet(uiDialog&,dlg,handle_);
    dlg.applyPushed.trigger( &handle_ );
}


void uiDialogBody::setVideoKey( const HelpKey& key, int idx )
{
    const bool valididx = videokeys_.validIdx( idx );
    if ( !key.isEmpty() && HelpProvider::hasHelp(key) )
    {
	if ( valididx )
	    videokeys_[idx] = key;
	else
	    videokeys_.addIfNew( key );
    }
}


HelpKey uiDialogBody::videoKey( int idx ) const
{
    return videokeys_.validIdx(idx) ? videokeys_[idx] : HelpKey();
}


int uiDialogBody::nrVideos() const
{ return videokeys_.size(); }


void uiDialogBody::removeVideo( int idx )
{
    if ( videokeys_.validIdx(idx) )
	videokeys_.removeSingle( idx );
}
