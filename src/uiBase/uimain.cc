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
			    , lostreleasefixevent_( 0 )
			{}
protected:
    bool		eventFilter(QObject*,QEvent*);

    bool		mousepressed_;
    bool		checklongleft_;

    QMouseEvent*	lostreleasefixevent_;
    bool		islostreleasefixed_;
    Qt::MouseButton	mousebutton_;

    Geom::Point2D<int>		lastdragpos_;
};


bool QtTabletEventFilter::eventFilter( QObject* obj, QEvent* ev )
{
    const QTabletEvent* qte = dynamic_cast<QTabletEvent*>( ev );
    if ( qte )
    {
	TabletInfo& ti = TabletInfo::latestState();

	ti.eventtype_ = (TabletInfo::EventType) qte->type();
	ti.pointertype_ = (TabletInfo::PointerType) qte->pointerType();
	ti.device_ = (TabletInfo::TabletDevice) qte->device();
	ti.globalpos_.x_ = qte->globalX();
	ti.globalpos_.y_ = qte->globalY();
	ti.pos_.x_ = qte->x();
	ti.pos_.y_ = qte->y();
	ti.pressure_ = qte->pressure();
	ti.rotation_ = qte->rotation();
	ti.tangentialpressure_ = qte->tangentialPressure();
	ti.uniqueid_ = qte->uniqueId();
	ti.xtilt_ = qte->xTilt();
	ti.ytilt_ = qte->yTilt();
	ti.z_ = qte->z();

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


const uiFont* uiMain::font_ = 0;
QApplication* uiMain::app_ = 0;
uiMain*	uiMain::themain_ = 0;
KeyboardEventHandler* uiMain::keyhandler_ = 0;
KeyboardEventFilter* uiMain::keyfilter_ = 0;
QtTabletEventFilter* uiMain::tabletfilter_ = 0;
static BufferString app_name_( "OpendTect" );


static void initQApplication()
{
    uiMain::cleanQtOSEnv();
    QApplication::setDesktopSettingsAware( true );

    ApplicationData::setOrganizationName( "dGB");
    ApplicationData::setOrganizationDomain( "opendtect.org" );
    ApplicationData::setApplicationName( app_name_.str() );

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


#if QT_VERSION >= 0x050000
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


uiMain::uiMain()
    : clp_(new CommandLineParser)
{
    OD::uiInitProcessStatus();

    initQApplication();
    init( 0 );

    if ( setAppIcon(app_) )
	qdesktop_ = app_->desktop();
}


uiMain::uiMain( QApplication* qapp )
    : clp_(new CommandLineParser)
{
    OD::uiInitProcessStatus();

    initQApplication();
    init( qapp );
    app_ = qapp;
    if ( setAppIcon(app_) )
	qdesktop_ = app_->desktop();
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


void uiMain::setAppName( const char* appnm )
{
    app_name_ = appnm;
}


void uiMain::cleanQtOSEnv()
{
    UnsetOSEnvVar( "QT_PLUGIN_PATH" ); //!Avoids loading incompatible plugins
}


void uiMain::init( QApplication* qap )
{
    if ( app_ )
	{ pErrMsg("You already have a uiMain object!"); return; }

    themain_ = this;

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "Constructing QApplication ..." );

    app_ = qap ? qap : new QApplication( GetArgC(), GetArgV() );

    KeyboardEventHandler& kbeh = keyboardEventHandler();
    keyfilter_ = new KeyboardEventFilter( kbeh );
    app_->installEventFilter( keyfilter_ );

    tabletfilter_ = new QtTabletEventFilter();
    app_->installEventFilter( tabletfilter_ );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "... done." );

#if QT_VERSION >= 0x050000
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

    const BufferString stylenm = OD::getActiveStyleName();
    const BufferString qssfnm = OD::getStyleFile( stylenm, "qss" );
    if ( !qssfnm.isEmpty() )
	setStyleSheet( qssfnm );

    font_ = 0;
    setFont( *font() , true );

    OD::loadLocalization();
    mAttachCB( TrMgr().languageChange, uiMain::languageChangeCB );
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
{ return qdesktop_ ? qdesktop_->screenCount() : -1; }


const char* uiMain::getScreenName( int screenidx ) const
{
    mDeclStaticString(screennm);
    screennm.set( "Screen " ).add( screenidx );
#if QT_VERSION >= 0x050000
    QList<QScreen*> screens = QGuiApplication::screens();
    if ( !screens.isEmpty() && screenidx>=0 && screenidx<screens.size() )
    {
	QScreen* qscreen = screens.at( screenidx );
	screennm = qscreen->name();
    }
#endif

    return screennm.buf();
}


uiSize uiMain::getScreenSize( int screennr, bool available ) const
{
    if ( !qdesktop_ )
	return uiSize( mUdf(int), mUdf(int) );

    QRect qrect = available ? qdesktop_->availableGeometry( screennr )
			    : qdesktop_->screenGeometry( screennr );
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
	app_->flush();
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
