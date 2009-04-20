/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          10/12/1999
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimain.cc,v 1.51 2009-04-20 04:47:34 cvsnanne Exp $";

#include "uimain.h"

#include "uifont.h"
#include "uimainwin.h"
#include "uiobjbody.h"

#include "bufstringset.h"
#include "debugmasks.h"
#include "errh.h"
#include "settings.h"
#include "uimsg.h"

#include <QApplication>
#include <QCleanlooksStyle>
#include <QIcon>

#include "dtect.xpm"

#ifdef __win__
# include <QWindowsXPStyle>
# include <QWindowsVistaStyle>
#endif


#ifdef __mac__
# define __machack__
# include <QMacStyle>
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


void myMessageOutput( QtMsgType type, const char *msg );


const uiFont* uiMain::font_ = 0;
QApplication* uiMain::app_ = 0;
uiMain*	uiMain::themain_ = 0;


static void initQApplication()
{
    QApplication::setDesktopSettingsAware( true );
    QApplication::setColorSpec( QApplication::ManyColor );

    QCoreApplication::setOrganizationName( "dGB");
    QCoreApplication::setOrganizationDomain( "opendtect.org" );
    QCoreApplication::setApplicationName( "OpendTect" );
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
    app_->setWindowIcon( QIcon(dtect_xpm_data) );
}


uiMain::uiMain( QApplication* qapp )
    : mainobj_( 0 )
{ 
    initQApplication();
    app_ = qapp;
    app_->setWindowIcon( QIcon(dtect_xpm_data) );
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

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "... done." );

    qInstallMsgHandler( myMessageOutput );

    QApplication::setStyle( new QCleanlooksStyle );
#ifdef __win__
    QApplication::setStyle( QSysInfo::WindowsVersion == QSysInfo::WV_VISTA ?
	    new QWindowsVistaStyle : new QWindowsXPStyle );
#endif
#ifdef __mac__
    QApplication::setStyle( new QMacStyle );
#endif

    font_ = 0;
    setFont( *font() , true );
}


uiMain::~uiMain()
{
    delete app_;
    delete font_;
}


int uiMain::exec()          			
{
    if ( !app_ )  { pErrMsg("Huh?") ; return -1; }
    return app_->exec();
}


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
