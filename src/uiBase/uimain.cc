/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          10/12/1999
 RCS:           $Id: uimain.cc,v 1.23 2004-12-16 10:34:22 bert Exp $
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

#include "qstyle.h"
#include "qcdestyle.h"


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


const uiFont* 	uiMain::font_   = 0;
QApplication* 	uiMain::app     = 0;
uiMain* 	uiMain::themain = 0;

uiMain::uiMain( int argc, char **argv )
    : mainobj( 0 )
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
    : mainobj( 0 )
{ 

    QApplication::setColorSpec( QApplication::ManyColor );
    QApplication::setLibraryPaths( QStringList("/dev") );

    app = qapp;
}


void uiMain::init( QApplication* qap, int argc, char **argv )
{
    if ( app ) 
    {
	pErrMsg("You already have a uiMain object!");
	return;
    }
    else
	themain = this;

    QApplication::setColorSpec( QApplication::ManyColor );
    QApplication::setDesktopSettingsAware( FALSE );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "Constructing QApplication ..." );

    if( qap ) 
	app = qap;
    else
	app = new QApplication( argc, argv );

    if ( DBG::isOn(DBG_UI) && !qap )
	DBG::message( "... done." );

    qInstallMsgHandler( myMessageOutput );

    app->setStyle( new QCDEStyle() );

    font_ = 0;
    setFont( *font() , true );

    bool enab = true;
    if ( !Settings::common().getYN("Ui.ToolTips.enable",enab) )
    {
	Settings::common().setYN( "Ui.ToolTips.enable", true );
	Settings::common().write();
	enab = true;
    }
    if ( !enab )
	uiObject::enableToolTips( false );
}

uiMain::~uiMain()
{
    delete app;
    delete font_;
}


int uiMain::exec()          			
{
    if( !app )  { pErrMsg("Huh?") ; return -1; }
    return app->exec(); 
}


void uiMain::setTopLevel( uiMainWin* obj )
{
    if( !obj ) return;
    if( !app )  { pErrMsg("Huh?") ; return; }

    if( mainobj ) mainobj->setExitAppOnClose( false );
    obj->setExitAppOnClose( true );

    mainobj = obj;
    init( obj->body()->qwidget() ); // inits SoQt if uicMain
    app->setMainWidget( obj->body()->qwidget() ); 
}


void uiMain::setFont( const uiFont& font, bool PassToChildren )
{   
    font_ = &font;
    if( !app )  { pErrMsg("Huh?") ; return; }
    app->setFont( font_->qFont(), PassToChildren ); 
}


void uiMain::exit ( int retcode ) 
{ 
    if( !app )  { pErrMsg("Huh?") ; return; }
    app->exit( retcode ); 
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
    if( !font_ )
    { font_ = &uiFontList::get( className(*this) );  }

    return font_; 
}

uiMain& uiMain::theMain()
{ 
    if( themain ) return *themain; 
    if ( !qApp ) 
    { 
	pFreeFnErrMsg("FATAL: no uiMain and no qApp.","uiMain::theMain()"); 
	QApplication::exit( -1 );
    }

    themain = new uiMain( qApp );
    return *themain;
}


void uiMain::flushX()        
{ 
    if( !app )	return; 
    app->flushX(); 
}

void uiMain::setTopLevelCaption( const char* txt )
{
    //QMainWindow* mw = dynamic_cast<QMainWindow*>(qApp->mainWidget());
    QWidget* mw = qApp->mainWidget();
    if( !mw ) return;
    mw->setCaption( QString(txt) );
}


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{ 
    if( !app )	return; 
    app->processEvents( msec ); 
}


uiSize uiMain::desktopSize()
{
    QWidget *d = QApplication::desktop();
    return uiSize ( d->width(), d->height(), true ); 
}


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
