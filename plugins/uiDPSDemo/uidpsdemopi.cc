/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
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

    mDefineStaticLocalObject( PluginInfo, retpi,(
	"DataPointSet demo",
	"OpendTect",
	"Bert",
	"7.8.9",
	commenttxt->buf() ) );
    return &retpi;
}


class uiDPSDemoMgr :  public CallBacker
{ mODTextTranslationClass(uiDPSDemoMgr)
public:

			uiDPSDemoMgr(uiODMain&);
			~uiDPSDemoMgr();

    uiODMain&		appl_;

    void		insertMenuItem(CallBacker* cb=0);
    void		insertIcon(CallBacker* cb=0);
    void		doIt(CallBacker*);

private:

    const uiString	sDPSDemo();
};

static const char* pixmapfilename = "dpsdemo";


uiDPSDemoMgr::uiDPSDemoMgr( uiODMain& a )
	: appl_(a)
{
    uiODMenuMgr& mnumgr = appl_.menuMgr();
    mAttachCB( mnumgr.dTectMnuChanged, uiDPSDemoMgr::insertMenuItem );
    mAttachCB( mnumgr.dTectTBChanged, uiDPSDemoMgr::insertIcon );

    insertMenuItem();
    insertIcon();
}


uiDPSDemoMgr::~uiDPSDemoMgr()
{
    detachAllNotifiers();
}


const uiString uiDPSDemoMgr::sDPSDemo()
{
    return tr("DataPointSet Demo");
}

void uiDPSDemoMgr::insertMenuItem( CallBacker* )
{
    if ( SI().has3D() )
	appl_.menuMgr().analMnu()->insertItem(
	      new uiAction(
	      uiStrings::phrThreeDots(sDPSDemo()),
	      mCB(this,uiDPSDemoMgr,doIt),pixmapfilename) );
}


void uiDPSDemoMgr::insertIcon( CallBacker* )
{
    if ( SI().has3D() )
	appl_.menuMgr().dtectTB()->addButton( pixmapfilename,
			sDPSDemo(), mCB(this,uiDPSDemoMgr,doIt) );
}


void uiDPSDemoMgr::doIt( CallBacker* )
{
    uiDPSDemo dpsdemo( &appl_, appl_.applMgr().visDPSDispMgr() );
    dpsdemo.go();
}


mDefODInitPlugin(uiDPSDemo)
{
    mDefineStaticLocalObject( PtrMan<uiDPSDemoMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiDPSDemoMgr( *ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate DPS Demo plugin";

    return 0;
}
