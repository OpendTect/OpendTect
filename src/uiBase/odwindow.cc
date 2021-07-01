/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/

#include "odwindow.h"

#include "uibutton.h"
#include "uidockwin.h"
#include "uihelpview.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiseparator.h"
#include "uistatusbar.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "filepath.h"
#include "keyboardevent.h"
#include "oddirs.h"

#include <QApplication>
#include <QCloseEvent>
#include <QMainWindow>
#include <QScreen>
#include <QSettings>
#include <QWindow>

mUseQtnamespace

static ObjectSet<uiMainWin>	mwlist_;
static Threads::Lock		mwlistlock_;


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

    mAttachCB( poptimer_.tick, uiMainWinBody::popTimTick );
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


void uiMainWinBody::getTopLevelWindows( ObjectSet<uiMainWin>& list,
					bool visibleonly )
{
    list.erase();
    Threads::Locker locker( mwlistlock_ );
    for ( int idx=0; idx<mwlist_.size(); idx++ )
    {
	auto* win = mwlist_[idx];
	if ( !visibleonly || !win->isHidden() )
	    list += win;
    }
}


void uiMainWinBody::addMainWindow( uiMainWin& mw )
{
    Threads::Locker locker( mwlistlock_ );
    mwlist_.add( &mw );
}


void uiMainWinBody::removeWindow( uiMainWin& mw )
{
    Threads::Locker locker( mwlistlock_ );
    const auto idxof = mwlist_.indexOf( &mw );
    if ( mwlist_.validIdx(idxof) )
	mwlist_.removeSingle( idxof, false );
    else
	{ pErrMsg("Huh"); }
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
    if ( uiMainWin::getActivateBehaviour() == OD::AlwaysActivateWindow )
    {
	activateWindow();
	uiMainWin::setActivateBehaviour( OD::DefaultActivateWindow );
    }

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
    Threads::Locker locker( mwlistlock_ );
    for ( int idx=0; idx<mwlist_.size(); idx++ )
	if ( mwlist_[idx] == &handle_ )
	    return true;
    return false;
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    // refuse if we have any modal window on the screen
    Threads::Locker locker( mwlistlock_ );
    for ( int idx=0; idx<mwlist_.size(); idx++ )
	if ( mwlist_[idx]->isModal() )
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



// uiDialogBody
uiDialogBody::uiDialogBody( uiDialog& hndle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(hndle,parnt,toString(s.wintitle_),s.modal_)
    , result_(-1)
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


bool uiDialogBody::exec( bool showminimized )
{
    uiSetResult( -1 );

    if ( setup_.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go( showminimized );
    return uiResult();
}


#define mHandle static_cast<uiDialog&>(handle_)

void uiDialogBody::reject( CallBacker* cb )
{
    mHandle.cancelpushed_ = cb == cnclbut_;
    if ( mHandle.rejectOK() )
	_done(0);
}


void uiDialogBody::accept( CallBacker* )
{
    if ( mHandle.acceptOK() )
	_done(1);
}


void uiDialogBody::applyCB( CallBacker* cb )
{
    if ( mHandle.applyOK() )
	uiSetResult(2);
}


void uiDialogBody::done( int res )
{
    _done( res );
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
	mDefineStaticLocalObject( bool, shwhid,
				  = GetEnvVarYN("DTECT_SHOW_HELP") );
#ifdef __debug__
	shwhid = true;
#endif

	helpbut_ = uiButton::getStd( butgrp, OD::Help,
				mCB(this,uiDialogBody,provideHelp), true,
				uiString::empty() );
	if ( shwhid )
	    helpbut_->setToolTip( HelpProvider::description(helpkey) );
	else
	    helpbut_->setToolTip( tr("Help on this window") );
    }

    const HelpKey videokey = videoKey(0);
    if ( !videokey.isEmpty() && HelpProvider::hasHelp(videokey) )
    {
	const CallBack cb = mCB(this,uiDialogBody,showVideo);
	videobut_ = uiButton::getStd( butgrp, OD::Video, cb, true,
				uiString::empty() );
	if ( videokeys_.size()==1 )
	{
	    uiString tt = HelpProvider::description( videokey );
	    videobut_->setToolTip( tt );
	}
	else
	{
	    uiMenu* menu = new uiMenu();
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
