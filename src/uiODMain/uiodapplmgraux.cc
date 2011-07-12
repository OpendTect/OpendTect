/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodapplmgraux.cc,v 1.31 2011-07-12 10:51:55 cvsbruno Exp $";

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"

#include "attribdescset.h"
#include "bidvsetarrayadapter.h"
#include "ctxtioobj.h"
#include "datapointset.h"
#include "datapackbase.h"
#include "emsurfacetr.h"
#include "filepath.h"
#include "ioobj.h"
#include "oddirs.h"
#include "odplatform.h"
#include "posvecdataset.h"
#include "separstr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "veldesc.h"
#include "vishorizondisplay.h"
#include "vishorizonsection.h"
#include "vissurvscene.h"

#include "ui2dgeomman.h"
#include "uimsg.h"
#include "uiconvpos.h"
#include "uidatapointset.h"
#include "uiveldesc.h"
#include "uifontsel.h"
#include "uipluginman.h"
#include "uishortcuts.h"
#include "uibatchprogs.h"
#include "uibatchlaunch.h"
#include "uistrattreewin.h"
#include "uiprestackimpmute.h"
#include "uiprestackexpmute.h"
#include "uibatchprestackproc.h"
#include "uiprestackanglemutecomputer.h"
#include "uivelocityfunctionimp.h"
#include "uivisdatapointsetdisplaymgr.h"
#include "uiprobdenfuncman.h"
#include "uiimpexppdf.h"
#include "uiimppvds.h"
#include "uiseisbayesclass.h"
#include "uisurvmap.h"

#include "uiattribpartserv.h"
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
		am_.emserv_->import3DHorizon( true );
	    else if ( opt == 1 )
		am_.emserv_->import3DHorizon( false );
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
		am_.emserv_->importFault(
				EMFault3DTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		am_.emserv_->importFault(
				EMFaultStickSetTranslatorGroup::keyword() );
	    else if ( opt == 2 )
		am_.emattrserv_->import2DFaultStickset(
				EMFaultStickSetTranslatorGroup::keyword() );
	    break;
	mCase(Exp):
	    if ( opt == 0 )
		am_.emserv_->exportFault(
				EMFault3DTranslatorGroup::keyword() );
	    else if ( opt == 1 )
		am_.emserv_->exportFault(
				EMFaultStickSetTranslatorGroup::keyword());
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

	break;
	mCase(Man):	am_.wellserv_->manageWells();	break;
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
	switch ( at )
	{
	default:	StratTWin().popUp();	break;
	}
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
    PosVecDataSet pvds;
    pvds.setEmpty();
    DataPointSet* newdps = new DataPointSet( pvds, false );
    DPM(DataPackMgr::PointID()).addAndObtain( newdps );
    uiDataPointSet* uidps =
	new uiDataPointSet( ODMainWin(), *newdps,
			    uiDataPointSet::Setup("CrossPlot from saved data"),
			    ODMainWin()->applMgr().visDPSDispMgr() );
    uidps->go();
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
void uiODApplMgrDispatcher::updateSoftware()
{
    FilePath fp( GetSoftwareDir(0) );

    BufferString pathonly = fp.pathOnly();
    BufferString reltypestr;

    fp.add( "relinfo" );
    fp.add( "readme.txt" );

    StreamData sd = StreamProvider( fp.fullPath() ).makeIStream();
    if ( !sd.usable() )
    { sd.close(); return; }

    while ( !sd.istrm->eof() )
    {
	char buf[1024];
	sd.istrm->getline( buf, 1024 );

	reltypestr = buf;
	break;
    }

    sd.close();

    SeparString ss( reltypestr, '-' );

    if ( ss.size() != 2 )
	return;

    const char* reltype = ss[1];

    FilePath instfp( GetSoftwareDir(0) );
    instfp.add( "bin" );
    instfp.add( OD::Platform::local().shortName() ).add( "od_installer" );

    BufferString cmd("@");
    cmd.add( instfp.fullPath() );
    cmd.add(" --instdir ").add("\"").add( pathonly ).add("\"");
    cmd.add( " --reltype ").add( "\"").add( reltype ).add("\"");

    StreamProvider strmprovinst( cmd );
    if ( !strmprovinst.executeCommand(false) )
	return;
}
void uiODApplMgrDispatcher::manageShortcuts()
{ uiShortcutsDlg dlg( par_, "ODScene" ); dlg.go(); }
void uiODApplMgrDispatcher::setFonts()
{ uiSetFonts dlg( par_, "Set font types" ); dlg.go(); }
void uiODApplMgrDispatcher::resortSEGY()
{ am_.seisserv_->resortSEGY(); }
