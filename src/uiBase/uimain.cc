/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          10/12/1999
 RCS:           $Id: uimain.cc,v 1.42 2008-01-03 07:54:35 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimain.h"
#include "uimainwin.h"
#include "uiobjbody.h"
#include "uifont.h"
#include "errh.h"
#include "debugmasks.h"
#include "settings.h"
#include "qapplication.h"
#include "uimsg.h"

#include "qstyle.h"
#include "qcdestyle.h"

#include "qiconset.h"

#include <QCleanlooksStyle>

#ifdef __win__
# include <QWindowsXPStyle>
#endif


#ifdef __mac__
# define __machack__
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

    init(0,argc,argv);
}


uiMain::uiMain( QApplication* qapp )
    : mainobj_( 0 )
{ 
    QApplication::setDesktopSettingsAware( false );
    QApplication::setEffectEnabled( Qt::UI_AnimateMenu, false );
    QApplication::setEffectEnabled( Qt::UI_AnimateTooltip, false );
    QApplication::setColorSpec( QApplication::ManyColor );
    QApplication::setLibraryPaths( QStringList("/dev") );

    QCoreApplication::setOrganizationName( "dGB");
    QCoreApplication::setOrganizationDomain( "opendtect.org" );
    QCoreApplication::setApplicationName( "OpendTect" );

    app_ = qapp;
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

# ifdef __win__
    QApplication::setStyle( new QWindowsXPStyle );
# else
    QApplication::setStyle( new QCleanlooksStyle );
# endif

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


void uiMain::setTopLevel( uiMainWin* obj )
{
    if ( !obj ) return;
    if ( !app_ )  { pErrMsg("Huh?") ; return; }

    if ( mainobj_ ) mainobj_->setExitAppOnClose( false );
    obj->setExitAppOnClose( true );

    mainobj_ = obj;
    init( mainobj_->body()->qwidget() ); // inits SoQt if uicMain
    app_->setMainWidget( mainobj_->body()->qwidget() );
}


void uiMain::setFont( const uiFont& font, bool PassToChildren )
{   
    font_ = &font;
    if ( !app_ )  { pErrMsg("Huh?") ; return; }
    app_->setFont( font_->qFont(), PassToChildren );
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
    { font_ = &uiFontList::get( className(*this) );  }

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
{ if ( app_ ) app_->flushX(); }


void uiMain::setTopLevelCaption( const char* txt )
{
    QWidget* mw = qApp->mainWidget();
    if ( mw ) mw->setCaption( QString(txt) );
}


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
