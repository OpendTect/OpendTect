/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/

#include "uimain.h"

#include "uifont.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uiobjbody.h"
#include "uiaction.h"
#include "uitreeview.h"
#include "uiprocessinit.h"

#include "applicationdata.h"
#include "bufstringset.h"
#include "commandlineparser.h"
#include "debug.h"
#include "envvars.h"
#include "file.h"
#include "genc.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "oddirs.h"
#include "odiconfile.h"
#include "oscommand.h"
#include "staticstring.h"
#include "settings.h"
#include "thread.h"
#include "texttranslation.h"
#include "uistrings.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QMenu>
#include <QKeyEvent>
#include <QScreen>
#include <QStyleFactory>
#include <QTreeWidget>

static BufferString icon_filename;
static const char** xpm_icon_data = 0;
static bool usenametooltip_ = false;
static Color normaltooltipbackgroundcolor_;
static Color normaltooltipforegroundcolor_;

void uiMain::setXpmIconData( const char** buf )	{ xpm_icon_data = buf; }
const char* uiMain::iconFileName()		{ return icon_filename; }
namespace OD { mGlobal(Basic) void loadLocalization(); }
mGlobal(Basic) void SetArgcAndArgv(int,char**);


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
			   : QObject()
			{}
protected:
    bool		eventFilter(QObject*,QEvent*);

    bool		mousepressed_ = false;
    bool		checklongleft_ = false;

    QMouseEvent*	lostreleasefixevent_ = 0;
    bool		islostreleasefixed_ = false;
    Qt::MouseButton	mousebutton_;

    Geom::Point2D<int>		lastdragpos_;
};


bool QtTabletEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    if ( !ev )
	return false;

    const QEvent::Type evtyp = ev->type();
    if ( evtyp >= QEvent::User )
	return false;

    const QChildEvent* qchildev = dynamic_cast<QChildEvent*>( ev );
    if ( qchildev )
	return false;

    const QTabletEvent* qtabev = dynamic_cast<QTabletEvent*>( ev );
    if ( qtabev )
    {
	TabletInfo& ti = TabletInfo::latestState();

	ti.eventtype_ = (TabletInfo::EventType) evtyp;
	ti.pointertype_ = (TabletInfo::PointerType) qtabev->pointerType();
	ti.device_ = (TabletInfo::TabletDevice) qtabev->device();
	ti.globalpos_.x_ = qtabev->globalX();
	ti.globalpos_.y_ = qtabev->globalY();
	ti.pos_.x_ = qtabev->x();
	ti.pos_.y_ = qtabev->y();
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
	 evtyp != QEvent::MouseButtonRelease )
    {
	lostreleasefixevent_ = new QMouseEvent(
					QEvent::MouseButtonRelease,
					qme->pos(), qme->globalPos(),
					mousebutton_,
					qme->buttons() & ~mousebutton_,
					qme->modifiers() );
	QApplication::postEvent( obj, lostreleasefixevent_ );
    }

    if ( evtyp==QEvent::MouseButtonPress )
    {
	lostreleasefixevent_ = 0;
	islostreleasefixed_ = false;
	mousebutton_ = qme->button();
    }

    if ( qme == lostreleasefixevent_ )
    {
	if ( !mousepressed_ )
	    return true;

	islostreleasefixed_ = true;
    }
    else if ( evtyp==QEvent::MouseButtonRelease )
    {
	if ( islostreleasefixed_ )
	    return true;
    }
    // End of hack

    // Hack to solve mouse/tablet dragging refresh problem
    if ( evtyp==QEvent::MouseButtonPress )
	lastdragpos_.setUdf();

    if ( evtyp==QEvent::MouseMove && mousepressed_ )
    {
	const Geom::Point2D<int> curpos( qme->globalX(), qme->globalY() );
	if ( !lastdragpos_.isDefined() )
	    lastdragpos_ = curpos;
	else if ( lastdragpos_ != curpos )
	    { lastdragpos_.setUdf(); return true; }
    }
    // End of hack

    if ( evtyp == QEvent::MouseButtonPress )
    {
	mousepressed_ = true;
	if ( qme->button() == Qt::LeftButton )
	    checklongleft_ = true;
    }

    if ( evtyp == QEvent::MouseButtonRelease )
    {
	mousepressed_ = false;
	checklongleft_ = false;
    }

    if ( ti && evtyp==QEvent::MouseMove && mousepressed_ )
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


const uiFont* uiMain::font_ = 0;
QApplication* uiMain::app_ = 0;
uiMain*	uiMain::themain_ = 0;
KeyboardEventHandler* uiMain::keyhandler_ = 0;
KeyboardEventFilter* uiMain::keyfilter_ = 0;
QtTabletEventFilter* uiMain::tabletfilter_ = 0;
static const uiString dontchange( toUiString("-") );


static void initQApplication( uiString appnm, uiString orgnm )
{
    uiMain::cleanQtOSEnv();
    QApplication::setDesktopSettingsAware( true );

    if ( orgnm != dontchange )
    {
	if ( orgnm.isEmpty() )
	    orgnm = uiStrings::sdGB();
	ApplicationData::setOrganizationName( orgnm );
    }
    if ( appnm != dontchange )
    {
	if ( appnm.isEmpty() )
	    appnm = uiStrings::sOpendTect();
	ApplicationData::setApplicationName( appnm );
    }

    ApplicationData::setOrganizationDomain( "opendtect.org" );

#ifdef __mac__
    ApplicationData::swapCommandAndCTRL( true );
#endif
}


static bool setAppIcon( QApplication* app )
{
    if ( !app )
	{ pFreeFnErrMsg( "No QApplication!" ); return false; }

    if ( !xpm_icon_data && !File::exists(icon_filename) )
	icon_filename = GetSetupDataFileName(
				ODSetupLoc_ApplSetupPref, "od.svg", true );

    if ( File::exists(icon_filename) )
	app->setWindowIcon( QIcon(QString(icon_filename.str())) );
    else if ( !xpm_icon_data )
	pFreeFnErrMsg( "No application icon available" );
    else
    {
	pFreeFnErrMsg( "XPM icons don't scale. Try uiMain::setIconFileName" );
	const QPixmap pixmap( xpm_icon_data );
	app->setWindowIcon( QIcon(pixmap) );
    }

    return true;
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

    return 0;
}


static void qtMessageOutput( QtMsgType type, const QMessageLogContext&,
			     const QString& msg )
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
	0
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


uiMain::uiMain( const uiString& appnm, const uiString& orgnm )
    : clp_(new CommandLineParser)
{
    OD::uiInitProcessStatus();
    init( 0 );
    initQApplication( appnm, orgnm );
}


uiMain::uiMain( QApplication* qapp )
    : clp_(new CommandLineParser)
{
    OD::uiInitProcessStatus();
    init( qapp );
    initQApplication( dontchange, dontchange );
}


uiMain::~uiMain()
{
    detachAllNotifiers();
    delete keyhandler_;
    delete keyfilter_;
    delete tabletfilter_;
    delete app_;
    delete clp_;
}


void uiMain::cleanQtOSEnv()
{
    UnsetOSEnvVar( "QT_PLUGIN_PATH" ); //!Avoids loading incompatible plugins
}


void uiMain::init( QApplication* qap )
{
    QLocale::setDefault( QLocale::c() );
    if ( app_ )
	{ pErrMsg("You already have a uiMain object!"); return; }
    themain_ = this;

    if ( qap )
	app_ = qap;
    else
    {
	if ( DBG::isOn(DBG_UI) )
	    DBG::message( "Constructing QApplication ..." );

	SetArgcAndArgv( clp_->getArgc(), clp_->getArgv() );
	app_ = new QApplication( GetArgC(), GetArgV() );
    }

    KeyboardEventHandler& kbeh = keyboardEventHandler();
    keyfilter_ = new KeyboardEventFilter( kbeh );
    app_->installEventFilter( keyfilter_ );

    tabletfilter_ = new QtTabletEventFilter();
    app_->installEventFilter( tabletfilter_ );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( " done." );

    qInstallMessageHandler( qtMessageOutput );

#ifndef __win__
    BufferString stylestr = getStyleFromSettings();
    if ( stylestr.isEmpty() )
	stylestr = __ismac__ ? "macintosh" : "cleanlooks";

    if ( !stylestr.isEmpty() && QStyleFactory::keys().contains(stylestr.str()) )
	QApplication::setStyle( QStyleFactory::create(stylestr.str()) );
#endif

    const BufferString stylenm = OD::getActiveStyleName();
    const BufferString qssfnm = OD::getStyleFile( stylenm, "qss" );
    if ( !qssfnm.isEmpty() )
	setStyleSheet( qssfnm );

    font_ = 0;
    setFont( *font() , true );

    OD::loadLocalization();
    mAttachCB( TrMgr().languageChange, uiMain::languageChangeCB );

    if ( setAppIcon(app_) )
	qdesktop_ = app_->desktop();
}


bool uiMain::setStyleSheet( const char* fnm )
{
    QFile file( fnm );
    if ( !file.open( QFile::ReadOnly| QFile::Text ) )
	return false;
    QString filecontents = QLatin1String( file.readAll() );
    if ( filecontents.isEmpty() )
	return false;

    app_->setStyleSheet( filecontents );
    return true;
}


void uiMain::setIcon( const char* icid )
{
    File::Path fp( icid );
    QIcon qic;
    if ( fp.isAbsolute() )
    {
	qic.addFile( icid );
	icon_filename = icid;
    }
    else
    {
	OD::IconFile icfil( icid );
	const BufferStringSet& fnms = icfil.fileNames();
	if ( fnms.isEmpty() )
	    return;
	for ( int idx=0; idx<fnms.size(); idx++ )
	    qic.addFile( fnms.get(idx).buf() );
	icon_filename = fnms.get(0);
    }

    if ( app_ )
	app_->setWindowIcon( qic );
}


int uiMain::exec()
{
    if ( !app_ )
	{ pErrMsg("Huh?") ; return -1; }
    return app_->exec();
}


void* uiMain::thread()
{
    return qApp ? qApp->thread() : 0;
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
}


void uiMain::setFont( const uiFont& fnt, bool PassToChildren )
{
    font_ = &fnt;
    if ( !app_ )
	{ pErrMsg("Huh?"); return; }
    app_->setFont( font_->qFont() );
}


void uiMain::restart()
{
    RestartProgram();
}


void uiMain::exit( int retcode )
{
    if ( app_ )
	app_->exit( retcode );
    else
	{ pErrMsg("Huh? No app_?"); }

    ExitProgram( retcode ); // This should only be reached if !app_
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


const uiFont* uiMain::font()
{
    if ( !font_ )
	font_ = &FontList().get( FontData::Control );
    return font_;
}


Color uiMain::windowColor() const
{
    const QColor& qcol =
	 QApplication::palette().color( QPalette::Window );
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


int uiMain::nrScreens() const
{
    return QGuiApplication::screens().size();
}


const char* uiMain::getScreenName( int screenidx ) const
{
    mDeclStaticString(screennm);
    screennm.set( "Screen " ).add( screenidx );
    QList<QScreen*> screens = QGuiApplication::screens();
    if ( !screens.isEmpty() && screenidx>=0 && screenidx<screens.size() )
    {
	QScreen* qscreen = screens.at( screenidx );
	screennm = qscreen->name();
    }

    return screennm.buf();
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


void uiMain::flushX()
{
    if ( app_ )
    {
	app_->sendPostedEvents();
	app_->processEvents();
    }
}


int uiMain::getDPI()
{
    const int xdpi = QApplication::desktop()->physicalDpiX();
    const int ydpi = QApplication::desktop()->physicalDpiY();
    return xdpi > ydpi ? ydpi : xdpi;
}


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{
    if ( app_ )
	app_->processEvents( QEventLoop::AllEvents, msec );
}


void uiMain::languageChangeCB( CallBacker* )
{
    updateAllToolTips();
    uiAction::updateAllTexts();
}


void uiMain::updateAllToolTips()
{
    uiObject::updateAllToolTips();
    uiTreeViewItem::updateAllToolTips();
    uiAction::updateAllToolTips();
}


void uiMain::useNameToolTip( bool yn )
{
    if ( usenametooltip_ == yn )
	return;

    usenametooltip_ = yn;
    updateAllToolTips();
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


bool isMainThread(Threads::ThreadID thread)
{
    return uiMain::theMain().thread() == thread;
}


bool isMainThreadCurrent()
{
    return isMainThread( Threads::currentThread() );
}
