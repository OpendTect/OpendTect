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
#include "oddirs.h"
#include "odinst.h"
#include "odplatform.h"
#include "odsession.h"
#include "posvecdataset.h"
#include "posvecdatasettr.h"
#include "separstr.h"
#include "odinst.h"
#include "string2.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "veldesc.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "vissurvscene.h"

#include "ui2dgeomman.h"
#include "uidatapointsetman.h"
#include "uimsg.h"
#include "uiconvpos.h"
#include "uidatapointset.h"
#include "uiveldesc.h"
#include "uifontsel.h"
#include "uipluginman.h"
#include "uishortcuts.h"
#include "uiselsimple.h"
#include "uibatchprogs.h"
#include "uibatchlaunch.h"
#include "uistrattreewin.h"
#include "uiprestackimpmute.h"
#include "uiprestackexpmute.h"
#include "uibatchprestackproc.h"
#include "uimanprops.h"
#include "uiprestackanglemutecomputer.h"
#include "uivelocityfunctionimp.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "uiprobdenfuncman.h"
#include "uiimpexppdf.h"
#include "uiimppvds.h"
#include "uiseisbayesclass.h"
#include "uisurvmap.h"
#include "uiseis2dto3d.h"
#include "uicreate2dgrid.h"

#include "uiattribpartserv.h"
#include "uicreatelogcubedlg.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uinlapartserv.h"
#include "uipickpartserv.h"
#include "uiseispartserv.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"


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
    uiParent* res = uiMainWin::activeWindow();
    if ( !res )
	res = par_;

    return res;
}


void uiODApplMgrDispatcher::survChg( bool before )
{
    if ( before && convposdlg_ )
	{ delete convposdlg_; convposdlg_ = 0; }

    if ( basemapdlg_ )
    {
	delete basemapdlg_;
	basemapdlg_ = 0;
	basemap_ = 0;
    }

    deepErase( uidpsset_ );
}


#define mCase(val) case uiODApplMgr::val

void uiODApplMgrDispatcher::doOperation( int iot, int iat, int opt )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    const uiODApplMgr::ActType at = (uiODApplMgr::ActType)iat;

    switch ( ot )
    {
    mCase(Seis):
	switch ( at )
	{
	mCase(Imp):	am_.seisserv_->importSeis( opt );	break;
	mCase(Exp):	am_.seisserv_->exportSeis( opt );	break;
	mCase(Man):	am_.seisserv_->manageSeismics(opt==1);	break;
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
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->export3DHorizon();
	    else if ( opt == 1 )
		am_.emserv_->export2DHorizon();
	    break;
	mCase(Man):
	    if ( opt == 0 )
		am_.emserv_->manageSurfaces(
				 EMAnyHorizonTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		am_.emserv_->manageSurfaces(
				EMHorizon2DTranslatorGroup::keyword());
	    else if ( opt == 2 )
		am_.emserv_->manageSurfaces(
				EMHorizon3DTranslatorGroup::keyword());
	    break;
	}
    break;
    mCase(Flt):
	switch( at )
	{
	mCase(Imp):
	    if ( opt == 0 )
		am_.emserv_->importFault();
	    else if ( opt == 1 )
		am_.emserv_->importFaultStickSet();
	    else if ( opt == 2 )
		am_.emattrserv_->import2DFaultStickset(
				EMFaultStickSetTranslatorGroup::keyword() );
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->exportFault();
	    else if ( opt == 1 )
		am_.emserv_->exportFaultStickSet();
	    break;
	mCase(Man):
	    if ( opt == 0 ) opt = SI().has3D() ? 2 : 1;
	    if ( opt == 1 )
		am_.emserv_->manageSurfaces(
				EMFaultStickSetTranslatorGroup::keyword() );
	    else if ( opt == 2 )
		am_.emserv_->manageSurfaces(
				EMFault3DTranslatorGroup::keyword() );
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
	    else if ( opt == 3 )
		am_.wellattrserv_->importSEGYVSP();
	    else if ( opt == 4 )
		am_.wellserv_->createSimpleWells();
	    else if ( opt == 5 )
		am_.wellserv_->bulkImportTrack();
	    else if ( opt == 6 )
		am_.wellserv_->bulkImportLogs();
	    else if ( opt == 7 )
		am_.wellserv_->bulkImportMarkers();

	break;
	mCase(Man):	am_.wellserv_->manageWells();	break;
	default:					break;
	}
    break;
    mCase(Attr):
	if ( at == uiODApplMgr::Man )
	    am_.attrserv_->manageAttribSets();
    break;
    mCase(Pick):
	switch ( at )
	{
	mCase(Imp):	am_.pickserv_->impexpSet( true );	break;
	mCase(Exp):	am_.pickserv_->impexpSet( false );	break;
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
	    PreStack:: uiImportMute dlgimp( par_ );
	    dlgimp.go();
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
	    Vel::uiImportVelFunc dlgvimp( par_ );
	    dlgvimp.go();
	}
    break;
    mCase(Strat):
	StratTWin().popUp();
    break;
    mCase(PDF):
        if ( at == uiODApplMgr::Imp )
	{
	    uiImpRokDocPDF dlg( par_ );
	    dlg.go();
	}
	else if ( at == uiODApplMgr::Exp )
	{
	    uiExpRokDocPDF dlg( par_ );
	    dlg.go();
	}
	else if ( at == uiODApplMgr::Man )
	{
	    uiProbDenFuncMan dlg( par_ );
	    dlg.go();
	}
    break;
    mCase(Geom):
	if ( at == uiODApplMgr::Man )
	{
	    ui2DGeomManageDlg dlg( par_ );
	    dlg.go();
	}
    break;
    mCase(PVDS):
        if ( at == uiODApplMgr::Imp )
	{
	    uiImpPVDS dlg( par_ );
	    dlg.go();
	}
    	else if ( at == uiODApplMgr::Man )
	{
	    uiDataPointSetMan mandlg( par_ );
	    mandlg.go();
	}
    break;
    mCase(Body):
  	if ( at == uiODApplMgr::Man )
	    am_.emserv_->manageSurfaces( EMBodyTranslatorGroup::sKeyword() );
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
	    uiSessionMan mandlg( par_ );
    	    mandlg.go();
	}
    mCase(NLA):
	    pErrMsg("NLA event occurred");
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


void uiODApplMgrDispatcher::showBaseMap()
{
    if ( !basemapdlg_ )
    {
	const int sceneid = am_.sceneMgr().askSelectScene();
	mDynamicCastGet(visSurvey::Scene*,scene,
			am_.visserv_->getObject(sceneid) );
	if ( !scene ) return;

	basemapdlg_ = new uiDialog( par_, uiDialog::Setup("Base Map","","") );
	basemapdlg_->setModal( false );
	basemapdlg_->setCtrlStyle( uiDialog::LeaveOnly );
	basemap_ = new uiSurveyMap( basemapdlg_ );
	basemap_->setPrefHeight( 250 );
	basemap_->setPrefWidth( 250 );
	basemap_->drawMap( &SI() );
	scene->setBaseMap( basemap_ );
    }

    basemapdlg_->show();
    basemapdlg_->raise();
}


int uiODApplMgrDispatcher::createMapDataPack( const DataPointSet& data,
					      int colnr )
{
    uiVisPartServer* uivispartserv = am_.visServer();
    if ( !uivispartserv ) return -1;

    const int selobjid = uivispartserv->getSelObjectId();
    const visBase::DataObject* dataobj = uivispartserv->getObject( selobjid );
    mDynamicCastGet(const visSurvey::EMObjectDisplay*,emobj,dataobj)
    mDynamicCastGet(const visSurvey::HorizonDisplay*,hordisp,emobj)
    if ( !hordisp ) return -1;

    const visBase::HorizonSection* horsec = hordisp->getSection( 0 );
    if ( !horsec ) return -1;

    const int attrnr = uivispartserv->getSelAttribNr();
    const BinIDValueSet* cache = horsec->getCache( attrnr );
    if ( !cache ) return -1;

    BinID step( SI().inlStep(), SI().crlStep() );
    BIDValSetArrAdapter* bvsarr = new BIDValSetArrAdapter(*cache, colnr, step);

    MapDataPack* newpack = new MapDataPack( "Attribute", data.name(), bvsarr );
    StepInterval<int> tempinlrg = bvsarr->hrg_.inlRange();
    StepInterval<int> tempcrlrg = bvsarr->hrg_.crlRange();
    StepInterval<double> inlrg( (double)tempinlrg.start, (double)tempinlrg.stop,
	    			(double)tempinlrg.step );
    StepInterval<double> crlrg( (double)tempcrlrg.start, (double)tempcrlrg.stop,
	    			(double)tempcrlrg.step );
    BufferStringSet dimnames;
    dimnames.add("X").add("Y").add("In-Line").add("Cross-line");
    newpack->setProps( inlrg, crlrg, true, &dimnames );
    DataPackMgr& dpman = DPM( DataPackMgr::FlatID() );
    dpman.add( newpack );
    return newpack->id();
}


void uiODApplMgrDispatcher::openXPlot()
{
    CtxtIOObj ctio( PosVecDataSetTranslatorGroup::ioContext() );
    ctio.ctxt.forread = true;
    uiIOObjSelDlg seldlg( par_, ctio );
    if ( !seldlg.go() || !seldlg.ioObj() ) return;

    MouseCursorManager::setOverride( MouseCursor::Wait );

    PosVecDataSet pvds;
    BufferString errmsg;
    bool rv = pvds.getFrom(seldlg.ioObj()->fullUserExpr(true),errmsg);
    MouseCursorManager::restoreOverride();

    if ( !rv )
    { uiMSG().error( errmsg ); return; }
    if ( pvds.data().isEmpty() )
    { uiMSG().error("Selected data set is empty"); return; }

    DataPointSet* newdps = new DataPointSet( pvds, false );
    newdps->setName( seldlg.ioObj()->name() );
    DPM(DataPackMgr::PointID()).addAndObtain( newdps );
    uiDataPointSet* uidps =
	new uiDataPointSet( ODMainWin(), *newdps,
			    uiDataPointSet::Setup("CrossPlot from saved data"),
			    ODMainWin()->applMgr().visDPSDispMgr() );
    uidps->go();
    uidpsset_ += uidps;
}


void uiODApplMgrDispatcher::startInstMgr()
{
    uiMSG().message( "If you make changes to the application,"
	    "\nplease restart OpendTect for the changes to take effect." );
    ODInst::startInstManagement();
}


void uiODApplMgrDispatcher::setAutoUpdatePol()
{
    const ODInst::AutoInstType curait = ODInst::getAutoInstType();
    uiGetChoice dlg( par_, ODInst::autoInstTypeUserMsgs(),
	    		"Select policy for auto-update", true, "0.4.5" );
    dlg.setDefaultChoice( (int)curait );
    if ( !dlg.go() )
	return;
    ODInst::AutoInstType newait = (ODInst::AutoInstType)dlg.choice();
    if ( newait != curait )
	ODInst::setAutoInstType( newait );
    if ( newait == ODInst::InformOnly )
	am_.appl_.updateCaption();
}


void uiODApplMgrDispatcher::processPreStack()
{ PreStack::uiBatchProcSetup dlg( par_, false ); dlg.go(); }
void uiODApplMgrDispatcher::genAngleMuteFunction()
{ PreStack::uiAngleMuteComputer dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::bayesClass( bool is2d )
{ new uiSeisBayesClass( ODMainWin(), is2d ); }
void uiODApplMgrDispatcher::reStartProc()
{ uiRestartBatchDialog dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::batchProgs()
{ uiBatchProgLaunch dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::pluginMan()
{ uiPluginMan dlg( par_ ); dlg.go(); }
void uiODApplMgrDispatcher::manageShortcuts()
{ uiShortcutsDlg dlg( par_, "ODScene" ); dlg.go(); }
void uiODApplMgrDispatcher::setFonts()
{ uiSetFonts dlg( par_, "Set font types" ); dlg.go(); }
void uiODApplMgrDispatcher::resortSEGY()
{ am_.seisserv_->resortSEGY(); }
void uiODApplMgrDispatcher::createCubeFromWells()
{ uiCreateLogCubeDlg dlg( par_, 0 ); dlg.go(); }

void uiODApplMgrDispatcher::process2D3D( bool to2d )
{
    if ( to2d )
    { uiCreate2DGrid dlg( par_, 0 ); dlg.go(); }
    else
    { uiSeis2DTo3D dlg( par_ ); dlg.go(); }
}

