/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/

static const char* rcsID = "$Id: uidpsdemopi.cc,v 1.7 2009-11-19 13:07:59 cvsbert Exp $";


#include "uidpsdemo.h"

#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uitoolbar.h"

#include "odver.h"
#include "datapointset.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "pixmap.h"
#include "plugins.h"
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
    uiDPSDemo*		dpsdemo_;

    int 		dpsid_;
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
	, dpsdemo_(0)
	, dpsid_(-1)
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
    dpsdemo_ = new uiDPSDemo( &appl_ );
    dpsdemo_->selPtsToBeShown.notify( mCB(this,uiDPSDemoMgr,showSelPtsCB) );
    dpsdemo_->selPtsToBeRemoved.notify( mCB(this,uiDPSDemoMgr,removeSelPtsCB) );
    dpsdemo_->setDeleteOnClose( true );
    dpsdemo_->go();
}


void uiDPSDemoMgr::showSelPtsCB( CallBacker* )
{
    const DataPointSet& dps = dpsdemo_->getDPS();
    if ( !dpsdispmgr_ ) return;

    dpsdispmgr_->lock();
    if ( dpsid_ < 0 )
	dpsid_ = dpsdispmgr_->addDisplay( dpsdispmgr_->availableParents(), dps);
    else
	dpsdispmgr_->updateDisplay( dpsid_,dpsdispmgr_->availableParents(),dps);

    dpsdispmgr_->unLock();
}


void uiDPSDemoMgr::removeSelPtsCB( CallBacker* )
{
    if ( dpsdispmgr_ )
	dpsdispmgr_->removeDisplay( dpsid_ );
    dpsid_ = -1;
}


mExternC const char* InituiDPSDemoPlugin( int, char** )
{
    static uiDPSDemoMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiDPSDemoMgr( *ODMainWin() );
    return 0;
}
