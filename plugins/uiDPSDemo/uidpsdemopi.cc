/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uidpsdemopi.cc,v 1.2 2009-11-04 09:49:24 cvsbert Exp $";


#include "uidpsdemo.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uitoolbar.h"

#include "odver.h"
#include "pixmap.h"
#include "plugins.h"


mExternC int GetuiDPSDemoPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC PluginInfo* GetuiDPSDemoPluginInfo()
{
    // Just to show a way to make plugin info text variable
    static BufferString* commenttxt = 0;
    if ( !commenttxt )
    {
	commenttxt = new BufferString( "Showing a few DataPointSet things."
		"\n\nAs present in version " );
	*commenttxt += mODMajorVersion; *commenttxt += ".";
	*commenttxt += mODMinorVersion;
	*commenttxt += " ("; *commenttxt += GetFullODVersion();
	*commenttxt += ").";
    }

    static PluginInfo retpi = {
	"DataPointSet demo",
	"Bert",
	"7.8.9",
	commenttxt->buf() };
    return &retpi;
}


class uiDPSDemoMgr :  public CallBacker
{
public:

			uiDPSDemoMgr(uiODMain&);

    uiODMain&		appl_;
    void		doIt(CallBacker*);
};


uiDPSDemoMgr::uiDPSDemoMgr( uiODMain& a )
	: appl_(a)
{
    uiODMenuMgr& mnumgr = appl_.menuMgr();
    const ioPixmap pm( "dpsdemo.png" );
    const CallBack cb( mCB(this,uiDPSDemoMgr,doIt) );

    mnumgr.analMnu()->insertItem( new uiMenuItem("&DataPointSet demo ...",
						   cb,&pm) );
    mnumgr.dtectTB()->addButton( pm, cb, "DataPointSet demo" );
}


void uiDPSDemoMgr::doIt( CallBacker* )
{
    uiDPSDemo dlg( &appl_ );
    dlg.go();
}


mExternC const char* InituiDPSDemoPlugin( int, char** )
{
    static uiDPSDemoMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiDPSDemoMgr( *ODMainWin() );
    return 0;
}
