/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uidpsdemopi.cc,v 1.12 2009-12-08 08:33:23 cvssatyaki Exp $";


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
#include "plugins.h"
#include "randcolor.h"
#include "survinfo.h"


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

    DataPointSetDisplayMgr* dpsdispmgr_;
    const ioPixmap	pixmap_;

    void		insertMenuItem(CallBacker* cb=0);
    void		insertIcon(CallBacker* cb=0);
    void		doIt(CallBacker*);
    void		showSelPtsCB(CallBacker*);
    void		removeSelPtsCB(CallBacker*);

};


uiDPSDemoMgr::uiDPSDemoMgr( uiODMain& a )
	: appl_(a)
	, dpsdispmgr_(a.applMgr().visDPSDispMgr())
	, pixmap_("dpsdemo.png")
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
		mCB(this,uiDPSDemoMgr,doIt),&pixmap_) );
}


void uiDPSDemoMgr::insertIcon( CallBacker* )
{
    if ( SI().has3D() )
	appl_.menuMgr().dtectTB()->addButton( pixmap_,
		mCB(this,uiDPSDemoMgr,doIt), "DataPointSet demo" );
}


void uiDPSDemoMgr::doIt( CallBacker* )
{
    uiDPSDemo* dpsdemo = new uiDPSDemo( &appl_, dpsdispmgr_ );
    dpsdemo->setDeleteOnClose( true );
    dpsdemo->go();
}


mExternC const char* InituiDPSDemoPlugin( int, char** )
{
    static uiDPSDemoMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiDPSDemoMgr( *ODMainWin() );
    return 0;
}
