/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidpsdemo.h"

#include "uimenu.h"
#include "uimsg.h"
#include "uiodmain.h"
#include "uiodmenumgr.h"
#include "uistrings.h"
#include "uitoolbar.h"
#include "uivisdatapointsetdisplaymgr.h"

#include "datapointset.h"
#include "odplugin.h"
#include "odver.h"
#include "randcolor.h"
#include "survinfo.h"


mDefODPluginInfo(uiDPSDemo)
{
    // Just to show a way to make plugin info text variable
    mDefineStaticLocalObject( PtrMan<BufferString>, commenttxt, (0) );
    if ( !commenttxt )
    {
	commenttxt = new BufferString( "Showing a few DataPointSet things."
		"\n\nAs present in version " );
	*commenttxt += mODMajorVersion; *commenttxt += ".";
	*commenttxt += mODMinorVersion;
	*commenttxt += " ("; *commenttxt += GetFullODVersion();
	*commenttxt += ").";
    }

    static PluginInfo retpi(
	"DataPointSet demo (GUI)",
	commenttxt->buf() );
    return &retpi;
}


class uiDPSDemoMgr :  public uiPluginInitMgr
{ mODTextTranslationClass(uiDPSDemoMgr)
public:

			uiDPSDemoMgr();

private:

    uiDialog*		dlg_ = nullptr;

    void		dTectMenuChanged() override;
    void		dTectToolbarChanged() override;
    void		cleanup() override;

    void		showDlgCB(CallBacker*);

    const uiString	sDPSDemo();
};

static const char* pixmapfilename = "dpsdemo";


uiDPSDemoMgr::uiDPSDemoMgr()
    : uiPluginInitMgr()
{
    init();
}


const uiString uiDPSDemoMgr::sDPSDemo()
{
    return tr("DataPointSet Demo");
}


void uiDPSDemoMgr::dTectMenuChanged()
{
    if ( !SI().has3D() )
	return;

    appl().menuMgr().analMnu()->insertAction(
			    new uiAction( m3Dots(sDPSDemo()),
				mCB(this,uiDPSDemoMgr,showDlgCB),
				pixmapfilename ) );
}


void uiDPSDemoMgr::dTectToolbarChanged()
{
    if ( !SI().has3D() )
	return;

    appl().menuMgr().dtectTB()->addButton( pixmapfilename, sDPSDemo(),
					   mCB(this,uiDPSDemoMgr,showDlgCB) );
}


void uiDPSDemoMgr::cleanup()
{
    closeAndZeroPtr( dlg_ );
    uiPluginInitMgr::cleanup();
}


void uiDPSDemoMgr::showDlgCB( CallBacker* )
{
    if ( !dlg_ )
    {
	dlg_ = new uiDPSDemo( &appl(), appl_.applMgr().visDPSDispMgr() );
	dlg_->setModal( false );
    }

    dlg_->show();
}


mDefODInitPlugin(uiDPSDemo)
{
    mDefineStaticLocalObject( PtrMan<uiDPSDemoMgr>, theinst_,
			    = new uiDPSDemoMgr() );
    if ( !theinst_ )
	return "Cannot instantiate the DPS Demo plugin";

    return nullptr;
}
