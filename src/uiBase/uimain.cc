/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimain.cc,v 1.62 2011-01-18 13:12:45 cvsbert Exp $";

#include "uimain.h"

#include "uifont.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "uiobjbody.h"

#include "bufstringset.h"
#include "debugmasks.h"
#include "envvars.h"
#include "errh.h"
#include "keyboardevent.h"
#include "mouseevent.h"
#include "oddirs.h"
#include "settings.h"
#include "thread.h"
#include "timefun.h"

#include <QApplication>
#include <QKeyEvent>
#include <QIcon>
#include <QCDEStyle>
#include <QWindowsStyle>
#include <QPlastiqueStyle>
#include <QGtkStyle>

#include <QTreeWidget>
#include <QMenu>

#include "dtect.xpm"
const char** uiMain::XpmIconData = dtect_xpm_data;
void uiMain::setXpmIconData( const char** xpmdata )
{ 
    XpmIconData = xpmdata;
}

#ifdef __win__
#include <QWindowsVistaStyle>
#endif
#ifdef __mac__
# define __machack__
#include <QMacStyle>
#endif

#ifdef __machack__
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>

extern "C"
{

typedef struct CPSProcessSerNum
{
        UInt32          lo;
        UInt32          hi;
} CPSProcessSerNum;

extern OSErr    CPSGetCurrentProcess(CPSProcessSerNum *);
extern OSErr    CPSEnableForegroundOperation( CPSProcessSerNum *,
                                              UInt32, UInt32, UInt32, UInt32);
extern OSErr    CPSSetFrontProcess(CPSProcessSerNum *);
extern OSErr    NativePathNameToFSSpec(char *, FSSpec *, unsigned long);

}

#endif


mClass KeyboardEventFilter : public QObject
{
public:
    			KeyboardEventFilter(KeyboardEventHandler& kbeh)
			    : kbehandler_(kbeh)				{}
protected:
    bool		eventFilter(QObject*,QEvent*);

    KeyboardEventHandler& kbehandler_;
};


bool KeyboardEventFilter::eventFilter( QObject* obj, QEvent* event )
{
    if ( event->type()!=QEvent::KeyPress && event->type()!=QEvent::KeyRelease )
	return false;

    const QKeyEvent* qke = dynamic_cast<QKeyEvent*>( event );
    if ( !qke ) return false;

    KeyboardEvent kbe;
    kbe.key_ = (OD::KeyboardKey) qke->key();
    kbe.modifier_ = OD::ButtonState( (int) qke->modifiers() );
    kbe.isrepeat_ = qke->isAutoRepeat();

    if ( event->type() == QEvent::KeyPress )
	kbehandler_.triggerKeyPressed( kbe );
    else 
	kbehandler_.triggerKeyReleased( kbe );

    if ( kbehandler_.isHandled() )
	return true;

    return QObject::eventFilter( obj, event );
}


mClass QtTabletEventFilter : public QObject
{
public:
    			QtTabletEventFilter()
			    : longleftstamp_( mUdf(int) )
			    , mousepressed_( false )
			{}
protected:
    bool		eventFilter(QObject*,QEvent*);

    int			longleftstamp_;
    bool		mousepressed_;
};


bool QtTabletEventFilter::eventFilter( QObject* obj, QEvent* event )
{
    const QTabletEvent* qte = dynamic_cast<QTabletEvent*>( event );
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

	return false;		// Qt will resent it as a QMouseEvent
    }

    const QMouseEvent* qme = dynamic_cast<QMouseEvent*>( event );
    if ( !qme || !TabletInfo::currentState() )
	return false;

    if ( !TabletInfo::currentState()->pressure_ )
    {
	mousepressed_ = false;
	longleftstamp_ = mUdf(int);
    }

    if ( qme->type() == QEvent::MouseButtonPress )
    {
	mousepressed_ = true;
	if ( qme->button() == Qt::LeftButton )
	    longleftstamp_ = Time::getMilliSeconds();
    }

    if ( event->type()==QEvent::MouseMove && mousepressed_ )
    {
	if ( !mIsUdf(longleftstamp_) && Time::passedSince(longleftstamp_)>500 )
	{
	    longleftstamp_ = mUdf(int);
	    QEvent* qev = new QEvent( mUsrEvLongTabletPress );
	    QApplication::postEvent( QApplication::focusWidget(), qev );
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


void myMessageOutput( QtMsgType type, const char *msg );


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
#ifndef __win__
    QCoreApplication::addLibraryPath( GetBinPlfDir() ); // Qt plugin libraries
#endif
}


uiMain::uiMain( int& argc, char **argv )
    : mainobj_( 0 )
{
#ifdef __machack__
        ProcessSerialNumber psn;
        CPSProcessSerNum PSN;

        GetCurrentProcess(&psn);
        if (!CPSGetCurrentProcess(&PSN))
        {
            if (!CPSEnableForegroundOperation(&PSN, 0x03, 0x3C, 0x2C, 0x1103))
            {
                if (!CPSSetFrontProcess(&PSN))
                {
                    GetCurrentProcess(&psn);
                }
            }
        }
#endif

    initQApplication();
    init( 0, argc, argv );
    app_->setWindowIcon( QIcon(XpmIconData) );
}


uiMain::uiMain( QApplication* qapp )
    : mainobj_( 0 )
{ 
    initQApplication();
    app_ = qapp;
    app_->setWindowIcon( QIcon(XpmIconData) );
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

    qInstallMsgHandler( myMessageOutput );

    const char* lookpref = Settings::common().find( "dTect.LookStyle" );
    if ( !lookpref || !*lookpref )
	lookpref = GetEnvVar( "OD_LOOK_STYLE" );
    QStyle* styl = 0;
    if ( lookpref && *lookpref )
    {
	if ( !strcmp(lookpref,"CDE") )
	    styl = new QCDEStyle;
	else if ( !strcmp(lookpref,"Motif") )
	    styl = new QMotifStyle;
	else if ( !strcmp(lookpref,"Windows") )
	    styl = new QWindowsStyle;
	else if ( !strcmp(lookpref,"Plastique") )
	    styl = new QPlastiqueStyle;
	else if ( !strcmp(lookpref,"Cleanlooks") )
	    styl = new QCleanlooksStyle;
    }
#ifdef __win__
    if ( !styl )
	styl = QSysInfo::WindowsVersion == QSysInfo::WV_VISTA
	    	? new QWindowsVistaStyle : new QWindowsXPStyle;
#else
# ifdef __mac__
    if ( !styl )
	styl = new QMacStyle;
# else
    if ( !styl )
	styl = new QCleanlooksStyle;
# endif
#endif

    QApplication::setStyle( styl );
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
	args.add( qargs.at(idx).toAscii() );
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


void uiMain::setFont( const uiFont& font, bool PassToChildren )
{   
    font_ = &font;
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
    { font_ = &FontList().get( className(*this) );  }

    return font_;
}


Color uiMain::windowColor() const
{
    const QColor& qcol = QApplication::palette().color( QPalette::Window );
    return Color( qcol.red(), qcol.green(), qcol.blue() );
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


void myMessageOutput( QtMsgType type, const char *msg )
{
    switch ( type ) {
	case QtDebugMsg:
	    ErrMsg( msg, true );
	    break;
	case QtWarningMsg:
	    ErrMsg( msg, true );
	    break;
	case QtFatalMsg:
	    ErrMsg( msg );
	    break;
    }
}


bool isMainThread( void* thread )
{ return uiMain::theMain().thread() == thread; }

bool isMainThreadCurrent()
{ return isMainThread( Threads::Thread::currentThread() ); }
