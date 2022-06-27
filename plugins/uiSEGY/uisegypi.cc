
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/


#include "uisegymod.h"

#include "segydirecttr.h"
#include "survinfo.h"
#include "ioman.h"

#include "uisegysip.h"
#include "uisegysipclassic.h"
#include "uisegydefdlg.h"
#include "uisegyexp.h"
#include "uisegyread.h"
#include "uisegyresortdlg.h"
#include "uisegyreadstarter.h"

#include "uiseisfileman.h"
#include "uiseispsman.h"
#include "uisurvinfoed.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "envvars.h"

#include "odplugin.h"

static const char* segyiconid_ = "segy";

static bool enableClassic()
{
    const bool enabclassic = GetEnvVarYN( "OD_ENABLE_SEGY_CLASSIC" );
    return enabclassic;
}


mDefODPluginInfo(uiSEGY)
{
    mDefineStaticLocalObject( PluginInfo, retpi, (
	"SEG-Y support (GUI)",
	"OpendTect",
	"dGB Earth Sciences",
	"=od",
	"Support for the SEG-Y I/O in the OpendTect main application" ))
    return &retpi;
}


class uiSEGYMgr	: public uiPluginInitMgr
{ mODTextTranslationClass(uiSEGYMgr);
public:
			uiSEGYMgr();
			~uiSEGYMgr();

private:

    uiSEGYReadStarter*	impdlg_ = nullptr;
    uiSEGYExp*		expdlg_ = nullptr;

    void		beforeSurveyChange() override { cleanup(); }
    void		dTectMenuChanged() override;
    void		dTectToolbarChanged() override;
    void		cleanup();

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
    void		applicationClosingCB(CallBacker*);

    void		impClassicCB( CallBacker* )	{ impClassic( false ); }
    void		linkClassicCB( CallBacker* )	{ impClassic( true ); }
    void		impClassic(bool);

    static const uiString sSEGYString( bool imm )
			 { return imm ? tr("SEG-Y") : m3Dots(tr("SEG-Y")); }

};


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)


uiSEGYMgr::uiSEGYMgr()
    : uiPluginInitMgr()
{
    init();
    auto* bdef = new uiSeisFileMan::BrowserDef(
				SEGYDirectSeisTrcTranslator::translKey() );
    bdef->tooltip_ = tr("Change file/folder names in SEG-Y file %1");
    bdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisFileMan::addBrowser( bdef );

    uiSurveyInfoEditor::addInfoProvider( new uiSEGYSurvInfoProvider() );
    if ( enableClassic() )
	uiSurveyInfoEditor::addInfoProvider(
			    new uiSEGYClassicSurvInfoProvider() );

    auto* psbdef = new uiSeisPreStackMan::BrowserDef(
				SEGYDirectSeisPS3DTranslator::translKey() );
    psbdef->tooltip_ = tr("Change file/folder names in SEG-Y file %1");
    psbdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisPreStackMan::addBrowser( psbdef );

    mAttachCB( IOM().applicationClosing, uiSEGYMgr::applicationClosingCB );
}


uiSEGYMgr::~uiSEGYMgr()
{
    detachAllNotifiers();
}


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)


void uiSEGYMgr::applicationClosingCB( CallBacker* )
{
    const ObjectSet<uiSurvInfoProvider>& sips =
				uiSurveyInfoEditor::survInfoProvs();
    for ( int idx=sips.size()-1; idx>=0; idx-- )
    {
	const uiSurvInfoProvider* sip = sips[idx];
	mDynamicCastGet(const uiSEGYSurvInfoProvider*,segysip,sip)
	if ( !segysip )
	    continue;

	const_cast<ObjectSet<uiSurvInfoProvider>&>( sips ) -=
		   const_cast<uiSurvInfoProvider*>( sip );
	delete segysip;
    }
}


void uiSEGYMgr::dTectMenuChanged()
{
    uiODMenuMgr& mnumgr = appl().menuMgr();
    const bool have2d = IOM().isBad() || SI().has2D();
    const bool only2d = !IOM().isBad() && !SI().has3D();
    uiMenu* impseismnu = mnumgr.getMnu( true, uiODApplMgr::Seis );
    auto* impsgymnu = new uiMenu( &appl(), sSEGYString(true), segyiconid_ );
    impseismnu->addMenu( impsgymnu, impseismnu->actions()[0]->getMenu() );

    uiMenu* expseismnu = mnumgr.getMnu( false, uiODApplMgr::Seis );
    auto* expsgymnu = new uiMenu( &appl(), sSEGYString(true), segyiconid_ );
    expseismnu->addMenu( expsgymnu, expseismnu->actions()[0]->getMenu() );

    if ( have2d )
    {
	const char* lineicid = "seismicline2d";
	const char* linepsicid = "prestackdataset2d";
	uiString linestr = only2d ? m3Dots(tr("Line(s)"))
				  : m3Dots(uiStrings::s2D());
	uiString linepsstr = only2d ? m3Dots(tr("Prestack Data"))
				: m3Dots(tr("Prestack 2D"));

	impsgymnu->insertAction(
		new uiAction(linestr,muiSEGYMgrCB(imp2DCB),lineicid) );
	impsgymnu->insertAction(
		new uiAction(linepsstr,muiSEGYMgrCB(imp2DPSCB),linepsicid) );
	expsgymnu->insertAction(
		new uiAction(linestr,muiSEGYMgrCB(exp2DCB),lineicid) );
	expsgymnu->insertAction(
		new uiAction(linepsstr,muiSEGYMgrCB(exp2DPSCB),linepsicid) );
    }

    if ( !only2d )
    {
	const char* volicid = "seismiccube";
	uiString volstr = have2d ? m3Dots(uiStrings::s3D())
				 : m3Dots(uiStrings::sVolume());
	const char* volpsicid = "prestackdataset";
	uiString volpsstr = have2d ? m3Dots(tr("Prestack 3D"))
				: m3Dots(tr("Prestack Volume"));

	impsgymnu->insertAction( new uiAction(volstr,muiSEGYMgrCB(imp3DCB),
					volicid) );
	impsgymnu->insertAction( new uiAction(volpsstr,muiSEGYMgrCB(imp3DPSCB),
					volpsicid) );

	expsgymnu->insertAction( new uiAction(volstr,muiSEGYMgrCB(exp3DCB),
					volicid) );
	expsgymnu->insertAction( new uiAction(volpsstr,muiSEGYMgrCB(exp3DPSCB),
					volpsicid) );
    }

    mnumgr.getMnu( true, uiODApplMgr::Wll )->insertAction(
	new uiAction( m3Dots(tr("VSP (SEG-Y)")), muiSEGYMgrCB(impVSPCB),
			"vsp0" ) );
    mnumgr.createSeisOutputMenu()->insertAction(
	new uiAction(m3Dots(tr("Re-sort Scanned SEG-Y")),
			muiSEGYMgrCB(reSortCB)) );

    if ( enableClassic() )
    {
	uiString classicmnutitle = tr("SEG-Y [Classic]");
	auto* impclassmnu = new uiMenu( &appl(), classicmnutitle, "launch" );
	impseismnu->addMenu( impclassmnu );
	impclassmnu->insertAction( new uiAction( uiStrings::sImport(),
		       muiSEGYMgrCB(impClassicCB), "import") );
	impclassmnu->insertAction( new uiAction( tr("Link"),
		       muiSEGYMgrCB(linkClassicCB), "link") );
    }
}


void uiSEGYMgr::dTectToolbarChanged()
{
    appl().menuMgr().dtectTB()->addButton( segyiconid_,
			tr( "SEG-Y import" ),
			mCB(this,uiSEGYMgr,readStarterCB) );
}


void uiSEGYMgr::cleanup()
{
    closeAndZeroPtr( impdlg_ );
    closeAndZeroPtr( expdlg_ );
}


#define mImplImpCB(typ,arg) \
void uiSEGYMgr::imp##typ##CB( CallBacker* ) \
{ \
    const SEGY::ImpType imptyp( arg ); \
    delete impdlg_; \
    impdlg_ = new uiSEGYReadStarter( &appl(), false, &imptyp ); \
    impdlg_->go(); \
}

mImplImpCB( 2D, Seis::Line )
mImplImpCB( 3D, Seis::Vol )
mImplImpCB( 2DPS, Seis::LinePS )
mImplImpCB( 3DPS, Seis::VolPS )
mImplImpCB( VSP, true )


#define mImplExpCB(typ,arg) \
void uiSEGYMgr::exp##typ##CB( CallBacker* ) \
{ \
    delete expdlg_; \
    expdlg_ = new uiSEGYExp( &appl(), arg ); \
    expdlg_->go(); \
}

mImplExpCB( 2D, Seis::Line )
mImplExpCB( 2DPS, Seis::LinePS )
mImplExpCB( 3D, Seis::Vol )
mImplExpCB( 3DPS, Seis::VolPS )


void uiSEGYMgr::impClassic( bool islink )
{
    uiSEGYRead::Setup su( islink ? uiSEGYRead::DirectDef : uiSEGYRead::Import );
    new uiSEGYRead( &appl(), su );
}


void uiSEGYMgr::reSortCB( CallBacker* )
{
    uiResortSEGYDlg dlg( &appl() );
    dlg.go();
}


void uiSEGYMgr::edFiles( CallBacker* cb )
{
    mDynamicCastGet(uiObjFileMan*,sfm,cb)
    if ( !sfm || !sfm->curIOObj() )
	return;

    uiEditSEGYFileDataDlg dlg( sfm, *sfm->curIOObj() );
    dlg.go();
}


void uiSEGYMgr::readStarterCB( CallBacker* )
{
    delete impdlg_;
    impdlg_ = new uiSEGYReadStarter( &appl(), false );
    impdlg_->go();
}


mDefODInitPlugin(uiSEGY)
{
    mDefineStaticLocalObject( PtrMan<uiSEGYMgr>, theinst_, = new uiSEGYMgr() );

    if ( !theinst_ )
	return "Cannot instantiate the SEG-Y plugin";

    return nullptr;
}
