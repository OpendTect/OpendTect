/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          May 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uimfcdialog.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "plugins.h"
#include "uimainwin.h"



class MnuMgr : public CallBacker
{
public:
		MnuMgr(uiODMain&);

protected:
    void	Mnufnctn(CallBacker*);
    uiODMain&	appl_;
};


MnuMgr::MnuMgr( uiODMain& a )
    : appl_(a)
{
    uiAction* newitem =
	new uiAction( "Launch MFC Dialog", mCB(this,MnuMgr,Mnufnctn) );
    appl_.menuMgr().utilMnu()->insertItem( newitem );
}


void MnuMgr::Mnufnctn( CallBacker* )
{ 
    initMFCDialog( NULL );
}


mDefODPluginInfo(uiMFC)
{
    mDefineStaticLocalObject( (PluginInfo, retpi, (
	"MFC Classes (GUI)",
	"OpendTect",
	"dGB Earth Sciences (Ranojay)",
	"=od",
        "Shows simple MFC Dialog Class built in Developer Studio" ))
    return &retpi;
}


mDefODInitPlugin(uiMFC)
{
    (void)new MnuMgr( *ODMainWin() );
    return nullptr;
}
