/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimain.h"

#include "uifont.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uiobjbody.h"
#include "uiaction.h"
#include "uitreeview.h"

#include "bufstringset.h"
#include "debug.h"
#include "envvars.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "oddirs.h"
#include "settings.h"
#include "thread.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QMenu>
#include <QKeyEvent>
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
	ti.globalpos_.x = qte->globalX();
	ti.globalpos_.y = qte->globalY();
	ti.pos_.x = qte->x();
	ti.pos_.y = qte->y();
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


#if QT_VERSION >= 0x050000
void myMessageOutput( QtMsgType, const QMessageLogContext&, const QString& );
#else
void myMessageOutput( QtMsgType type, const char* msg );
#endif


const uiFont* uiMain::font_ = 0;
QApplication* uiMain::app_ = 0;
uiMain*	uiMain::themain_ = 0;

KeyboardEventHandler* uiMain::keyhandler_ = 0;
KeyboardEventFilter* uiMain::keyfilter_ = 0;
QtTabletEventFilter* uiMain::tabletfilter_ = 0;


static void initQApplication()
{
    QApplication::setDesktopSettingsAware( true );

    QCoreApplication::setOrganizationName( "dGB");
    QCoreApplication::setOrganizationDomain( "opendtect.org" );
    QCoreApplication::setApplicationName( "OpendTect" );
#ifdef __mac__

    QCoreApplication::setAttribute(
				Qt::AA_MacDontSwapCtrlAndMeta, true );
#endif

#ifndef __win__
    QCoreApplication::addLibraryPath( GetBinPlfDir() ); // Qt plugin libraries
#endif
}


uiMain::uiMain( int& argc, char **argv )
    : mainobj_( 0 )
{
#ifdef __mac__
    uiInitMac();
#endif

    initQApplication();
    init( 0, argc, argv );

    const QPixmap pixmap( XpmIconData );
    app_->setWindowIcon( QIcon(pixmap) );
}


uiMain::uiMain( QApplication* qapp )
    : mainobj_( 0 )
{
    initQApplication();
    app_ = qapp;
    const QPixmap pixmap( XpmIconData );
    app_->setWindowIcon( QIcon(pixmap) );
}


static const char* getStyleFromSettings()
{
    const char* lookpref = Settings::common().find( "dTect.LookStyle" );
    if ( !lookpref || !*lookpref )
	lookpref = GetEnvVar( "OD_LOOK_STYLE" );

    if ( lookpref && *lookpref )
    {
#ifndef QT_NO_STYLE_CDE
	if ( !strcmp(lookpref,"CDE") )
	    return "cde";
#endif
#ifndef QT_NO_STYLE_MOTIF
	if ( !strcmp(lookpref,"Motif") )
	    return "motif";
#endif
#ifndef QT_NO_STYLE_WINDOWS
	if ( !strcmp(lookpref,"Windows") )
	    return "windows";
#endif
#ifndef QT_NO_STYLE_PLASTIQUE
	if ( !strcmp(lookpref,"Plastique") )
	    return "plastique";
#endif
#ifndef QT_NO_STYLE_CLEANLOOKS
	if ( !strcmp(lookpref,"Cleanlooks") )
	    return "cleanlooks";
#endif
    }

    return 0;
}


void uiMain::init( QApplication* qap, int& argc, char **argv )
{
    if ( app_ )
    {
	pErrMsg("You already have a uiMain object!");
	return;
    }
    else
	themain_ = this;

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "Constructing QApplication ..." );

    if ( qap )
	app_ = qap;
    else
	app_ = new QApplication( argc, argv );

    KeyboardEventHandler& kbeh = keyboardEventHandler();
    keyfilter_ = new KeyboardEventFilter( kbeh );
    app_->installEventFilter( keyfilter_ );

    tabletfilter_ = new QtTabletEventFilter();
    app_->installEventFilter( tabletfilter_ );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "... done." );

#if QT_VERSION >= 0x050000
    qInstallMessageHandler( myMessageOutput );
#else
    qInstallMsgHandler( myMessageOutput );
#endif

    BufferString stylestr = getStyleFromSettings();
#ifdef __win__
    if ( stylestr.isEmpty() )
	stylestr = QSysInfo::WindowsVersion == QSysInfo::WV_VISTA
		? "windowsvista" : "windowsxp";
#else
# ifdef __mac__
    if ( stylestr.isEmpty() )
	stylestr = "macintosh";
# else
    if ( stylestr.isEmpty() )
	stylestr = "cleanlooks";
# endif
#endif

    QApplication::setStyle( QStyleFactory::create(stylestr.buf()) );
    font_ = 0;
    setFont( *font() , true );
}


uiMain::~uiMain()
{
    delete app_;
    delete font_;

    delete keyhandler_;
    delete keyfilter_;
    delete tabletfilter_;
}


int uiMain::exec()
{
    if ( !app_ )  { pErrMsg("Huh?") ; return -1; }
    return app_->exec();
}


void* uiMain::thread()
{ return qApp ? qApp->thread() : 0; }


void uiMain::getCmdLineArgs( BufferStringSet& args ) const
{
    QStringList qargs = app_->arguments();
    for ( int idx=0; idx<qargs.count(); idx++ )
	args.add( qargs.at(idx) );
}


void uiMain::setTopLevel( uiMainWin* obj )
{
    if ( !obj ) return;
    if ( !app_ )  { pErrMsg("Huh?") ; return; }

    if ( mainobj_ ) mainobj_->setExitAppOnClose( false );
    obj->setExitAppOnClose( true );

    mainobj_ = obj;
    init( mainobj_->body()->qwidget() ); // inits SoQt if uicMain
}


void uiMain::setFont( const uiFont& fnt, bool PassToChildren )
{
    font_ = &fnt;
    if ( !app_ )  { pErrMsg("Huh?") ; return; }
    app_->setFont( font_->qFont() );
}


void uiMain::exit( int retcode )
{
    if ( !app_ )  { pErrMsg("Huh?") ; return; }
    app_->exit( retcode );
}
/*!<
    \brief Tells the application to exit with a return code.

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
	{ font_ = &FontList().get( "Control" );  }

    return font_;
}


Color uiMain::windowColor() const
{
    const QColor& qcol =
	 QApplication::palette().color( QPalette::Window );
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


uiSize uiMain::desktopSize() const
{
    if ( !app_ || !app_->desktop() )
	return uiSize( mUdf(int), mUdf(int) );

    return uiSize( app_->desktop()->width(), app_->desktop()->height() );
}


uiMain& uiMain::theMain()
{
    if ( themain_ ) return *themain_;
    if ( !qApp )
    {
	pFreeFnErrMsg("FATAL: no uiMain and no qApp.","uiMain::theMain()");
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
{ if ( app_ ) app_->flush(); }


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{ if ( app_ ) app_->processEvents( QEventLoop::AllEvents, msec ); }


static bool usenametooltip_ = false;
static Color normaltooltipcolor_;

void uiMain::useNameToolTip( bool yn )
{
    if ( usenametooltip_ == yn )
	return;

    Color col( normaltooltipcolor_ );
    if ( yn )
    {
	col = Color( 220, 255, 255 ); // Pale cyan (to differ from pale yellow)
	normaltooltipcolor_ =
	    Color( QToolTip::palette().color(QPalette::ToolTipBase).rgb() );
    }

    QPalette palette;
    palette.setColor( QPalette::ToolTipBase, QColor(col.r(),col.g(),col.b()) );
    QToolTip::setPalette( palette );

    usenametooltip_ = yn;
    uiObject::updateToolTips();
    uiAction::updateToolTips();
    uiTreeViewItem::updateToolTips();
}


bool uiMain::isNameToolTipUsed()
{ return usenametooltip_; }


void uiMain::formatNameToolTipString( BufferString& namestr )
{
    BufferString bufstr( namestr );
    bufstr.replace( "&&", "\a" );
    bufstr.remove( '&' );
    bufstr.replace( '\a', '&' );

    namestr = "\""; namestr += bufstr; namestr += "\"";
}


#if QT_VERSION >= 0x050000
void myMessageOutput( QtMsgType type, const QMessageLogContext&,
		      const QString& msg )
#else
void myMessageOutput( QtMsgType type, const char* msg )
#endif
{
    const BufferString str( msg );
    switch ( type ) {
	case QtDebugMsg:
	    ErrMsg( str, true );
	    break;
	case QtWarningMsg:
	    ErrMsg( str, true );
	    break;
	case QtFatalMsg:
	    ErrMsg( str );
	    break;
	case QtCriticalMsg:
	    ErrMsg( str );
	    break;
	default:
	    break;
    }
}


bool isMainThread( const void* thread )
{ return uiMain::theMain().thread() == thread; }

bool isMainThreadCurrent()
{ return isMainThread( Threads::currentThread() ); }
