/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          10/12/1999
 RCS:           $Id: uimain.cc,v 1.7 2001-09-26 14:47:42 arend Exp $
________________________________________________________________________

-*/

#include "uimain.h"
#include "uimainwin.h"
#include "uiobjbody.h"
#include "uifont.h"
#include "errh.h"
#include "settings.h"
#include "qapplication.h"

#include "qstyle.h"
#include "qcdestyle.h"


const uiFont* 	uiMain::font_   = 0;
QApplication* 	uiMain::app     = 0;
uiMain* 	uiMain::themain = 0;

uiMain::uiMain( int argc, char **argv )
: mainobj( 0 )
{
    if ( app ) 
    {
	pErrMsg("You already have a uiMain object!");
	return;
    }
    else
	themain = this;

    app = new QApplication( argc, argv );
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

uiMain::uiMain( QApplication* app_  )
: mainobj( 0 )
{
    app = app_;
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
    if( !app )  { pErrMsg("Huh?") ; return; }
    mainobj = obj;
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
    if( !app )  { pFreeFnErrMsg("Huh?","uiMain::flushX()") ; return; }
    app->flushX(); 
}


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{ 
    if( !app )  { pFreeFnErrMsg("Huh?","uiMain::processEvents") ; return; }
    app->processEvents( msec ); 
}


uiSize uiMain::desktopSize()
{
    QWidget *d = QApplication::desktop();
    return uiSize ( d->width()-1, d->height()-1 ); 
}
