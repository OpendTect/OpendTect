
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : May 2007
-*/

#include "uisegymod.h"
#include "segydirecttr.h"
#include "survinfo.h"
#include "dbman.h"

#include "uisegymultivintageimporter.h"
#include "uisegydef.h"
#include "uisegydirectinserter.h"
#include "uisegysip.h"
#include "uisegysipclassic.h"
#include "uisegydefdlg.h"
#include "uisegyexp.h"
#include "uisegyread.h"
#include "uisegyresortdlg.h"
#include "uiwellimpsegyvsp.h"
#include "uisegyreadstarter.h"
#include "uisegyimptype.h"
#include "uisegyread.h"

#include "uifileselector.h"
#include "uisurvinfoed.h"
#include "uiseisfileman.h"
#include "uiseispsman.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uiodmain.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "envvars.h"

#include "odplugin.h"

static const char* segyiconid_ = "segy";

static bool wantClassicTopLevel()
{
    static const bool yn = GetEnvVarYN( "OD_SEGY_CLASSIC_TOPLEVEL" );
    return yn;
}


mDefODPluginInfo(uiSEGY)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"SEG-Y support",
	"OpendTect",
	"dGB (Bert)",
	"=od",
	"Supports the SEG-Y format") );
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

    void		dTectMenuChanged() override;
    void		dTectToolbarChanged() override;
    void		cleanup() override;

    void		edFiles(CallBacker*);
    void		handleImpExpMnu(CallBacker*);
    void		impVSPCB(CallBacker*);
    void		reSortCB(CallBacker*);
    void		readStarterCB(CallBacker*);
    void		bulkImport(CallBacker*);

    void		impClassicCB( CallBacker* )	{ impClassic( false ); }
    void		linkClassicCB( CallBacker* )	{ impClassic( true ); }
    void		impClassic(bool);

    static const uiString sSEGYString( bool imm )
			 { return imm ? tr("SEG-Y") : m3Dots(tr("SEG-Y")); }

};


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)

mDefODPluginSurvRelToolsLoadFn(uiSEGY)
{
    uiSurveyInfoEditor::addInfoProvider( new uiSEGYSurvInfoProvider() );
    if ( wantClassicTopLevel() )
	uiSurveyInfoEditor::addInfoProvider(
			    new uiSEGYClassicSurvInfoProvider() );
}


uiSEGYMgr::uiSEGYMgr()
    : uiPluginInitMgr()
{
    init();
    auto* bdef = new uiSeisFileMan::BrowserDef(
				SEGYDirectSeisTrcTranslator::translKey() );
    bdef->tooltip_ = tr("Change file/directory names in SEG-Y file %1");
    bdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisFileMan::addBrowser( bdef );

    auto* psbdef = new uiSeisPreStackMan::BrowserDef(
				SEGYDirectSeisPS3DTranslator::translKey() );
    psbdef->tooltip_ = tr("Change file/directory names in SEG-Y file %1");
    psbdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisPreStackMan::addBrowser( psbdef );

    mCallODPluginSurvRelToolsLoadFn( uiSEGY );
}


uiSEGYMgr::~uiSEGYMgr()
{
    detachAllNotifiers();
}


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)
#define mHandleCB muiSEGYMgrCB(handleImpExpMnu)
#define mImpStartID 100
#define mExpStartID 200


void uiSEGYMgr::dTectMenuChanged()
{
    auto& mnumgr = appl().menuMgr();
    uiMenu* impseismnu = mnumgr.getMnu( true, uiODApplMgr::Seis );
    uiMenu* impsgymnu = mnumgr.addFullSeisSubMenu( impseismnu,
		sSEGYString(true), segyiconid_, mHandleCB, mImpStartID );
    uiMenu* expseismnu = mnumgr.getMnu( false, uiODApplMgr::Seis );
    mnumgr.addFullSeisSubMenu( expseismnu,
		sSEGYString(true), segyiconid_, mHandleCB, mExpStartID );

    mnumgr.impWellLogsMenu()->insertAction(
	new uiAction( m3Dots(tr("VSP (SEG-Y)")), muiSEGYMgrCB(impVSPCB),
			"vsp0" ) );
    mnumgr.createSeisOutputMenu()->insertAction(
	new uiAction(m3Dots(tr("Re-sort Scanned SEG-Y")),
			    muiSEGYMgrCB(reSortCB), "shuffle_data") );

    uiString classicmnutitle = wantClassicTopLevel() ? tr("SEG-Y [Classic]")
						   : tr("Classic tool");
    uiMenu* impclassmnu = new uiMenu( &appl_, classicmnutitle, "launch" );
    (wantClassicTopLevel() ? impseismnu : impsgymnu)->addMenu( impclassmnu );
    impclassmnu->insertAction( new uiAction( uiStrings::sImport(),
		   muiSEGYMgrCB(impClassicCB), "import") );
    impclassmnu->insertAction( new uiAction( uiStrings::sLink(),
		   muiSEGYMgrCB(linkClassicCB), "link") );
}


void uiSEGYMgr::dTectToolbarChanged()
{
    auto& mnumgr = appl().menuMgr();
    int segyimp = mnumgr.dtectTB()->addButton( segyiconid_,
						tr("SEG-Y import") );
    uiMenu* mnu = mnumgr.dtectTB()->addButtonMenu( segyimp,
						uiToolButton::InstantPopup);
    mnu->insertAction(new uiAction(m3Dots(tr("Single-Vintage")),
		    mCB(this,uiSEGYMgr,readStarterCB),"singlefile") );
    mnu->insertAction(new uiAction(m3Dots(tr("Multiple-Vintage")),
		    mCB(this,uiSEGYMgr,bulkImport),"copyobj") );
}


void uiSEGYMgr::cleanup()
{
    closeAndZeroPtr( impdlg_ );
    closeAndZeroPtr( expdlg_ );
    uiPluginInitMgr::cleanup();
}


void uiSEGYMgr::handleImpExpMnu( CallBacker* cb )
{
    mDynamicCastGet(uiAction*,itm,cb)
    if ( !itm )
	{ pErrMsg("Huh?"); return; }

    int id = itm->getID();
    const bool isimp = id < mExpStartID;
    if ( isimp )
	id -= mImpStartID;
    else
	id -= mExpStartID;

    const Seis::GeomType gt = (Seis::GeomType)id;
    if ( isimp )
    {
	SEGY::ImpType imptyp( gt );
	uiSEGYReadStarter::Setup su( false, &imptyp );
	delete impdlg_;
	impdlg_ = new uiSEGYReadStarter( &appl_, su );
	impdlg_->setModal( false );
	impdlg_->go();
    }
    else
    {
	delete expdlg_;
	expdlg_ = new uiSEGYExp( &appl_, gt );
	expdlg_->setModal( false );
	expdlg_->go();
    }
}


void uiSEGYMgr::impVSPCB( CallBacker* )
{
    const SEGY::ImpType imptyp( true );
    uiSEGYReadStarter::Setup su( false, &imptyp );
    uiSEGYReadStarter dlg( &appl_, su );
    dlg.go();
}


void uiSEGYMgr::impClassic( bool islink )
{
    uiSEGYRead::Setup su( islink ? uiSEGYRead::DirectDef : uiSEGYRead::Import );
    new uiSEGYRead( &appl_, su );
}


void uiSEGYMgr::reSortCB( CallBacker* )
{
    uiResortSEGYDlg dlg( &appl_ );
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
    impdlg_ = new uiSEGYReadStarter( &appl_,
				     uiSEGYReadStarter::Setup(false) );
    impdlg_->setModal( false );
    impdlg_->go();
}


void uiSEGYMgr::bulkImport( CallBacker* )
{
    uiSEGYMultiVintageImporter bulkimpdlg( &appl_ );
    if ( !bulkimpdlg.nrSelFiles() )
	return;

    bulkimpdlg.go();
}

mDefODInitPlugin(uiSEGY)
{
    mDefineStaticLocalObject( PtrMan<uiSEGYMgr>, theinst_, = new uiSEGYMgr() );

    if ( !theinst_ )
	return "Cannot instantiate SEG-Y plugin";

    return nullptr;
}
