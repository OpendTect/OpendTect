/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"

#include "attribdescset.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "datapackbase.h"
#include "embodytr.h"
#include "emsurfacetr.h"
#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "odsession.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "separstr.h"
#include "string2.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "veldesc.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "vissurvscene.h"

#include "ui2dgeomman.h"
#include "uibatchhostsdlg.h"
#include "uibatchlaunch.h"
#include "uibatchprestackproc.h"
#include "uibatchprogs.h"
#include "uicoltabimport.h"
#include "uicoltabman.h"
#include "uiconvpos.h"
#include "uicreatelogcubedlg.h"
#include "uidatapointset.h"
#include "uidatapointsetman.h"
#include "uifontsel.h"
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
#include "uiveldesc.h"
#include "uivelocityfunctionimp.h"
#include "uivisdatapointsetdisplaymgr.h"

#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uinlapartserv.h"
#include "uipickpartserv.h"
#include "uiseispartserv.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"
#include "od_helpids.h"
#include "winutils.h"
#include "commandlineparser.h"


bool uiODApplService::eventOccurred( const uiApplPartServer* ps, int evid )
{
    return applman_.handleEvent( ps, evid );
}


void* uiODApplService::getObject( const uiApplPartServer* ps, int evid )
{
    return applman_.deliverObject( ps, evid );
}


uiParent* uiODApplService::parent() const
{
    return par_;
}


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
    closeAndZeroPtr( convposdlg_ );
    closeAndZeroPtr( mandpsdlg_ );
    closeAndZeroPtr( manpropsdlg_ );
    closeAndZeroPtr( man2dgeomdlg_ );
    closeAndZeroPtr( manpdfdlg_ );
    closeAndZeroPtr( mansessiondlg_ );
    closeAndZeroPtr( impcrossplotdlg_ );
    closeAndZeroPtr( impmutedlg_ );
    closeAndZeroPtr( imppdfdlg_ );
    closeAndZeroPtr( exppdfdlg_ );
    closeAndZeroPtr( impvelfunc_ );
    closeAndZeroPtr( exp2dgeomdlg_ );
    closeAndZeroPtr( imp2dgeomdlg_ );
    closeAndZeroPtr( batchprocps2ddlg_ );
    closeAndZeroPtr( batchprocps3ddlg_ );
}


#define mCase(val) case uiODApplMgr::val

void uiODApplMgrDispatcher::doOperation( int iot, int iat, int opt )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    const uiODApplMgr::ActType at = (uiODApplMgr::ActType)iat;

    switch ( ot )
    {
    mCase(NrObjTypes): break;
    mCase(Seis):
	switch ( at )
	{
	mCase(Exp):	am_.seisserv_->exportSeis( opt );	break;
	mCase(Man):	am_.seisserv_->manageSeismics( opt );	break;
	mCase(Imp):	am_.seisserv_->importSeis( opt );	break;
	}
    break;
    mCase(Hor):
	switch ( at )
	{
	mCase(Imp):
	    if ( opt == 0 )
		am_.emserv_->import3DHorGeom();
	    else if ( opt == 1 )
		am_.emserv_->import3DHorAttr();
	    else if ( opt == 2 )
		am_.emattrserv_->import2DHorizon();
	    else if ( opt == 3 )
		am_.emserv_->import3DHorGeom( true );
	    else if ( opt == 4 )
		am_.emserv_->importBulk2DHorizon();
	    else if ( opt == 5 )
		am_.emserv_->importHorFromZMap();
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->export3DHorizon();
	    else if ( opt == 1 )
		am_.emserv_->export2DHorizon();
	    else if ( opt == 2 )
		am_.emserv_->export3DHorizon(true);
	    else if ( opt == 3 )
		am_.emserv_->export2DHorizon(true);
	    break;
	mCase(Man):
	    if ( opt == 0 ) opt = SI().has3D() ? 2 : 1;
	    if ( opt == 1 )
		am_.emserv_->manage2DHorizons();
	    else if ( opt == 2 )
		am_.emserv_->manage3DHorizons();
	    break;
	}
    break;
    mCase(Flt):
	switch( at )
	{
	mCase(Imp):
	    if ( opt==0 ) am_.emserv_->importFault();
	    if ( opt==1 ) am_.emserv_->importBulkFaults();
	    break;
	mCase(Exp):
	    am_.emserv_->exportFault( opt==0 );
	    break;
	mCase(Man):
	    if ( opt == 0 ) opt = SI().has3D() ? 2 : 1;
	    if ( opt == 1 )
		am_.emserv_->manageFaultStickSets();
	    else if ( opt == 2 )
		am_.emserv_->manage3DFaults();
	    break;
	}
    break;
    mCase(Fltss):
	switch ( at )
	{
	    mCase(Imp):
	    if ( opt == 0 )
		am_.emserv_->importFaultStickSet();
	    else if ( opt == 1 )
		am_.emserv_->import2DFaultStickset();
	    else if ( opt == 2 )
		am_.emserv_->importBulkFaultStickSet();
	    else if ( opt == 3 )
		am_.emserv_->importBulk2DFaultStickset();
	    break;
	    mCase(Exp):
		am_.emserv_->exportFaultStickSet( opt==0 );
	    break;
	    default:
		break;
	}
    break;
    mCase(FltSet):
	switch( at )
	{
	mCase(Imp):
	    am_.emserv_->importFaultSet();
	    break;
	mCase(Exp):
	    am_.emserv_->exportFaultSet();
	    break;
	mCase(Man):
	    am_.emserv_->manageFaultSets();
	    break;
	}
    break;

    mCase(Wll):
	switch ( at )
	{
	mCase(Imp):
	    if ( opt == 0 )
		am_.wellserv_->importTrack();
	    else if ( opt == 1 )
		am_.wellserv_->importLogs();
	    else if ( opt == 2 )
		am_.wellserv_->importMarkers();
	    else if ( opt == 4 )
		am_.wellserv_->createSimpleWells();
	    else if ( opt == 5 )
		am_.wellserv_->bulkImportTrack();
	    else if ( opt == 6 )
		am_.wellserv_->bulkImportLogs();
	    else if ( opt == 7 )
		am_.wellserv_->bulkImportMarkers();
	    else if ( opt == 8 )
		am_.wellserv_->bulkImportD2TModel();
	    else if ( opt == 9 )
		am_.wellserv_->bulkImportDirectional();
	break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.wellserv_->exportWellData();
	    else if ( opt == 1 )
		am_.wellserv_->exportLogToLAS();
	break;
	mCase(Man):	am_.wellserv_->manageWells();	break;
	default:					break;
	}
    break;
    mCase(Attr):
	switch( at )
	{
	mCase(Man): am_.attrserv_->manageAttribSets(opt==1);  break;
	mCase(Imp):
	    if ( opt == 0 )
		am_.attrserv_->importAttrSetFromFile();
	    else if ( opt == 1 )
		am_.attrserv_->importAttrSetFromOtherSurvey();
	break;
	default:					    break;
	}
    break;
    mCase(Pick):
	switch ( at )
	{
	mCase(Imp):	am_.pickserv_->importSet();		break;
	mCase(Exp):	am_.pickserv_->exportSet();		break;
	mCase(Man):	am_.pickserv_->managePickSets();	break;
	}
    break;
    mCase(Wvlt):
	switch ( at )
	{
	mCase(Imp):	am_.seisserv_->importWavelets();	break;
	mCase(Exp):	am_.seisserv_->exportWavelets();	break;
	default:	am_.seisserv_->manageWavelets();	break;
	}
    break;
    mCase(MDef):
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
    mCase(Vel):
	if ( at == uiODApplMgr::Imp)
	{
	    if ( !impvelfunc_ )
		impvelfunc_ = new Vel::uiImportVelFunc( par_ );

	    impvelfunc_->show();
	}
    break;
    mCase(Strat):
	StratTWin().popUp();
    break;
    mCase(PDF):
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
    mCase(Geom):
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
	    if ( !imp2dgeomdlg_ )
	    {
		imp2dgeomdlg_ = new uiImp2DGeom( par_ );
		imp2dgeomdlg_->setModal( false );
	    }

	    imp2dgeomdlg_->show();
	}
    break;
    mCase(PVDS):
	if ( at == uiODApplMgr::Imp )
	{
	    if ( !impcrossplotdlg_ )
		impcrossplotdlg_ = new uiImpPVDS( par_ );

	    impcrossplotdlg_->show();
	}
	else if ( at == uiODApplMgr::Man )
	{
	    delete mandpsdlg_;
	    mandpsdlg_ = new uiDataPointSetMan( par_ );
	    mandpsdlg_->show();
	}
    break;
    mCase(Body):
	if ( at == uiODApplMgr::Man )
	    am_.emserv_->manageBodies();
    break;
    mCase(Props):
	if ( at == uiODApplMgr::Man )
	{
	    delete manpropsdlg_;
	    manpropsdlg_ = new uiManPROPS( par_ );
	    manpropsdlg_->setModal( false );
	    manpropsdlg_->show();
	}
    break;
    mCase(Sess):
	if ( at == uiODApplMgr::Man )
	{
	    delete mansessiondlg_;
	    mansessiondlg_ = new uiSessionMan( par_ );
	    mansessiondlg_->show();
	}
    mCase(NLA):
	    pErrMsg("NLA event occurred");
    break;
    mCase(ColTab):
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
    mCase(RanL):
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
	    am_.seisserv_->managePreLoad();
	break;
	case uiODApplMgr::Hor:
	    am_.emserv_->managePreLoad();
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
    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt_.forread_ = true;
    ctio.ctxt_.setName( "Cross-plot Data" );
    uiIOObjSelDlg seldlg( par_, ctio );
    seldlg.setHelpKey( mODHelpKey(mOpenCossplotHelpID) );
    if ( !seldlg.go() || !seldlg.ioObj() ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );

    PosVecDataSet pvds;
    BufferString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    MouseCursorManager::restoreOverride();

    if ( !rv )
	{ uiMSG().error(mToUiStringTodo( errmsg ) ); return; }
    if ( pvds.data().isEmpty() )
    { uiMSG().error(uiDataPointSetMan::sSelDataSetEmpty()); return; }

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
#ifndef __win__
    options = alloptions;
#else
    options.add( alloptions.get( (int)ODInst::InformOnly) );
    options.add( alloptions.get( (int)ODInst::NoAuto) );
#endif

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
	am_.appl_.updateCaption();
}


#define mPreStackBatchdlg(dlgobj) \
	if ( !dlgobj ) \
	    dlgobj = new PreStack::uiBatchProcSetup( par_, is2d ); \
	dlgobj->show(); \

void uiODApplMgrDispatcher::processPreStack( bool is2d )
{
    if ( is2d )
    {
#ifdef __debug__
	mPreStackBatchdlg(batchprocps2ddlg_)
#else
	uiMSG().message( tr("Coming soon") );
	return;
#endif
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
{ am_.wellattrserv_->createLogCube( MultiID::udf() ); }

void uiODApplMgrDispatcher::process2D3D( int opt )
{
    if ( opt==0 )
	am_.emattrserv_->create2DGrid( nullptr );
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
		FilePath(GetExecPlfDir(), "od_BatchHosts").fullPath());
    const OS::CommandExecPars pars( OS::RunInBG );
    mc.execute(pars);
}
