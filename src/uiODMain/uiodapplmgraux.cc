/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uicreate2dgrid.h"
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
    : am_(a), par_(p)
    , convposdlg_(0)
    , mandpsdlg_(0)
    , man2dgeomdlg_(0)
    , manpdfdlg_(0)
    , manrldlg_(0)
    , mansessiondlg_(0)
    , impcrossplotdlg_(0)
    , impmutedlg_(0)
    , imppdfdlg_(0)
    , exppdfdlg_(0)
    , impvelfunc_(0)
    , exp2dgeomdlg_(0)
    , batchprocps2ddlg_(0)
    , batchprocps3ddlg_(0)
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
	delete convposdlg_; convposdlg_ = 0;
	delete mandpsdlg_; mandpsdlg_ = 0;
	delete man2dgeomdlg_; man2dgeomdlg_ = 0;
	delete manpdfdlg_; manpdfdlg_ = 0;
	delete mansessiondlg_; mansessiondlg_ = 0;
	delete impcrossplotdlg_; impcrossplotdlg_ = 0;
	delete impmutedlg_; impmutedlg_ = 0;
	delete imppdfdlg_; imppdfdlg_ = 0;
	delete exppdfdlg_; exppdfdlg_ = 0;
	delete impvelfunc_; impvelfunc_ = 0;
	delete exp2dgeomdlg_; exp2dgeomdlg_ = 0;
	delete batchprocps2ddlg_; batchprocps2ddlg_ = 0;
	delete batchprocps3ddlg_; batchprocps3ddlg_ = 0;
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
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->export3DHorizon();
	    else if ( opt == 1 )
		am_.emserv_->export2DHorizon();
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
	    am_.emserv_->exportFault();
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
	    break;
	    mCase(Exp):
		am_.emserv_->exportFaultStickSet();
	    break;
	    default:
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

	break;
	mCase(Man):	am_.wellserv_->manageWells();	break;
	default:					break;
	}
    break;
    mCase(Attr):
	switch( at )
	{
	mCase(Man):	am_.attrserv_->manageAttribSets();  break;
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
		impmutedlg_ = new PreStack::uiImportMute( par_ );

	    impmutedlg_->show();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    PreStack::uiExportMute dlgexp( par_ );
	    dlgexp.go();
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
	    mandpsdlg_->go();
	}
    break;
    mCase(Body):
	if ( at == uiODApplMgr::Man )
	    am_.emserv_->manageBodies();
    break;
    mCase(Props):
	if ( at == uiODApplMgr::Man )
	{
	    uiManPROPS mandlg( par_ );
	    mandlg.go();
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
    convposdlg_ = 0;
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

    DataPointSet* newdps = new DataPointSet( pvds, false );
    newdps->setName( seldlg.ioObj()->name() );
    DPM(DataPackMgr::PointID()).addAndObtain( newdps );
    uiDataPointSet* uidps =
	new uiDataPointSet( ODMainWin(), *newdps,
	uiDataPointSet::Setup(tr("CrossPlot from saved data")),
	ODMainWin()->applMgr().visDPSDispMgr() );
    uidps->go();
    uidpsset_ += uidps;
}


void uiODApplMgrDispatcher::startInstMgr()
{
#ifndef __win__
    uiString msg = tr("If you make changes to the application,\nplease "
		      "restart OpendTect for the changes to take effect.");
#else
    uiString msg = tr("Please close OpendTect application and all other "
		      "OpendTect processes before proceeding for"
		      " installation/update");
#endif
    uiMSG().message( msg );
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
    { uiCreate2DGrid dlg( par_, 0 ); dlg.go(); }
    else if ( opt==1 )
    { uiSeis2DFrom3D dlg( par_ ); dlg.go(); }
    else if ( opt==2 )
    { uiSeis2DTo3D dlg( par_ ); dlg.go(); }
    else if ( opt==3 )
    {
	uiString str = uiStrings::phrCreate(tr("3D cube from 2D DataSet"));
	uiSeis2DTo3DInterPol dlg( par_, str ); dlg.go();
    }
}


void uiODApplMgrDispatcher::setupBatchHosts()
{ uiBatchHostsDlg dlg( par_ ); dlg.go(); }
