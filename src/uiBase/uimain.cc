/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimain.h"

#include "uiaction.h"
#include "uifont.h"
#include "uiicon.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uiobjbody.h"
#include "uitreeview.h"

#include "applicationdata.h"
#include "bufstringset.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "genc.h"
#include "ioman.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "oddirs.h"
#include "oscommand.h"
#include "settings.h"
#include "thread.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QKeyEvent>
#include <QMenu>
#include <QScreen>
#include <QStyleFactory>
#include <QToolTip>
#include <QTreeWidget>

#ifdef __mac__
# include "odlogo128x128.xpm"
  const char** uiMain::XpmIconData = od_logo_128x128;
#else
# include "uimainicon.xpm"
  const char** uiMain::XpmIconData = uimainicon_xpm_data;
#endif
void uiMain::setXpmIconData( const char** xpmdata )
{
    XpmIconData = xpmdata;
}

#ifdef __mac__
# include "uimacinit.h"
#endif


class KeyboardEventFilter : public QObject
{
public:
			KeyboardEventFilter(KeyboardEventHandler& kbeh)
			    : kbehandler_(kbeh)				{}
protected:
    bool		eventFilter(QObject*,QEvent*);

    KeyboardEventHandler& kbehandler_;
};


bool KeyboardEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    if ( ev->type()!=QEvent::KeyPress &&
	 ev->type()!=QEvent::KeyRelease )
	return false;

    const QKeyEvent* qke = dynamic_cast<QKeyEvent*>( ev );
    if ( !qke ) return false;

    KeyboardEvent kbe;
    kbe.key_ = (OD::KeyboardKey) qke->key();
    kbe.modifier_ = OD::ButtonState( (int) qke->modifiers() );
    kbe.isrepeat_ = qke->isAutoRepeat();

    if ( ev->type() == QEvent::KeyPress )
	kbehandler_.triggerKeyPressed( kbe );
    else
	kbehandler_.triggerKeyReleased( kbe );

    if ( kbehandler_.isHandled() )
	return true;

    return QObject::eventFilter( obj, ev );
}


class QtTabletEventFilter : public QObject
{
public:
			QtTabletEventFilter()
			    : mousepressed_( false )
			    , checklongleft_( false )
			    , lostreleasefixevent_( nullptr )
			    , islostreleasefixed_(false)
			{}
protected:
    bool		eventFilter(QObject*,QEvent*);

    bool		mousepressed_;
    bool		checklongleft_;

    QMouseEvent*	lostreleasefixevent_;
    bool		islostreleasefixed_;
    Qt::MouseButton	mousebutton_ = Qt::NoButton;

    Geom::Point2D<int>		lastdragpos_;
};


bool QtTabletEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    const QTabletEvent* qtabev = dynamic_cast<QTabletEvent*>( ev );
    if ( qtabev )
    {
	TabletInfo& ti = TabletInfo::latestState();

	ti.eventtype_ = (TabletInfo::EventType) qtabev->type();
	ti.pointertype_ = (TabletInfo::PointerType) qtabev->pointerType();
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
	ti.device_ = (TabletInfo::TabletDevice) qtabev->deviceType();
#else
	ti.device_ = (TabletInfo::TabletDevice) qtabev->device();
#endif
	ti.globalpos_.x = qtabev->globalX();
	ti.globalpos_.y = qtabev->globalY();
	ti.pos_.x = qtabev->x();
	ti.pos_.y = qtabev->y();
	ti.pressure_ = qtabev->pressure();
	ti.rotation_ = qtabev->rotation();
	ti.tangentialpressure_ = qtabev->tangentialPressure();
	ti.uniqueid_ = qtabev->uniqueId();
	ti.xtilt_ = qtabev->xTilt();
	ti.ytilt_ = qtabev->yTilt();
	ti.z_ = qtabev->z();

	ti.updatePressData();
	return false;		// Qt will resent it as a QMouseEvent
    }

    const QMouseEvent* qme = dynamic_cast<QMouseEvent*>( ev );
    const TabletInfo* ti = TabletInfo::currentState();

    if ( !qme )
	return false;

    // Hack to repair missing mouse release events from tablet pen on Linux
    if ( mousepressed_ && !lostreleasefixevent_ && ti && !ti->pressure_ &&
	 qme->type()!=QEvent::MouseButtonRelease )
    {
	lostreleasefixevent_ = new QMouseEvent(
					QEvent::MouseButtonRelease,
					qme->pos(), qme->globalPos(),
					mousebutton_,
					qme->buttons() & ~mousebutton_,
					qme->modifiers() );
	QApplication::postEvent( obj, lostreleasefixevent_ );
    }

    if ( qme->type()==QEvent::MouseButtonPress )
    {
	lostreleasefixevent_ = nullptr;
	islostreleasefixed_ = false;
	mousebutton_ = qme->button();
    }

    if ( qme == lostreleasefixevent_ )
    {
	if ( !mousepressed_ )
	    return true;

	islostreleasefixed_ = true;
    }
    else if ( qme->type()==QEvent::MouseButtonRelease )
    {
	if ( islostreleasefixed_ )
	    return true;
    }
    // End of hack

    // Hack to solve mouse/tablet dragging refresh problem
    if ( qme->type() == QEvent::MouseButtonPress )
	lastdragpos_ = Geom::Point2D<int>::udf();

    if ( qme->type()==QEvent::MouseMove && mousepressed_ )
    {
	const Geom::Point2D<int> curpos( qme->globalX(), qme->globalY() );
	if ( !lastdragpos_.isDefined() )
	    lastdragpos_ = curpos;
	else if ( lastdragpos_ != curpos )
	{
	    lastdragpos_ = Geom::Point2D<int>::udf();
	    return true;
	}
    }
    // End of hack

    if ( qme->type() == QEvent::MouseButtonPress )
    {
	mousepressed_ = true;
	if ( qme->button() == Qt::LeftButton )
	    checklongleft_ = true;
    }

    if ( qme->type() == QEvent::MouseButtonRelease )
    {
	mousepressed_ = false;
	checklongleft_ = false;
    }

    if ( ti && qme->type()==QEvent::MouseMove && mousepressed_ )
    {
	if ( checklongleft_ &&
	     ti->postPressTime()>500 && ti->maxPostPressDist()<5 )
	{
	    checklongleft_ = false;
	    QEvent* qev = new QEvent( mUsrEvLongTabletPress );
	    QApplication::postEvent(
				   QApplication::focusWidget(), qev );
	}

	QWidget* tlw = QApplication::topLevelAt( qme->globalPos() );
	if ( dynamic_cast<QMenu*>(tlw) )
	    return true;

	QWidget* fw = QApplication::focusWidget();
	if ( dynamic_cast<QTreeWidget*>(fw) )
	    return true;
    }

    return false;
}


const uiFont* uiMain::font_ = nullptr;
QApplication* uiMain::app_ = nullptr;
uiMain*	uiMain::themain_ = nullptr;

KeyboardEventHandler* uiMain::keyhandler_ = nullptr;
KeyboardEventFilter* uiMain::keyfilter_ = nullptr;
QtTabletEventFilter* uiMain::tabletfilter_ = nullptr;


static void initQApplication()
{
    uiMain::cleanQtOSEnv();

    ApplicationData::setOrganizationName( "dGB");
    ApplicationData::setOrganizationDomain( "opendtect.org" );
    ApplicationData::setApplicationName( "OpendTect" );

#ifdef __mac__
    ApplicationData::swapCommandAndCTRL( true );
#endif
}



static const char* getStyleFromSettings()
{
    FixedString lookpref = Settings::common().find( "dTect.LookStyle" );
    if ( lookpref.isEmpty() )
	lookpref = GetEnvVar( "OD_LOOK_STYLE" );

#ifndef QT_NO_STYLE_CDE
    if ( lookpref == "CDE" )
	return "cde";
#endif
#ifndef QT_NO_STYLE_MOTIF
    else if ( lookpref == "Motif" )
	return "motif";
#endif
#ifndef QT_NO_STYLE_WINDOWS
    else if ( lookpref == "Windows" )
	return "windows";
#endif
#ifndef QT_NO_STYLE_PLASTIQUE
    else if ( lookpref == "Plastique" )
	return "plastique";
#endif
#ifndef QT_NO_STYLE_CLEANLOOKS
    else if ( lookpref == "Cleanlooks" )
	return "cleanlooks";
#endif

    return nullptr;
}


#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
static void qtMessageOutput( QtMsgType type, const QMessageLogContext&,
			     const QString& msg )
#else
static void qtMessageOutput( QtMsgType type, const char* msg )
#endif
{
    const BufferString str( msg );
    if ( str.isEmpty() )
	return;

    static const char* suppress[] =
    {
	"KGlobal",
	"kfilemodule",
	"QXcbConnection: XCB error:",
	"QOpenGLContext::swapBuffers()",
	nullptr
    };

    switch ( type )
    {
	case QtDebugMsg:
	case QtWarningMsg:
	{
	    const char** supprptr = suppress;
	    while ( *supprptr )
	    {
		if ( str.startsWith(*supprptr) )
		    return;
		supprptr++;
	    }
	    ErrMsg( str, true );
	} break;
	case QtFatalMsg:
	case QtCriticalMsg:
	    ErrMsg( str );
	    break;
	default:
	    break;
    }
}


uiMain::uiMain( int& argc, char **argv )
    : mainobj_(nullptr)
{
#ifdef __mac__
    uiInitMac();
#endif

    initQApplication();
    init( nullptr, argc, argv );

    const QPixmap pixmap( XpmIconData );
    app_->setWindowIcon( QIcon(pixmap) );
}


uiMain::uiMain( QApplication* qapp )
    : mainobj_(nullptr)
{
    initQApplication();
    app_ = qapp;
    const QPixmap pixmap( XpmIconData );
    app_->setWindowIcon( QIcon(pixmap) );
}


uiMain::~uiMain()
{
    detachAllNotifiers();
    if ( NeedDataBase() )
	IOM().applClosing();
    delete keyhandler_;
    delete keyfilter_;
    delete tabletfilter_;
    delete app_;
}


void uiMain::cleanQtOSEnv()
{
    UnsetOSEnvVar( "QT_PLUGIN_PATH" ); //!Avoids loading incompatible plugins
}


void uiMain::preInit()
{
    QApplication::setDesktopSettingsAware( true );
    QCoreApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
    // Attributes added with Qt5.3 and Qt5.4
    QApplication::setAttribute( Qt::AA_ShareOpenGLContexts );
    if ( QApplication::testAttribute(Qt::AA_UseDesktopOpenGL) ||
	 QApplication::testAttribute(Qt::AA_UseOpenGLES) ||
	 QApplication::testAttribute(Qt::AA_UseSoftwareOpenGL) )
	return;

# ifdef __unix__
    const bool directren = directRendering();
    if ( !directren )
    {
	QApplication::setAttribute( Qt::AA_UseSoftwareOpenGL );
	/* Prevents corruption of QtWebEngine widget on remote Unix.
	   Keeping dynamic loading of DLLs on Windows for 'standards' uiMain
	 */
    }
# endif
#endif
}

static bool reqopengl = false;
void uiMain::preInitForOpenGL()
{
    reqopengl = true;
#ifdef __win__
    /*Dynamic dll loading makes OSG crash with Remote Desktop Protocol.
      Needs to set it explicitely. Sufficient for machine with Nvidia Quadro,
      but not with GeForce cards.
      */
# if QT_VERSION >= QT_VERSION_CHECK(5,3,0)
    QApplication::setAttribute( Qt::AA_UseDesktopOpenGL );
# endif
#endif
}


bool uiMain::reqOpenGL()
{
    return reqopengl;
}


void uiMain::init( QApplication* qap, int& argc, char **argv )
{
    QLocale::setDefault( QLocale::c() );
    if ( app_ )
	{ pErrMsg("You already have a uiMain object!"); return; }
    themain_ = this;

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "Constructing QApplication ..." );

    preInit();
    app_ = qap ? qap : new QApplication( argc, argv );

    KeyboardEventHandler& kbeh = keyboardEventHandler();
    keyfilter_ = new KeyboardEventFilter( kbeh );
    app_->installEventFilter( keyfilter_ );

    tabletfilter_ = new QtTabletEventFilter();
    app_->installEventFilter( tabletfilter_ );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "... done." );

#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    qInstallMessageHandler( qtMessageOutput );
#else
    qInstallMsgHandler( qtMessageOutput );
#endif

#ifndef __win__
    BufferString stylestr = getStyleFromSettings();
    if ( stylestr.isEmpty() )
	stylestr = __ismac__ ? "macintosh" : "cleanlooks";

    QApplication::setStyle( QStyleFactory::create(stylestr.buf()) );
#endif

    BufferString qssfnm = Settings::common().find( "dTect.StyleSheet" );
    if ( qssfnm.isEmpty() )
	qssfnm = GetEnvVar( "OD_STYLESHEET" );

    if ( File::exists(qssfnm) )
    {
	QFile file( qssfnm.buf() );
	file.open( QFile::ReadOnly );
	QString sheet = QLatin1String( file.readAll() );
	app_->setStyleSheet( sheet );
    }

    qApp->setStyleSheet(
	QString("[readOnly=\"true\"] { background-color: %0 }")
	    .arg(qApp->palette().color(QPalette::Window).name(QColor::HexRgb)));

    font_ = nullptr;
    setFont( *font() , true );
}


void uiMain::restart()
{
    RestartProgram();
}


int uiMain::exec()
{
    if ( !app_ )
	{ pErrMsg("Huh?") ; return -1; }

    const int ret = app_->exec();
    return ret;
}


void uiMain::exit( int retcode )
{
    if ( app_ )
	app_->exit( retcode );
    else
	{ pErrMsg( "Huh?" ); return; }
}


/*!<\brief Tells the application to exit with a return code.

    After this function has been called, the application leaves the main
    event loop and returns from the call to exec(). The exec() function
    returns retcode.

    By convention, retcode 0 means success, any non-zero value indicates
    an error.

    Note that unlike the C library exit function, this function does
    return to the caller - it is event processing that stops
*/


void* uiMain::thread()
{
    return qApp ? qApp->thread() : nullptr;
}


void uiMain::getCmdLineArgs( BufferStringSet& args ) const
{
    QStringList qargs = app_->arguments();
    for ( int idx=0; idx<qargs.count(); idx++ )
	args.add( qargs.at(idx) );
}


void uiMain::setTopLevel( uiMainWin* obj )
{
    if ( !obj || !app_ )
    {
	if ( !app_ )
	    { pErrMsg("Huh?"); }
	return;
    }

    if ( mainobj_ )
	mainobj_->setExitAppOnClose( false );
    obj->setExitAppOnClose( true );

    mainobj_ = obj;
    init( mainobj_->body()->qwidget() ); // inits SoQt if uicMain
}


void uiMain::setFont( const uiFont& fnt, bool PassToChildren )
{
    font_ = &fnt;
    if ( !app_ )
	{ pErrMsg("Huh?"); return; }
    app_->setFont( font_->qFont() );
}


const uiFont* uiMain::font()
{
    if ( !font_ )
	font_ = &FontList().get( FontData::Control );
    return font_;
}


OD::Color uiMain::windowColor() const
{
    const QColor& qcol =
	 QApplication::palette().color( QPalette::Window );
    return OD::Color( qcol.red(), qcol.green(), qcol.blue() );
}


void uiMain::setIcon( const char* iconnm )
{
    uiIcon icon( iconnm );
    if ( app_ )
	app_->setWindowIcon( icon.qicon() );
}


int uiMain::nrScreens() const
{
    return QGuiApplication::screens().size();
}


uiSize uiMain::getScreenSize( int screennr, bool available ) const
{
    QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.isEmpty() || screennr<0 || screennr>=screens.size() )
        return uiSize( mUdf(int), mUdf(int) );

    QScreen* qscreen = screens.at( screennr );
    QRect qrect = available ? qscreen->availableGeometry()
                            : qscreen->geometry();
    return uiSize( qrect.width(), qrect.height() );
}


uiSize uiMain::desktopSize() const
{
    if ( !app_ || !app_->desktop() )
	return uiSize( mUdf(int), mUdf(int) );

    return uiSize( app_->desktop()->width(), app_->desktop()->height() );
}


double uiMain::getDevicePixelRatio( int screennr ) const
{
    const QList<QScreen*> screens = QGuiApplication::screens();
    if ( screens.isEmpty() || screennr<0 || screennr>=screens.size() )
	return mUdf(double);

    const QScreen* qscreen = screens.at( screennr );
    if ( !qscreen )
	return mUdf(double);

    return mCast(double,qscreen->logicalDotsPerInchX()) /
	   mCast(double,qscreen->physicalDotsPerInchX());
}


uiMain& uiMain::theMain()
{
    if ( themain_ )
	return *themain_;

    if ( !qApp )
    {
	pFreeFnErrMsg("FATAL: no uiMain and no qApp." );
	QApplication::exit( -1 );
    }

    themain_ = new uiMain( qApp );
    return *themain_;
}


KeyboardEventHandler& uiMain::keyboardEventHandler()
{
    if ( !keyhandler_ )
	keyhandler_ = new KeyboardEventHandler();

    return *keyhandler_;
}


void uiMain::repaint()
{
    if ( app_ )
	app_->sendPostedEvents( nullptr, QEvent::Paint );
}


IdxPair uiMain::getDPI()
{
    return IdxPair( QApplication::desktop()->physicalDpiX(),
		    QApplication::desktop()->physicalDpiY() );
}


int uiMain::getMinDPI()
{
    const IdxPair dpi = getDPI();
    return dpi.first < dpi.second ? dpi.first : dpi.second;
}


double uiMain::getDefZoomLevel()
{
    mDefineStaticLocalObject(double, defzoom,
			= GetEnvVarDVal("OD_QTWEBENGINE_ZOOM",mUdf(double)) );
    if ( !mIsUdf(defzoom) )
	return defzoom;

    return 1.;
}


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{
    if ( app_ )
	app_->processEvents( QEventLoop::AllEvents, msec );
}


static bool usenametooltip_ = false;
static OD::Color normaltooltipbackgroundcolor_;
static OD::Color normaltooltipforegroundcolor_;

void uiMain::useNameToolTip( bool yn )
{
    if ( usenametooltip_ == yn )
	return;

    OD::Color bg( normaltooltipbackgroundcolor_ );
    OD::Color fg( normaltooltipforegroundcolor_ );
    if ( yn )
    {
	bg = OD::Color( 220, 255, 255 );//Pale cyan (to differ from pale yellow)
	fg = OD::Color::Black();

	normaltooltipbackgroundcolor_ =
	    OD::Color( QToolTip::palette().color(QPalette::ToolTipBase).rgb() );
	normaltooltipforegroundcolor_ =
	    OD::Color( QToolTip::palette().color(QPalette::ToolTipText).rgb() );
    }

    QPalette palette;
    palette.setColor( QPalette::ToolTipBase, QColor(bg.r(),bg.g(),bg.b()) );
    palette.setColor( QPalette::ToolTipText, QColor(fg.r(),fg.g(),fg.b()) );
    QToolTip::setPalette( palette );

    usenametooltip_ = yn;
    uiObject::updateToolTips();
    uiAction::updateToolTips();
    uiTreeViewItem::updateToolTips();
}


bool uiMain::isNameToolTipUsed()
{
    return usenametooltip_;
}


void uiMain::formatNameToolTipString( BufferString& namestr )
{
    BufferString bufstr( namestr );
    bufstr.replace( "&&", "\a" );
    bufstr.remove( '&' );
    bufstr.replace( '\a', '&' );

    namestr = "\""; namestr += bufstr; namestr += "\"";
}


static Threads::Atomic<int> directrendering( 0 );
//0 = don't know
//1 = yes
//-1 = no

bool uiMain::directRendering()
{
    if ( directrendering )
	return directrendering == 1;

    OS::MachineCommand cmd( "od_glxinfo" );
    BufferString stdoutstr;
    if ( !cmd.execute(stdoutstr) || stdoutstr.isEmpty() )
	return false;

    BufferStringSet glxinfostrs;
    glxinfostrs.unCat( stdoutstr.str() );
    BufferString dorender;
    for ( const auto line : glxinfostrs )
    {
	if ( !line->startsWith("direct rendering:") )
	    continue;

	dorender.set( line->find( ':' )+1 );
	dorender.trimBlanks();
	if ( !dorender.isEmpty() )
	{
	    directrendering = dorender == FixedString("Yes")
			    ? 1
			    : -1;
	}
    }

    return directrendering == 1;
}


bool isMainThread(Threads::ThreadID thread)
{
    return uiMain::theMain().thread() == thread;
}


bool isMainThreadCurrent()
{
    return isMainThread( Threads::currentThread() );
}
