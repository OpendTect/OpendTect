
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uitutpi.cc,v 1.9 2007-11-22 04:21:43 cvsraman Exp $";

#include "uitutorialattrib.h"
#include "uituthortools.h"
#include "uitutseistools.h"
#include "uitutwelltools.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uivismenuitemhandler.h"
#include "uivispartserv.h"
#include "viswelldisplay.h"

#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"

#include "plugins.h"

static const int sTutIdx = -1100;

extern "C" int GetuiTutPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiTutPluginInfo()
{
    static PluginInfo retpi = {
	"Tutorial plugin development",
	"dGB (Raman/Bert)",
	"3.0",
    	"Shows some simple plugin basics." };
    return &retpi;
}


class uiTutMgr :  public CallBacker
{
public:
			uiTutMgr(uiODMain*);

    uiODMain*		appl_;
    uiVisMenuItemHandler	wellmnuitmhandler_;

    void		doSeis(CallBacker*);
    void		doHor(CallBacker*);
    void		doWells(CallBacker*);
};


uiTutMgr::uiTutMgr( uiODMain* a )
	: appl_(a)
	, wellmnuitmhandler_(visSurvey::WellDisplay::getStaticClassName(),
		  	      *a->applMgr().visServer(),"&Tut Well Tools ...",
			      mCB(this,uiTutMgr,doWells),sTutIdx)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    uiPopupMenu* mnu = new uiPopupMenu( appl_, "&Tut Tools" );
    mnu->insertItem( new uiMenuItem("&Seismic (Direct) ...",
			mCB(this,uiTutMgr,doSeis)) );
    mnu->insertItem( new uiMenuItem("&Horizon ...",
			mCB(this,uiTutMgr,doHor)) );
    mnumgr.utilMnu()->insertItem( mnu );
}


void uiTutMgr::doSeis( CallBacker* )
{
    uiTutSeisTools dlg( appl_ );
    dlg.go();
}


void uiTutMgr::doHor( CallBacker* )
{
    uiTutHorTools dlg( appl_ );
    dlg.go();
}


void uiTutMgr::doWells( CallBacker* )
{
    const int displayid = wellmnuitmhandler_.getDisplayID();
    mDynamicCastGet(visSurvey::WellDisplay*,wd,
	    		appl_->applMgr().visServer()->getObject(displayid))
    if ( !wd )
	return;

    const MultiID wellid = wd->getMultiID();
    PtrMan<IOObj> ioobj = IOM().get( wellid );
    if ( !ioobj )
    {
	uiMSG().error( "Cannot find well in database.\n"
		       "Perhaps it's not stored yet?" );
	return;
    }

    uiTutWellTools dlg( appl_, wellid );
    dlg.go();
}


extern "C" const char* InituiTutPlugin( int, char** )
{
    static uiTutMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiTutMgr( ODMainWin() );

    uiTutorialAttrib::initClass();
    return 0;
}
