/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          10/12/1999
 RCS:           $Id: uimain.cc,v 1.1 2000-11-27 10:20:35 bert Exp $
________________________________________________________________________

-*/

#include "uimain.h"
#include "uimainwin.h"
#include "uiobj.h"
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

#if 0

    QPalette p( QColor( 75, 123, 130 ) );
    p.setColor( QPalette::Active, QColorGroup::Base, QColor( 55, 77, 78 ) );
    p.setColor( QPalette::Inactive, QColorGroup::Base, QColor( 55, 77, 78 ) );
    p.setColor( QPalette::Disabled, QColorGroup::Base, QColor( 55, 77, 78 ) );
    p.setColor( QPalette::Active, QColorGroup::Highlight, Qt::white );
    p.setColor( QPalette::Active, QColorGroup::HighlightedText, QColor( 55, 77, 78 ) );
    p.setColor( QPalette::Inactive, QColorGroup::Highlight, Qt::white );
    p.setColor( QPalette::Inactive, QColorGroup::HighlightedText, QColor( 55, 77, 78 ) );
    p.setColor( QPalette::Disabled, QColorGroup::Highlight, Qt::white );
    p.setColor( QPalette::Disabled, QColorGroup::HighlightedText, QColor( 55, 77, 78 ) );
    p.setColor( QPalette::Active, QColorGroup::Foreground, Qt::white );
    p.setColor( QPalette::Active, QColorGroup::Text, Qt::white );
    p.setColor( QPalette::Active, QColorGroup::ButtonText, Qt::white );
    p.setColor( QPalette::Inactive, QColorGroup::Foreground, Qt::white );
    p.setColor( QPalette::Inactive, QColorGroup::Text, Qt::white );
    p.setColor( QPalette::Inactive, QColorGroup::ButtonText, Qt::white );
    p.setColor( QPalette::Disabled, QColorGroup::Foreground, Qt::lightGray );
    p.setColor( QPalette::Disabled, QColorGroup::Text, Qt::lightGray );
    p.setColor( QPalette::Disabled, QColorGroup::ButtonText, Qt::lightGray );
    app->setPalette( p, TRUE );
#endif

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
    return app->exec(); 
}


void uiMain::setTopLevel( uiMainWin* obj )
{
    mainobj = obj;
    app->setMainWidget( &obj->qWidget() ); 
}


void uiMain::setFont( const uiFont& font, bool PassToChildren )
{   
    font_ = &font;
    app->setFont( font_->qFont(), PassToChildren ); 
}


void uiMain::exit ( int retcode ) 
{ 
    return app->exit( retcode ); 
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


void uiMain::flushX()        
{ 
    app->flushX(); 
}


//! waits [msec] milliseconds for new events to occur and processes them.
void uiMain::processEvents( int msec )
{ 
    app->processEvents( msec ); 
}


uiSize uiMain::desktopSize()
{
    QWidget *d = QApplication::desktop();
    return uiSize ( d->width(), d->height() ); 
}
