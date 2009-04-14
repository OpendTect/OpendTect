
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uitutpi.cc,v 1.14 2009-04-14 10:04:09 cvsnanne Exp $";

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
#include "seistype.h"
#include "survinfo.h"

#include "plugins.h"

static const int sTutIdx = -1100;

mExternC mGlobal int GetuiTutPluginType()
{
    return PI_AUTO_INIT_LATE;
}


mExternC mGlobal PluginInfo* GetuiTutPluginInfo()
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

    bool		is2d_;
    uiODMain*		appl_;
    uiPopupMenu*	mnuhor_;
    uiPopupMenu*	mnuseis_;
    uiVisMenuItemHandler	wellmnuitmhandler_;

    void 		insertSubMenu();

    void		doSeis(CallBacker*);
    void		do2DSeis(CallBacker*);
    void		do3DSeis(CallBacker*);
    void		doHor(CallBacker*);
    void		doWells(CallBacker*);
};


uiTutMgr::uiTutMgr( uiODMain* a )
	: appl_(a)
	, is2d_(false)
	, wellmnuitmhandler_(visSurvey::WellDisplay::getStaticClassName(),
		  	      *a->applMgr().visServer(),"&Tut Well Tools ...",
			      mCB(this,uiTutMgr,doWells),sTutIdx)
{
    uiODMenuMgr& mnumgr = appl_->menuMgr();
    uiPopupMenu* mnu = new uiPopupMenu( appl_, "&Tut Tools" );
    if ( SI().has2D() && SI().has3D() ) 
    {	mnuseis_ = new uiPopupMenu( appl_, "3D" );
    	mnu->insertMenu( mnuseis_, mnu );
    	mnuhor_ = new uiPopupMenu( appl_, "2D" );
    	mnu->insertMenu( mnuhor_, mnu );
	insertSubMenu();
    }	
    else
    {
	mnu->insertItem( new uiMenuItem("&Seismic (Direct) ...",
					mCB(this,uiTutMgr,doSeis)) );
	mnu->insertItem( new uiMenuItem("&Horizon ...",
					mCB(this,uiTutMgr,doHor)) );
    }
    mnumgr.toolsMnu()->insertItem( mnu );
}

void uiTutMgr::insertSubMenu()
{
    mnuseis_->insertItem( new uiMenuItem("&Seismic (Direct) ...",
					 mCB(this,uiTutMgr,do3DSeis)) );
    mnuseis_->insertItem( new uiMenuItem("&Horizon ...",
					 mCB(this,uiTutMgr,doHor)) );

    mnuhor_->insertItem( new uiMenuItem("&Seismic (Direct) ...",
					mCB(this,uiTutMgr,do2DSeis)) );
    mnuhor_->insertItem( new uiMenuItem("&Horizon ...",
					mCB(this,uiTutMgr,doHor)) );
}


void uiTutMgr::do3DSeis( CallBacker* cb )
{
    is2d_ = false;
    doSeis( cb );
}    


void uiTutMgr::do2DSeis( CallBacker* cb )
{
    is2d_ = true;
    doSeis( cb );
}    


void uiTutMgr::doSeis( CallBacker* )
{
    if ( is2d_ )
    {
	uiTutSeisTools dlg( appl_, Seis::Line );
	dlg.go();
    }
    else
    {
	uiTutSeisTools dlg( appl_, Seis::Vol );
	dlg.go();
    }
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


mExternC mGlobal const char* InituiTutPlugin( int, char** )
{
    static uiTutMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiTutMgr( ODMainWin() );

    uiTutorialAttrib::initClass();
    return 0;
}
