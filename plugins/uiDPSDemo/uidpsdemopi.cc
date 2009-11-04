/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uidpsdemopi.cc,v 1.1 2009-11-04 09:20:25 cvsbert Exp $";


#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "odver.h"
#include "plugins.h"


#include "uimsg.h"

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

    uiODMain&		appl;
    void		doIt(CallBacker*);
};


uiDPSDemoMgr::uiDPSDemoMgr( uiODMain& a )
	: appl(a)
{
    uiMenuItem* newitem = new uiMenuItem( "&DataPointSet demo ...",
	    				  mCB(this,uiDPSDemoMgr,doIt) );
    appl.menuMgr().analMnu()->insertItem( newitem );
}


void uiDPSDemoMgr::doIt( CallBacker* )
{
    uiMSG().message( "doIt callback" );
}


mExternC const char* InituiDPSDemoPlugin( int, char** )
{
    static uiDPSDemoMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiDPSDemoMgr( *ODMainWin() );
    return 0;
}
