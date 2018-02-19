
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/


#include "uisegycommon.h"

#include "segydirecttr.h"
#include "survinfo.h"
#include "dbman.h"

#include "uisegybulkimporter.h"
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
#include "uimsg.h"
#include "uitoolbar.h"
#include "envvars.h"

#include "odplugin.h"

static const char* segyiconid_ = "segy";
static const bool segyclassictoplevel_
			= GetEnvVarYN( "OD_SEGY_CLASSIC_TOPLEVEL" );


mDefODPluginInfo(uiSEGY)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"SEG-Y support",
	mODPluginODPackage,
	mODPluginCreator, mODPluginVersion,
	"Adds SEG-Y support to OpendTect") );
    return &retpi;
}


class uiSEGYMgr	: public CallBacker
{ mODTextTranslationClass(uiSEGYMgr);
public:

			uiSEGYMgr(uiODMain*);
			~uiSEGYMgr();

    uiODMain*		appl_;
    uiODMenuMgr&	mnumgr_;

    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);
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
    if ( segyclassictoplevel_ )
	uiSurveyInfoEditor::addInfoProvider(
			    new uiSEGYClassicSurvInfoProvider() );
}


uiSEGYMgr::uiSEGYMgr( uiODMain* a )
    : mnumgr_(a->menuMgr())
    , appl_(a)
{
    uiSEGY::initClasses();

    uiSeisFileMan::BrowserDef* bdef = new uiSeisFileMan::BrowserDef(
				SEGYDirectSeisTrcTranslator::translKey() );
    bdef->tooltip_ = tr("Change file/directory names in SEG-Y file %1");
    bdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisFileMan::addBrowser( bdef );

    uiSeisPreStackMan::BrowserDef* psbdef = new uiSeisPreStackMan::BrowserDef(
				SEGYDirectSeisPS3DTranslator::translKey() );
    psbdef->tooltip_ = tr("Change file/directory names in SEG-Y file %1");
    psbdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisPreStackMan::addBrowser( psbdef );

    mCallODPluginSurvRelToolsLoadFn( uiSEGY );
    mAttachCB( DBM().surveyChanged, uiSEGYMgr::updateMenu );

    updateMenu(0);
}


uiSEGYMgr::~uiSEGYMgr()
{
    detachAllNotifiers();
}


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)
#define mHandleCB muiSEGYMgrCB(handleImpExpMnu)
#define mImpStartID 100
#define mExpStartID 200


void uiSEGYMgr::updateMenu( CallBacker* )
{
    uiMenu* impseismnu = mnumgr_.getMnu( true, uiODApplMgr::Seis );
    uiMenu* impsgymnu = mnumgr_.addFullSeisSubMenu( impseismnu,
		sSEGYString(true), segyiconid_, mHandleCB, mImpStartID );
    uiMenu* expseismnu = mnumgr_.getMnu( false, uiODApplMgr::Seis );
    mnumgr_.addFullSeisSubMenu( expseismnu,
		sSEGYString(true), segyiconid_, mHandleCB, mExpStartID );

    mnumgr_.impWellLogsMenu()->insertAction(
	new uiAction( m3Dots(tr("VSP (SEG-Y)")), muiSEGYMgrCB(impVSPCB),
			"vsp0" ) );
    mnumgr_.createSeisOutputMenu()->insertAction(
	new uiAction(m3Dots(tr("Re-sort Scanned SEG-Y")),
			    muiSEGYMgrCB(reSortCB), "shuffle_data") );

    uiString classicmnutitle = segyclassictoplevel_ ? tr("SEG-Y [Classic]")
						   : tr("Classic tool");
    uiMenu* impclassmnu = new uiMenu( appl_, classicmnutitle, "launch" );
    (segyclassictoplevel_ ? impseismnu : impsgymnu)->addMenu( impclassmnu );
    impclassmnu->insertAction( new uiAction( uiStrings::sImport(),
		   muiSEGYMgrCB(impClassicCB), "import") );
    impclassmnu->insertAction( new uiAction( uiStrings::sLink(),
		   muiSEGYMgrCB(linkClassicCB), "link") );

    int segyimp = mnumgr_.dtectTB()->addButton( segyiconid_,
						tr("SEG-Y import") );

    uiMenu* mnu = mnumgr_.dtectTB()->addButtonMenu( segyimp,
						uiToolButton::InstantPopup);
    mnu->insertAction(new uiAction(m3Dots(tr("Single-Vintage")),
		    mCB(this,uiSEGYMgr,readStarterCB),"singlefile") );
    mnu->insertAction(new uiAction(m3Dots(tr("Multiple-Vintage")),
		    mCB(this,uiSEGYMgr,bulkImport),"copyobj") );
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
	uiSEGYReadStarter dlg( appl_, su );
	dlg.go();
    }
    else
    {
	uiSEGYExp dlg( appl_, gt );
	dlg.go();
    }
}


void uiSEGYMgr::impVSPCB( CallBacker* )
{
    const SEGY::ImpType imptyp( true );
    uiSEGYReadStarter::Setup su( false, &imptyp );
    uiSEGYReadStarter dlg( appl_, su );
    dlg.go();
}


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
    mDynamicCastGet(uiObjFileMan*,sfm,cb)
    if ( !sfm || !sfm->curIOObj() )
	return;

    uiEditSEGYFileDataDlg dlg( sfm, *sfm->curIOObj() );
    dlg.go();
}


void uiSEGYMgr::readStarterCB( CallBacker* cb )
{
    uiSEGYReadStarter readstdlg( ODMainWin(), uiSEGYReadStarter::Setup(false) );
    readstdlg.go();
}


void uiSEGYMgr::bulkImport( CallBacker* )
{
    uiSEGYMultiVintageImporter bulkimpdlg( ODMainWin() );
    if ( !bulkimpdlg.nrSelFiles() )
	return;

    bulkimpdlg.go();
}

mDefODInitPlugin(uiSEGY)
{
    mDefineStaticLocalObject( PtrMan<uiSEGYMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiSEGYMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate SEG-Y plugin";

    return 0;
}
