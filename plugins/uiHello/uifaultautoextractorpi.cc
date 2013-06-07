/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/

static const char* rcsID = "$Id: uifaultautoextractorpi.cc,v 1.1 2012/02/14 23:20:31 cvsyuancheng Exp $";

#include "uiodmain.h"
#include "uiodapplmgr.h"
#include "uiodmenumgr.h"
#include "uimenu.h"

#include "odplugin.h"
#include "uitracksettingdlg.h"

mDefODPluginInfo(uiFaultAutoExtractor)
{
    static PluginInfo retpi = {
	"FaultAutoExtractor",
	"dGB",
	"=od",
	"The fault auto tracking system" };
    return &retpi;
}


class uiFaultAutoExtractorMgr : public CallBacker
{
public:
	
uiFaultAutoExtractorMgr( uiODMain& a )
    : appl_(a)
{
    appl_.menuMgr().procMnu()->insertItem( 
	    new uiMenuItem("F&ault Auto Tracking...",
		mCB(this,uiFaultAutoExtractorMgr,handleMenu)) );
}

void handleMenu( CallBacker* )
{
    uiTrackSettingDlg dlg( &appl_ );
    dlg.go();
}

uiODMain&	appl_;
};


mDefODInitPlugin(uiFaultAutoExtractor)
{
    static uiFaultAutoExtractorMgr* mgr = 0;
    if ( mgr ) return 0;
    mgr = new uiFaultAutoExtractorMgr( *ODMainWin() );
    
    return 0;
}


