/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uidpsdemopi.cc,v 1.16 2011/04/21 13:09:13 cvsbert Exp $";


#include "uidpsdemo.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uitoolbar.h"

#include "odver.h"
#include "datapointset.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "pixmap.h"
#include "randcolor.h"
#include "survinfo.h"

#include "odplugin.h"


mDefODPluginInfo(uiDPSDemo)
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

    void		insertMenuItem(CallBacker* cb=0);
    void		insertIcon(CallBacker* cb=0);
    void		doIt(CallBacker*);
};

static const char* pixmapfilename = "dpsdemo.png";


uiDPSDemoMgr::uiDPSDemoMgr( uiODMain& a )
	: appl_(a)
{
    uiODMenuMgr& mnumgr = appl_.menuMgr();
    mnumgr.dTectMnuChanged.notify( mCB(this,uiDPSDemoMgr,insertMenuItem) );
    mnumgr.dTectTBChanged.notify( mCB(this,uiDPSDemoMgr,insertIcon) );

    insertMenuItem();
    insertIcon();
}


void uiDPSDemoMgr::insertMenuItem( CallBacker* )
{
    if ( SI().has3D() )
	appl_.menuMgr().analMnu()->insertItem(
		new uiMenuItem("&DataPointSet demo ...",
		mCB(this,uiDPSDemoMgr,doIt),pixmapfilename) );
}


void uiDPSDemoMgr::insertIcon( CallBacker* )
{
    if ( SI().has3D() )
	appl_.menuMgr().dtectTB()->addButton( pixmapfilename,
			"DataPointSet demo", mCB(this,uiDPSDemoMgr,doIt) );
}


void uiDPSDemoMgr::doIt( CallBacker* )
{
    uiDPSDemo dpsdemo( &appl_, appl_.applMgr().visDPSDispMgr() );
    dpsdemo.go();
}


mDefODInitPlugin(uiDPSDemo)
{
    static uiDPSDemoMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiDPSDemoMgr( *ODMainWin() );
    return 0;
}
