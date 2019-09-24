/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          August 2001
________________________________________________________________________

-*/

#include "prog.h"


#include "uimain.h"
#include "uimainwin.h"
#include "uiwebengine.h"

#include "commandlineparser.h"
#include "moddepmgr.h"


class uiWebEngineViewer : public uiMainWin
{ mODTextTranslationClass(uiWebEngineViewer)
public:

		uiWebEngineViewer(uiParent*,const BufferString& url,
				  bool maximized=false);
		~uiWebEngineViewer();

private:

    void		initWin(CallBacker*);

    uiWebEngine*	webview_;
    bool		maximized_ = false;
    BufferString	url_;
};


#define nrstatusfld 0

uiWebEngineViewer::uiWebEngineViewer( uiParent* p, const BufferString& url,
				      bool maximized )
    : uiMainWin(p,tr("WebEngine Viewer"),nrstatusfld)
    , url_(url)
    , maximized_(maximized)
{
    webview_ = new uiWebEngine( this );
    webview_->setHSzPol( uiObject::WideMax );
    webview_->setVSzPol( uiObject::WideMax );
    webview_->setMinimumWidth( 384 );
    webview_->setMinimumHeight( 384 );

    mAttachCB( postFinalise(), uiWebEngineViewer::initWin );
}


uiWebEngineViewer::~uiWebEngineViewer()
{
    detachAllNotifiers();
}


void uiWebEngineViewer::initWin( CallBacker* )
{
    if ( !url_.isEmpty() )
	webview_->setUrl( url_.str() );

    if ( maximized_ )
    {
	showMaximized();
	webview_->reload();
    }
}


static void printBatchUsage( const char* prognm )
{
    od_ostream& strm = od_ostream::logStream();
    strm << "Usage: " << prognm;
    strm << "Opens a dialog showing a url using the QtWebEngine.\n";
    strm << "Mandatory arguments:\n";
    strm << "\t --url" << "\t\tURL to browse\n";
    strm << "Alternative positional arguments:\n";
    strm << "\t --hostnm" << "\tHost Name\n";
    strm << "\t --portnr" << "\tPort number\n";
    strm << "Optional arguments:\n";
    strm << "\t --maximized" << "\tDisplay maximized\n";
    strm << od_endl;
}

#undef mErrRet
#define mErrRet(act) \
{ \
    act; \
    return ExitProgram( 1 ); \
}

int main( int argc, char** argv )
{
    OD::SetRunContext( OD::UiProgCtxt );
    SetProgramArgs( argc, argv );
    uiMain app;
    OD::ModDeps().ensureLoaded( "uiBase" );

    auto& clp = app.commandLineParser();
    if ( argc < 2 )
	mErrRet(printBatchUsage( clp.getExecutableName() ))

    BufferString url;
    if ( clp.hasKey("url") )
    {
	url = clp.keyedString( "url" );
    }
    else if ( clp.hasKey("host") && clp.hasKey("port") )
    {
	const BufferString hostnm( clp.keyedString( "host" ) );
	const int portnr = clp.keyedValue<int>( "port" );
	url.set( "http://" ).add( hostnm ).add( ":" ).add( portnr );
    }

    const bool maximized = clp.hasKey( "maximized" );
    uiWebEngineViewer* pv = new uiWebEngineViewer( nullptr, url, maximized );
    if ( !pv )
	mErrRet(printBatchUsage( clp.getExecutableName() ))

    app.setTopLevel( pv );
    pv->show();

    const int ret = app.exec();
    delete pv;
    return ExitProgram( ret );
}
