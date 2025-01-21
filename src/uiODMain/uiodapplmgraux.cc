/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "ctxtioobj.h"
#include "datapointset.h"
#include "filepath.h"
#include "genc.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "oddirs.h"
#include "odinst.h"
#include "odsession.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "string2.h"
#include "survinfo.h"
#include "mousecursor.h"

#include "uiioobjseldlg.h"
#include "ui2dgeomman.h"
#include "uibatchlaunch.h"
#include "uibatchprestackproc.h"
#include "uibatchprogs.h"
#include "uicoltabimport.h"
#include "uicoltabman.h"
#include "uiconvpos.h"
#include "uidatapointset.h"
#include "uidatapointsetio.h"
#include "uidatapointsetman.h"
#include "uiimpexppdf.h"
#include "uiimpexp2dgeom.h"
#include "uiimppvds.h"
#include "uimanprops.h"
#include "uimsg.h"
#include "uipluginman.h"
#include "uiprestackanglemutecomputer.h"
#include "uiprestackexpmute.h"
#include "uiprestackimpmute.h"
#include "uiprobdenfuncman.h"
#include "uirandomlineman.h"
#include "uiseisbayesclass.h"
#include "uiseis2dfrom3d.h"
#include "uiseis2dto3d.h"
#include "uiseis2dto3dinterpol.h"
#include "uiselsimple.h"
#include "uishortcuts.h"
#include "uistrattreewin.h"
#include "uistrings.h"
#include "uivelocityfunctionimp.h"
#include "uivisdatapointsetdisplaymgr.h"

#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uipickpartserv.h"
#include "uiseispartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"


uiODApplService::uiODApplService( uiParent* p, uiODApplMgr& am )
    : uiApplService(p,am,"OpendTect")
{}


uiODApplService::~uiODApplService()
{}


//uiODApplMgrDispatcher

uiODApplMgrDispatcher::uiODApplMgrDispatcher( uiODApplMgr& a, uiParent* p )
    : am_(a)
    , par_(p)
{}


uiODApplMgrDispatcher::~uiODApplMgrDispatcher()
{
    deleteDlgs();
    deepErase( uidpsset_ );
}


void uiODApplMgrDispatcher::survChg( bool before )
{
    if ( before )
	deleteDlgs();

    deepErase( uidpsset_ );
}


void uiODApplMgrDispatcher::deleteDlgs()
{
    closeAndNullPtr( convposdlg_ );
    closeAndNullPtr( mandpsdlg_ );
    closeAndNullPtr( manpropsdlg_ );
    closeAndNullPtr( man2dgeomdlg_ );
    closeAndNullPtr( manpdfdlg_ );
    closeAndNullPtr( mansessiondlg_ );
    closeAndNullPtr( impcrossplotdlg_ );
    closeAndNullPtr( expcrossplotdlg_ );
    closeAndNullPtr( impmutedlg_ );
    closeAndNullPtr( imppdfdlg_ );
    closeAndNullPtr( exppdfdlg_ );
    closeAndNullPtr( impvelfunc2d_ );
    closeAndNullPtr( impvelfunc3d_ );
    closeAndNullPtr( exp2dgeomdlg_ );
    closeAndNullPtr( imp2dgeomdlg_ );
    closeAndNullPtr( batchprocps2ddlg_ );
    closeAndNullPtr( batchprocps3ddlg_ );
}


void uiODApplMgrDispatcher::doOperation( int iot, int iat, int opt )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    const uiODApplMgr::ActType at = (uiODApplMgr::ActType)iat;

    uiEMPartServer& emserv = *am_.EMServer();
    uiPickPartServer& pickserv = *am_.pickServer();
    uiSeisPartServer& seisserv = *am_.seisServer();
    uiWellPartServer& wellserv = *am_.wellServer();
    uiAttribPartServer& attrserv = *am_.attrServer();
    uiEMAttribPartServer& emattrserv = *am_.EMAttribServer();

    switch ( ot )
    {
    case uiODApplMgr::NrObjTypes:
    break;
    case uiODApplMgr::Seis:
	switch ( at )
	{
	case uiODApplMgr::Exp:	seisserv.exportSeis( opt );	break;
	case uiODApplMgr::Man:	seisserv.manageSeismics( opt ); break;
	case uiODApplMgr::Imp:	seisserv.importSeis( opt );	break;
	}
    break;
    case uiODApplMgr::Hor:
	switch ( at )
	{
	case uiODApplMgr::Imp:
	    if ( opt == 0 )
		emserv.import3DHorGeom();
	    else if ( opt == 1 )
		emserv.import3DHorAttr();
	    else if ( opt == 2 )
		emattrserv.import2DHorizon();
	    else if ( opt == 3 )
		emserv.import3DHorGeom( true );
	    else if ( opt == 4 )
		emserv.importBulk2DHorizon();
	    else if ( opt == 5 )
		emserv.importHorFromZMap();
	break;
	case uiODApplMgr::Exp:
	    if ( opt == 0 )
		emserv.export3DHorizon();
	    else if ( opt == 1 )
		emserv.export2DHorizon();
	    else if ( opt == 2 )
		emserv.export3DHorizon(true);
	    else if ( opt == 3 )
		emserv.export2DHorizon(true);
	break;
	case uiODApplMgr::Man:
	    if ( opt == 0 )
		opt = SI().has3D() ? 2 : 1;

	    if ( opt == 1 )
		emserv.manage2DHorizons();
	    else if ( opt == 2 )
		emserv.manage3DHorizons();
	break;
	}
    break;
    case uiODApplMgr::Flt:
	switch( at )
	{
	case uiODApplMgr::Imp:
	    if ( opt==0 )
		emserv.importFault();
	    else if ( opt==1 )
		emserv.importBulkFaults();
	break;
	case uiODApplMgr::Exp:
	    emserv.exportFault( opt==0 );
	break;
	case uiODApplMgr::Man:
	    if ( opt == 0 )
		opt = SI().has3D() ? 2 : 1;

	    if ( opt == 1 )
		emserv.manageFaultStickSets();
	    else if ( opt == 2 )
		emserv.manage3DFaults();
	break;
	}
    break;
    case uiODApplMgr::Fltss:
	switch ( at )
	{
	    case uiODApplMgr::Imp:
	    if ( opt == 0 )
		emserv.importFaultStickSet();
	    else if ( opt == 1 )
		emserv.import2DFaultStickset();
	    else if ( opt == 2 )
		emserv.importBulkFaultStickSet();
	    else if ( opt == 3 )
		emserv.importBulk2DFaultStickset();
	    break;
	    case uiODApplMgr::Exp:
		emserv.exportFaultStickSet( opt==0 );
	    break;
	    default:
		break;
	}
    break;
    case uiODApplMgr::FltSet:
	switch( at )
	{
	case uiODApplMgr::Imp:
	    emserv.importFaultSet();
	break;
	case uiODApplMgr::Exp:
	    emserv.exportFaultSet();
	break;
	case uiODApplMgr::Man:
	    emserv.manageFaultSets();
	break;
	}
    break;
    case uiODApplMgr::Wll:
	switch ( at )
	{
	case uiODApplMgr::Imp:
	    if ( opt == 0 )
		wellserv.importTrack();
	    else if ( opt == 1 )
		wellserv.importLogs();
	    else if ( opt == 2 )
		wellserv.importMarkers();
	    else if ( opt == 4 )
		wellserv.createSimpleWells();
	    else if ( opt == 5 )
		wellserv.bulkImportTrack();
	    else if ( opt == 6 )
		wellserv.bulkImportLogs();
	    else if ( opt == 7 )
		wellserv.bulkImportMarkers();
	    else if ( opt == 8 )
		wellserv.bulkImportD2TModel();
	    else if ( opt == 9 )
		wellserv.bulkImportDirectional();
	break;
	case uiODApplMgr::Exp:
	    if ( opt == 0 )
		wellserv.exportWellData();
	    else if ( opt == 1 )
		wellserv.exportLogToLAS();
	break;
	case uiODApplMgr::Man:	wellserv.manageWells();
	break;
	default:	break;
	}
    break;
    case uiODApplMgr::Attr:
	switch( at )
	{
	case uiODApplMgr::Man: attrserv.manageAttribSets( opt==1 );
	break;
	case uiODApplMgr::Imp:
	    if ( opt == 0 )
		attrserv.importAttrSetFromFile();
	    else if ( opt == 1 )
		attrserv.importAttrSetFromOtherSurvey();
	break;
	default:	break;
	}
    break;
    case uiODApplMgr::Pick:
	switch ( at )
	{
	case uiODApplMgr::Imp:	pickserv.importSet();		break;
	case uiODApplMgr::Exp:	pickserv.exportSet();		break;
	case uiODApplMgr::Man:	pickserv.managePickSets();	break;
	}
    break;
    case uiODApplMgr::Wvlt:
	switch ( at )
	{
	case uiODApplMgr::Imp:	seisserv.importWavelets();	break;
	case uiODApplMgr::Exp:	seisserv.exportWavelets();	break;
	default:		seisserv.manageWavelets();	break;
	}
    break;
    case uiODApplMgr::MDef:
	if ( at == uiODApplMgr::Imp )
	{
	    if ( !impmutedlg_ )
	    {
		impmutedlg_ = new PreStack::uiImportMute( par_ );
		impmutedlg_->setModal( false );
	    }

	    impmutedlg_->show();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    if ( !expmutedlg_ )
	    {
		expmutedlg_ = new PreStack::uiExportMute( par_ );
		expmutedlg_->setModal( false );
	    }

	    expmutedlg_->show();
	}
    break;
    case uiODApplMgr::Vel:
	if ( at == uiODApplMgr::Imp)
	{
	    if ( opt==0 )
	    {
		if ( !impvelfunc3d_ )
		    impvelfunc3d_ = new Vel::uiImportVelFunc( par_, false );

		impvelfunc3d_->show();
	    }
	    if ( opt==1 )
	    {
		if ( !impvelfunc2d_ )
		    impvelfunc2d_ = new Vel::uiImportVelFunc( par_, true );

		impvelfunc2d_->show();
	    }
	}
    break;
    case uiODApplMgr::Strat:
	StratTWin().popUp();
    break;
    case uiODApplMgr::PDF:
	if ( at == uiODApplMgr::Imp )
	{
	    if ( !imppdfdlg_ )
		imppdfdlg_ = new uiImpRokDocPDF( par_ );

	    imppdfdlg_->show();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    if ( !exppdfdlg_ )
		exppdfdlg_ = new uiExpRokDocPDF( par_ );

	    exppdfdlg_->show();
	}
	else if ( at == uiODApplMgr::Man )
	{
	    delete manpdfdlg_;
	    manpdfdlg_ = new uiProbDenFuncMan( par_ );
	    manpdfdlg_->go();
	}
    break;
    case uiODApplMgr::Geom:
	if ( at == uiODApplMgr::Man )
	{
	    delete man2dgeomdlg_;
	    man2dgeomdlg_ = new ui2DGeomManageDlg( par_ );
	    man2dgeomdlg_->go();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    if ( !exp2dgeomdlg_ )
		exp2dgeomdlg_ = new uiExp2DGeom( par_ );

	    exp2dgeomdlg_->show();
	}
	else if ( at == uiODApplMgr::Imp )
	{
	    if ( opt==0 )
	    {
		if ( !imp2dgeomdlg_ )
		{
		    imp2dgeomdlg_ = new uiImp2DGeom( par_ );
		    imp2dgeomdlg_->setModal( false );
		}

		imp2dgeomdlg_->show();
	    }
	}
    break;
    case uiODApplMgr::PVDS:
	if ( at == uiODApplMgr::Imp )
	{
	    if ( !impcrossplotdlg_ )
		impcrossplotdlg_ = new uiImpPVDS( par_ );

	    impcrossplotdlg_->show();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    if ( !expcrossplotdlg_ )
		expcrossplotdlg_ = new uiExportDataPointSet( par_ );

	    expcrossplotdlg_->show();
	}
	else if ( at == uiODApplMgr::Man )
	{
	    delete mandpsdlg_;
	    mandpsdlg_ = new uiDataPointSetMan( par_ );
	    mandpsdlg_->show();
	}
    break;
    case uiODApplMgr::Body:
	if ( at == uiODApplMgr::Man )
	    emserv.manageBodies();
    break;
    case uiODApplMgr::Props:
	if ( at == uiODApplMgr::Man )
	{
	    delete manpropsdlg_;
	    manpropsdlg_ = new uiManPROPS( par_ );
	    manpropsdlg_->setModal( false );
	    manpropsdlg_->show();
	}
    break;
    case uiODApplMgr::Sess:
	if ( at == uiODApplMgr::Man )
	{
	    delete mansessiondlg_;
	    mansessiondlg_ = new uiSessionMan( par_ );
	    mansessiondlg_->show();
	}
    break;
    case uiODApplMgr::NLA:
	    pErrMsg("NLA event occurred");
    break;
    case uiODApplMgr::ColTab:
	if ( at == uiODApplMgr::Man )
	{
	    ColTab::Sequence ctseq( "" );
	    uiColorTableMan dlg( par_, ctseq, true );
	    dlg.go();
	}
	else if ( at == uiODApplMgr::Imp )
	{
	    uiColTabImport dlg( par_ );
	    dlg.go();
	}
    break;
    case uiODApplMgr::RanL:
	if ( at == uiODApplMgr::Man )
	{
	    delete manrldlg_;
	    manrldlg_ = new uiRandomLineMan( par_ );
	    manrldlg_->show();
	}
    break;
    }
}


void uiODApplMgrDispatcher::manPreLoad( int iot )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    switch ( ot )
    {
	case uiODApplMgr::Seis:
	    am_.seisServer()->managePreLoad();
	break;
	case uiODApplMgr::Hor:
	    am_.EMServer()->managePreLoad();
	break;
	default:
	break;
    }
}


void uiODApplMgrDispatcher::posConversion()
{
    if ( !convposdlg_ )
    {
	convposdlg_ = new uiConvertPos( par_, SI(), false );
	convposdlg_->windowClosed.notify(
				mCB(this,uiODApplMgrDispatcher,posDlgClose) );
	convposdlg_->setDeleteOnClose( true );
    }
    convposdlg_->show();
}


void uiODApplMgrDispatcher::posDlgClose( CallBacker* )
{
    convposdlg_ = nullptr;
}


void uiODApplMgrDispatcher::openXPlot()
{
    IOObjContext ctxt = PosVecDataSetTranslatorGroup::ioContext();
    ctxt.forread_ = true;
    ctxt.setName( "Cross-plot Data" );
    uiIOObjSelDlg seldlg( par_, ctxt );
    seldlg.setHelpKey( mODHelpKey(mOpenCossplotHelpID) );
    if ( !seldlg.go() || !seldlg.ioObj() ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );

    PosVecDataSet pvds;
    uiString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    MouseCursorManager::restoreOverride();

    if ( !rv )
    {
	uiMSG().error( errmsg );
	return;
    }

    if ( pvds.data().isEmpty() )
    {
	uiMSG().error(uiDataPointSetMan::sSelDataSetEmpty());
	return;
    }

    RefMan<DataPointSet> newdps = new DataPointSet( pvds, false );
    newdps->setName( seldlg.ioObj()->name() );
    DPM(DataPackMgr::PointID()).add( newdps );
    uiDataPointSet* uidps =
	new uiDataPointSet( ODMainWin(), *newdps,
	uiDataPointSet::Setup(tr("Cross-plot Data: %1").arg(newdps->name())),
	ODMainWin()->applMgr().visDPSDispMgr() );
    uidps->go();
    uidpsset_ += uidps;
}


class uiUpdateInfoDlg : public uiDialog
{ mODTextTranslationClass(uiUpdateInfoDlg);
public:
		uiUpdateInfoDlg(uiParent*,uiString&);

protected:
    bool	acceptOK(CallBacker*) override;
    uiCheckBox* relnotesbut_;
};


uiUpdateInfoDlg::uiUpdateInfoDlg( uiParent* p, uiString& infomsg )
    : uiDialog( p, uiDialog::Setup(tr("Information"),infomsg,
				   "mNoHelpID"))
{
    setCancelText( uiString::emptyString() );
    relnotesbut_ = new uiCheckBox( this, tr("Show release notes") );
    relnotesbut_->setChecked( true );
}


bool uiUpdateInfoDlg::acceptOK( CallBacker* )
{
    if ( relnotesbut_->isChecked() )
	uiODApplMgr::showReleaseNotes( true );

    return true;
}


void uiODApplMgrDispatcher::startInstMgr()
{
#ifdef __win__
    uiString msg = tr("Please close OpendTect application and all other "
		      "OpendTect processes before proceeding for"
		      " installation/update");
#else
    uiString msg = tr("If you make changes to the application,\nplease "
		      "restart OpendTect for the changes to take effect.");
#endif

    uiUpdateInfoDlg dlg( par_, msg );
    dlg.go();
    ODInst::startInstManagement();
}


void uiODApplMgrDispatcher::setAutoUpdatePol()
{
    const ODInst::AutoInstType curait = ODInst::getAutoInstType();
    BufferStringSet options, alloptions;
    alloptions = ODInst::autoInstTypeUserMsgs();
    options.add( alloptions.get( (int)ODInst::InformOnly) );
    options.add( alloptions.get( (int)ODInst::NoAuto) );

    uiGetChoice dlg( par_, options,
			tr("Select policy for auto-update"), true,
		       mODHelpKey(mODApplMgrDispatchersetAutoUpdatePolHelpID));

    const int idx = options.indexOf( alloptions.get((int)curait) );
    dlg.setDefaultChoice( idx < 0 ? 0 : idx );
    if ( !dlg.go() )
	return;

    ODInst::AutoInstType newait = (ODInst::AutoInstType)
				alloptions.indexOf( options.get(dlg.choice()));
    if ( newait != curait )
	ODInst::setAutoInstType( newait );

    if ( newait == ODInst::InformOnly )
	am_.updateCaption();
}


#define mPreStackBatchdlg(dlgobj) \
	if ( !dlgobj ) \
	    dlgobj = new PreStack::uiBatchProcSetup( par_, gs ); \
	dlgobj->show(); \

void uiODApplMgrDispatcher::processPreStack( bool is2d )
{
    const OD::GeomSystem gs = is2d ? OD::Geom2D : OD::Geom3D;
    if ( is2d )
    {
	if ( OD::InDebugMode() )
	{
	    mPreStackBatchdlg( batchprocps2ddlg_ )
	}
	else
	{
	    uiMSG().message( tr("Coming soon") );
	    return;
	}
    }
    else
    { mPreStackBatchdlg(batchprocps3ddlg_) }
}


void uiODApplMgrDispatcher::genAngleMuteFunction()
{ PreStack::uiAngleMuteComputer dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::bayesClass( bool is2d )
{ new uiSeisBayesClass( ODMainWin(), is2d ); }
void uiODApplMgrDispatcher::startBatchJob()
{ uiStartBatchJobDialog dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::batchProgs()
{ uiBatchProgLaunch dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::pluginMan()
{ uiPluginMan dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::manageShortcuts()
{ uiShortcutsDlg dlg( par_, "ODScene" ); dlg.go(); }
void uiODApplMgrDispatcher::createCubeFromWells()
{ am_.wellAttribServer()->createLogCube( MultiID::udf() ); }

void uiODApplMgrDispatcher::process2D3D( int opt )
{
    if ( opt==0 )
	am_.EMAttribServer()->create2DGrid( nullptr );
    else if ( opt==1 )
    {
	uiSeis2DFrom3D dlg( par_ );
	dlg.go();
    }
    else if ( opt==2 )
    {
	uiSeis2DTo3D dlg( par_ );
	dlg.go();
    }
    else if ( opt==3 )
    {
	uiString str = uiStrings::phrCreate(tr("3D cube from 2D DataSet"));
	uiSeis2DTo3DInterPol dlg( par_, str );
	dlg.go();
    }
}


void uiODApplMgrDispatcher::setupBatchHosts()
{
    OS::MachineCommand mc(
		FilePath(GetExecPlfDir(),
			 ODInst::sKeyODBatchHostsExecNm()).fullPath());
    const OS::CommandExecPars pars( OS::RunInBG );
    mc.execute(pars);
}
