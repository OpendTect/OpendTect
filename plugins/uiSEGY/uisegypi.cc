
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uisegycommon.h"

#include "segydirecttr.h"
#include "survinfo.h"
#include "ioman.h"

#include "uisegydirectinserter.h"
#include "uisegywriteopts.h"
#include "uisegysip.h"
#include "uisegydefdlg.h"
#include "uisegyexp.h"
#include "uisegyread.h"
#include "uisegyresortdlg.h"
#include "uiwellimpsegyvsp.h"
#include "uisegyreadstarter.h"
#include "uisegyimptype.h"
#include "uisegyread.h"

#include "uiseisfileman.h"
#include "uisurvinfoed.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "envvars.h"

#include "odplugin.h"

static const char* segy_iconid = "segy";


mDefODPluginInfo(uiSEGY)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"SEG-Y support",
	"OpendTect",
	"dGB (Bert)",
	"1.0",
	"Supports the SEG-Y format") );
    return &retpi;
}


class uiSEGYMgr	: public CallBacker
{ mODTextTranslationClass(uiSEGYMgr);
public:

			uiSEGYMgr(uiODMain*);

    uiODMain*		appl_;
    uiODMenuMgr&	mnumgr_;

    void		updateMenu(CallBacker*);
    void		updateToolBar(CallBacker*);
    void		survChg(CallBacker*);
    void		edFiles(CallBacker*);
    void		imp2DCB(CallBacker*);
    void		imp2DPSCB(CallBacker*);
    void		imp3DCB(CallBacker*);
    void		imp3DPSCB(CallBacker*);
    void		impVSPCB(CallBacker*);
    void		exp2DCB(CallBacker*);
    void		exp2DPSCB(CallBacker*);
    void		exp3DCB(CallBacker*);
    void		exp3DPSCB(CallBacker*);
    void		reSortCB(CallBacker*);
    void		readStarterCB(CallBacker*);

    void		impClassicCB( CallBacker* )	{ impClassic( false ); }
    void		linkClassicCB( CallBacker* )	{ impClassic( true ); }
    void		impClassic(bool);

    static uiSEGYMgr*	theinst_;
    static const uiString sSEGYString( bool imm )
			 { return imm ? tr("SEG-Y") : m3Dots(tr("SEG-Y")); }

};

uiSEGYMgr* uiSEGYMgr::theinst_ = 0;


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)


uiSEGYMgr::uiSEGYMgr( uiODMain* a )
    : mnumgr_(a->menuMgr())
    , appl_(a)
{
    uiSEGYDirectVolOpts::initClass();
    uiSEGYDirectPS3DOpts::initClass();
    uiSEGYDirectVolInserter::initClass();
    uiSEGYDirect2DInserter::initClass();
    uiSEGYDirectPS3DInserter::initClass();

    uiSeisFileMan::BrowserDef* bdef = new uiSeisFileMan::BrowserDef(
				SEGYDirectSeisTrcTranslator::translKey() );
    bdef->tooltip_ = uiString( "Change file/directory names in SEG-Y file %1" );
    bdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisFileMan::addBrowser( bdef );

    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider();
    uiSurveyInfoEditor::addInfoProvider( sip );
    mnumgr_.dTectTBChanged.notify( muiSEGYMgrCB(updateToolBar) );
    IOM().surveyChanged.notify( muiSEGYMgrCB(updateMenu) );

    updateMenu(0);
    updateToolBar(0);
}


void uiSEGYMgr::updateMenu( CallBacker* )
{
    const bool have2d = SI().has2D(); const bool only2d = !SI().has3D();
    uiMenu* impseismnu = mnumgr_.getMnu( true, uiODApplMgr::Seis );
    uiMenu* impsgymnu = new uiMenu( appl_, sSEGYString(true), segy_iconid );
    impseismnu->insertItem( impsgymnu );
    uiMenu* expseismnu = mnumgr_.getMnu( false, uiODApplMgr::Seis );
    uiMenu* expsgymnu = new uiMenu( appl_, sSEGYString(true), segy_iconid );
    expseismnu->insertItem( expsgymnu );

    if ( have2d )
    {
	const char* lineicid = "seismicline2d";
	const char* linepsicid = "prestackdataset2d";
	uiString linestr = only2d ? m3Dots(tr("Line(s)"))
				  : m3Dots(uiStrings::s2D());
	uiString linepsstr = only2d ? m3Dots(tr("Pre-Stack Data"))
				: m3Dots(tr("Pre-Stack 2D"));

	impsgymnu->insertItem( new uiAction( linestr, muiSEGYMgrCB(imp2DCB),
				lineicid ) );
	impsgymnu->insertItem( new uiAction( linepsstr, muiSEGYMgrCB(imp2DPSCB),
				linepsicid ) );
	expsgymnu->insertItem( new uiAction( linestr, muiSEGYMgrCB(exp2DCB),
				lineicid ) );
	expsgymnu->insertItem( new uiAction( linepsstr, muiSEGYMgrCB(exp2DPSCB),
				linepsicid ) );
    }

    if ( !only2d )
    {
	const char* volicid = "seismiccube";
	uiString volstr = have2d ? m3Dots(uiStrings::s3D())
				 : m3Dots(uiStrings::sVolume());
	const char* volpsicid = "prestackdataset";
	uiString volpsstr = have2d ? m3Dots(tr("PreStack 3D"))
				: m3Dots(tr("Pre-Stack Volume"));

	impsgymnu->insertItem( new uiAction(volstr,muiSEGYMgrCB(imp3DCB),
					volicid) );
        impsgymnu->insertItem( new uiAction(volpsstr,muiSEGYMgrCB(imp3DPSCB),
					volpsicid) );

        expsgymnu->insertItem( new uiAction(volstr,muiSEGYMgrCB(exp3DCB),
					volicid) );
        expsgymnu->insertItem( new uiAction(volpsstr,muiSEGYMgrCB(exp3DPSCB),
					volpsicid) );
    }

    mnumgr_.getMnu( true, uiODApplMgr::Wll )->insertItem(
	new uiAction( m3Dots(tr("VSP (SEG-Y)")), muiSEGYMgrCB(impVSPCB),
			"vsp0" ) );
    mnumgr_.createSeisOutputMenu()->insertItem(
	new uiAction(m3Dots(tr("Re-sort Scanned SEG-Y")),
			muiSEGYMgrCB(reSortCB)) );

    bool segyclassictoplevel = GetEnvVarYN( "OD_SEGY_CLASSIC_TOPLEVEL" );
    uiString classicmnutitle = segyclassictoplevel ? tr("SEG-Y [Classic]")
						   : tr("Classic tool");
    uiMenu* impclassmnu = new uiMenu( appl_, classicmnutitle, "launch" );
    (segyclassictoplevel ? impseismnu : impsgymnu)->insertItem( impclassmnu );
    impclassmnu->insertItem( new uiAction( uiStrings::sImport(),
		   muiSEGYMgrCB(impClassicCB), "import") );
    impclassmnu->insertItem( new uiAction( tr("Link"),
		   muiSEGYMgrCB(linkClassicCB), "link") );
}


void uiSEGYMgr::updateToolBar( CallBacker* )
{
    mnumgr_.dtectTB()->addButton( segy_iconid, tr("SEG-Y import"),
				  mCB(this,uiSEGYMgr,readStarterCB) );
}

#define mImplImpCB(typ,arg) \
void uiSEGYMgr::imp##typ##CB( CallBacker* ) \
{ \
    const SEGY::ImpType imptyp( arg ); \
    uiSEGYReadStarter dlg( appl_, false, &imptyp ); \
    dlg.go(); \
}

mImplImpCB( 2D, Seis::Line )
mImplImpCB( 3D, Seis::Vol )
mImplImpCB( 2DPS, Seis::LinePS )
mImplImpCB( 3DPS, Seis::VolPS )
mImplImpCB( VSP, true )


#define mImplExpCB(typ,arg) \
void uiSEGYMgr::exp##typ##CB( CallBacker* ) \
{ \
    uiSEGYExp dlg( appl_, arg ); \
    dlg.go(); \
}

mImplExpCB( 2D, Seis::Line )
mImplExpCB( 2DPS, Seis::LinePS )
mImplExpCB( 3D, Seis::Vol )
mImplExpCB( 3DPS, Seis::VolPS )


void uiSEGYMgr::impClassic( bool islink )
{
    uiSEGYRead::Setup su( islink ? uiSEGYRead::DirectDef : uiSEGYRead::Import );
    if ( islink )
	su.geoms_ -= Seis::Line;
    new uiSEGYRead( appl_, su );
}


void uiSEGYMgr::reSortCB( CallBacker* )
{
    uiResortSEGYDlg dlg( appl_ );
    dlg.go();
}


void uiSEGYMgr::edFiles( CallBacker* cb )
{
    mDynamicCastGet(uiSeisFileMan*,sfm,cb)
    if ( !sfm || !sfm->curIOObj() )
	return;

    uiEditSEGYFileDataDlg dlg( sfm, *sfm->curIOObj() );
    dlg.go();
}


void uiSEGYMgr::readStarterCB( CallBacker* )
{
    uiSEGYReadStarter dlg( ODMainWin(), false );
    dlg.go();
}


mDefODInitPlugin(uiSEGY)
{
    if ( !uiSEGYMgr::theinst_ )
	uiSEGYMgr::theinst_ = new uiSEGYMgr( ODMainWin() );
    return 0;
}
