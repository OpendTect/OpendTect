
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/


#include "uimadagascarmain.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiodmenumgr.h"
#include "uitoolbar.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "maddefs.h"
#include "madio.h"
#include "odplugin.h"
#include "separstr.h"
#include "staticstring.h"



static bool checkEnvVars( uiString& msg )
{
    BufferString rsfdir = GetEnvVar( "RSFROOT" );
    if ( rsfdir.isEmpty() || !File::isDirectory(rsfdir.buf()) )
    {
	msg = od_static_tr("Madagascarlink_checkEnvVars",
					"RSFROOT is either not set or invalid");
	return false;
    }

    return true;
}


class uiMadagascarLinkMgr	: public CallBacker
{ mODTextTranslationClass(uiMadagascarLinkMgr);
public:

			uiMadagascarLinkMgr(uiODMain&);
			~uiMadagascarLinkMgr();

    uiODMain&		appl_;
    uiODMenuMgr&	mnumgr;
    uiMadagascarMain*	madwin_;
    bool		ishidden_;

    void		doMain(CallBacker*);
    void		updateToolBar(CallBacker*);
    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);
    void		winHide(CallBacker*);

    static uiString	pkgDispNm()    { return tr("Madagascar Link"); }

};


uiMadagascarLinkMgr::uiMadagascarLinkMgr( uiODMain& a )
    : mnumgr(a.menuMgr())
    , madwin_(0)
    , ishidden_(false)
    , appl_(a)
{
    mAttachCB( mnumgr.dTectTBChanged, uiMadagascarLinkMgr::updateToolBar );
    mAttachCB( mnumgr.dTectMnuChanged, uiMadagascarLinkMgr::updateMenu );
    mAttachCB( DBM().surveyToBeChanged, uiMadagascarLinkMgr::survChg );
    updateToolBar(0);
    updateMenu(0);
}


uiMadagascarLinkMgr::~uiMadagascarLinkMgr()
{
    detachAllNotifiers();
}


void uiMadagascarLinkMgr::updateToolBar( CallBacker* )
{
}


void uiMadagascarLinkMgr::updateMenu( CallBacker* )
{
    delete madwin_; madwin_ = 0; ishidden_ = false;
    uiAction* newitem = new uiAction( m3Dots(toUiString("Madagascar")),
				      mCB(this,uiMadagascarLinkMgr,doMain),
				      "madagascar" );
    mnumgr.procMnu()->insertAction( newitem );
}


void uiMadagascarLinkMgr::survChg( CallBacker* )
{
    if ( !madwin_ ) return;

    madwin_->askSave(false);
}


void uiMadagascarLinkMgr::winHide( CallBacker* )
{
    ishidden_ = true;
}


void uiMadagascarLinkMgr::doMain( CallBacker* )
{
    uiString errmsg;
    if ( !checkEnvVars(errmsg) )
    {
	gUiMsg().error( errmsg );
	return;
    }

    if ( !madwin_ )
    {
	madwin_ = new uiMadagascarMain( &appl_ );
	madwin_->windowHide.notify( mCB(this,uiMadagascarLinkMgr,winHide) );
    }

    ishidden_ = false;
    madwin_->show();
    madwin_->raise();
}


mDefODPluginInfo(uiMadagascar)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"Madagascar Link",
	mMadagascarLinkPackage,
	mODPluginCreator, mODPluginVersion,
	"Link to the Madagascar batch-level seismic processing tools." ));
    retpi.useronoffselectable_ = true;
    retpi.url_ = "reproducibility.org";
    mSetPackageDisplayName( retpi, uiMadagascarLinkMgr::pkgDispNm() );
    return &retpi;
}


mDefODInitPlugin(uiMadagascar)
{
    mDefineStaticLocalObject( PtrMan<uiMadagascarLinkMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiMadagascarLinkMgr( *ODMainWin() );
    if ( !theinst_ )
	return toString( ODMad::PI().errMsg() );

    DBMan::CustomDirData cdd( ODMad::cMadDirIDNr(), ODMad::sKeyMadagascar(),
			      "Madagascar data" );
    uiRetVal uirv = DBMan::addCustomDataDir( cdd );
    if ( !uirv.isOK() )
    {
	mDeclStaticString(ret);
	ret = uirv.getText();
	return ret.str();
    }

#ifdef MAD_UIMSG_IF_FAIL
    if ( ODMad::PI().errMsg().isSet() )
	gUiMsg().error( ODMad::PI().errMsg() );
#endif

    return 0;
}
