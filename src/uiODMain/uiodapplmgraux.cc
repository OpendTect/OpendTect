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
#include "ioobj.h"
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
#include "uicolseqimport.h"
#include "uicolseqman.h"
#include "uiconvpos.h"
#include "uicreate2dgrid.h"
#include "uicreatelogcubedlg.h"
#include "uicrssystem.h"
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
#include "uiselsimple.h"
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
    , convgeoposdlg_(0)
    , mandpsdlg_(0)
    , man2dgeomdlg_(0)
    , manpdfdlg_(0)
    , manrldlg_(0)
    , mansessiondlg_(0)
    , impcrossplotdlg_(0)
    , impmutedlg_(0)
    , expmutedlg_(0)
    , imppdfdlg_(0)
    , exppdfdlg_(0)
    , impvelfunc_(0)
    , exp2dgeomdlg_(0)
    , imp2dgeomdlg_(0)
	, impcolseqdlg_(0)
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
    deleteAndZeroPtr( convposdlg_ );
    deleteAndZeroPtr( convgeoposdlg_ );
    deleteAndZeroPtr( mandpsdlg_ );
    deleteAndZeroPtr( man2dgeomdlg_ );
    deleteAndZeroPtr( manpdfdlg_ );
    deleteAndZeroPtr( mansessiondlg_ );
    deleteAndZeroPtr( impcrossplotdlg_ );
    deleteAndZeroPtr( impmutedlg_ );
    deleteAndZeroPtr( expmutedlg_ );
    deleteAndZeroPtr( imppdfdlg_ );
    deleteAndZeroPtr( exppdfdlg_ );
    deleteAndZeroPtr( impvelfunc_ );
    deleteAndZeroPtr( exp2dgeomdlg_ );
    deleteAndZeroPtr( imp2dgeomdlg_ );
	deleteAndZeroPtr( impcolseqdlg_ );
    deleteAndZeroPtr( batchprocps2ddlg_ );
    deleteAndZeroPtr( batchprocps3ddlg_ );
}


#define mCase(val) case uiODApplMgr::val
#define mHandleUnknownCase() default: pErrMsg("Add case"); break;

void uiODApplMgrDispatcher::doOperation( int iot, int iat, int opt )
{
    const uiODApplMgr::ActType at = (uiODApplMgr::ActType)iat;
    if ( at == uiODApplMgr::PL )
	{ manPreLoad( iot ); return; }

    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    switch ( ot )
    {
    mCase(Seis):
	switch ( at )
	{
	mCase(Imp):	am_.seisserv_->importSeis( opt );	break;
	mCase(Exp):	am_.seisserv_->exportSeis( opt );	break;
	mCase(Man):	am_.seisserv_->manageSeismics( opt );	break;
	mHandleUnknownCase()
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
	mHandleUnknownCase()
	}
    break;
    mCase(Flt):
	switch( at )
	{
	mCase(Imp):
	    am_.emserv_->importFault( opt!=0 );
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
	    else if ( opt == 3 )
		am_.emserv_->manageFaultSets();
	break;
	mHandleUnknownCase()
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
	mCase(Man): break;
	mHandleUnknownCase()
	}
    break;
    mCase(FltSet):
	switch( at )
	{
	mCase( Imp ) :
	    am_.emserv_->importFaultSet();
	break;
	mCase(Exp):
	    am_.emserv_->exportFaultSet();
	break;
	mCase(Man):
	    am_.emserv_->manageFaultSets();
	break;
	mHandleUnknownCase()
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
	mCase(Exp): break;
	mCase(Man):
	    am_.wellserv_->manageWells();
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Attr):
	switch( at )
	{
	mCase(Imp):
	    if ( opt == 0 )
		am_.attrserv_->importAttrSetFromFile();
	    else if ( opt == 1 )
		am_.attrserv_->importAttrSetFromOtherSurvey();
	break;
	mCase(Exp): break;
	mCase(Man):
	    am_.attrserv_->manageAttribSets(opt==1);
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Pick): // [[fallthrough]]
    mCase(Poly):
	switch ( at )
	{
	mCase(Imp):	am_.pickserv_->importSet();		break;
	mCase(Exp):	am_.pickserv_->exportSet();		break;
	mCase(Man):	am_.pickserv_->managePickSets();	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Wvlt):
	switch ( at )
	{
	mCase(Imp):	am_.seisserv_->importWavelets();	break;
	mCase(Exp):	am_.seisserv_->exportWavelets();	break;
	mCase(Man):	am_.seisserv_->manageWavelets();	break;
	mHandleUnknownCase()
	}
    break;
    mCase(MDef):
	switch ( at )
	{
	mCase(Imp):
	{
	    if ( !impmutedlg_ )
		impmutedlg_ = new PreStack::uiImportMute( par_ );

	    impmutedlg_->show();
	}
	break;
	mCase(Exp):
	{
	    if ( !expmutedlg_ )
		expmutedlg_ = new PreStack::uiExportMute( par_ );

	    expmutedlg_->show();
	}
	break;
	mCase(Man): break;
	mHandleUnknownCase()
	}
    break;
    mCase(Vel):
	switch ( at )
	{
	mCase(Imp):
	{
	    if ( !impvelfunc_ )
		impvelfunc_ = new Vel::uiImportVelFunc( par_ );

	    impvelfunc_->show();
	}
	break;
	mCase(Exp): break;
	mCase(Man): break;
	mHandleUnknownCase()
	}
    break;
    mCase(Strat):
	StratTWin().popUp();
    break;
    mCase(PDF):
	switch ( at )
	{
	mCase(Imp):
	{
	    if ( !imppdfdlg_ )
		imppdfdlg_ = new uiImpRokDocPDF( par_ );

	    imppdfdlg_->show();
	}
	break;
	mCase(Exp):
	{
	    if ( !exppdfdlg_ )
		exppdfdlg_ = new uiExpRokDocPDF( par_ );

	    exppdfdlg_->show();
	}
	break;
	mCase(Man):
	{
	    delete manpdfdlg_;
	    manpdfdlg_ = new uiProbDenFuncMan( par_ );
	    manpdfdlg_->go();
	}
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Geom2D):
	switch ( at )
	{
	mCase(Imp):
	{
	    if ( !imp2dgeomdlg_ )
	    {
		imp2dgeomdlg_ = new uiImp2DGeom( par_ );
		imp2dgeomdlg_->setModal( false );
	    }

	    imp2dgeomdlg_->show();
	}
	break;
	mCase(Exp):
	{
	    if ( !exp2dgeomdlg_ )
		exp2dgeomdlg_ = new uiExp2DGeom( par_ );

	    exp2dgeomdlg_->show();
	}
	break;
	mCase(Man):
	{
	    delete man2dgeomdlg_;
	    man2dgeomdlg_ = new ui2DGeomManageDlg( par_ );
	    man2dgeomdlg_->go();
	}
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(XPlot):
	switch ( at )
	{
	mCase(Imp):
	{
	    if ( !impcrossplotdlg_ )
		impcrossplotdlg_ = new uiImpPVDS( par_ );

	    impcrossplotdlg_->show();
	}
	break;
	mCase(Exp): break;
	mCase(Man):
	{
	    delete mandpsdlg_;
	    mandpsdlg_ = new uiDataPointSetMan( par_ );
	    mandpsdlg_->go();
	}
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Body):
	switch ( at )
	{
	mCase(Imp): break;
	mCase(Exp): break;
	mCase(Man):
	    am_.emserv_->manageBodies();
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Props):
	switch ( at )
	{
	mCase(Imp): break;
	mCase(Exp): break;
	mCase(Man):
	{
	    uiManPROPS mandlg( par_ );
	    mandlg.go();
	}
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(Sess):
	switch ( at )
	{
	mCase(Imp): break;
	mCase(Exp): break;
	mCase(Man):
	{
	    delete mansessiondlg_;
	    mansessiondlg_ = new uiSessionMan( par_ );
	    mansessiondlg_->show();
	}
	break;
	mHandleUnknownCase()
	}
    mCase(NLA):
	    pErrMsg("NLA event occurred");
    break;
    mCase(ColTab):
	switch ( at )
	{
	mCase(Imp):
	{
		delete impcolseqdlg_;
		impcolseqdlg_ = new uiColSeqImport(par_);
		impcolseqdlg_->show();
	}
	break;
	mCase(Exp): break;
	mCase(Man):
	{
	    uiColSeqMan* dlg = new uiColSeqMan( par_ );
	    dlg->go();
	}
	break;
	mHandleUnknownCase()
	}
    break;
    mCase(RanL):
	switch ( at )
	{
	mCase(Imp): break;
	mCase(Exp): break;
	mCase(Man):
	{
	    delete manrldlg_;
	    manrldlg_ = new uiRandomLineMan( par_ );
	    manrldlg_->show();
	}
	break;
	mHandleUnknownCase()
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
	break;
	default:
	    pErrMsg("Preload unknown type requested");
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


void uiODApplMgrDispatcher::crsPosConversion()
{
    ConstRefMan<Coords::CoordSystem> crs = SI().getCoordSystem();
    TrcKeyZSampling survtkzs;
    SI().getSampling( survtkzs, OD::UsrWork );
    const Coord centerpos = survtkzs.hsamp_.center().getCoord();
    if ( !convgeoposdlg_ )
	convgeoposdlg_ = new Coords::uiConvertGeographicPos( par_, crs,
							     centerpos );

    convgeoposdlg_->show();
}


void uiODApplMgrDispatcher::posDlgClose( CallBacker* )
{
    convposdlg_ = 0;
}


#define mUiMsg() gUiMsg()

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
    uiString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    MouseCursorManager::restoreOverride();

    if ( !rv )
	{ mUiMsg().error( errmsg ); return; }
    if ( pvds.data().isEmpty() )
    { mUiMsg().error(uiDataPointSetMan::sSelDataSetEmpty()); return; }

    RefMan<DataPointSet> newdps = new DataPointSet( pvds, false );
    newdps->setName( seldlg.ioObj()->name() );
    DPM(DataPackMgr::PointID()).add( newdps );
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
    mUiMsg().message( msg );
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

    uiStringSet uioptions;
    for ( int idx=0; idx<options.size(); idx++ )
	uioptions.add( toUiString(options.get(idx)) );
    uiGetChoice dlg( par_, uioptions,
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
	mUiMsg().error(
		mINTERNAL("2D PS Batch Processing: not release-tested yet") );
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
void uiODApplMgrDispatcher::createCubeFromWells()
{ am_.wellattrserv_->createLogCube( DBKey::getInvalid() ); }

void uiODApplMgrDispatcher::process2D3D( int opt )
{
    if ( opt==0 )
    { uiCreate2DGrid dlg( par_, 0 ); dlg.go(); }
    else if ( opt==1 )
    { uiSeis2DFrom3D dlg( par_ ); dlg.go(); }
    else if ( opt==2 )
    {
	uiString titletext = tr("Create 3D cube from 2D DataSet");
	uiSeis2DTo3D dlg( par_, titletext );
	dlg.go();
    }
}


void uiODApplMgrDispatcher::setupBatchHosts()
{ uiBatchHostsDlg dlg( par_ ); dlg.go(); }
