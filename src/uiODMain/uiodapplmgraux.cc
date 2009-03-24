/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodapplmgraux.cc,v 1.2 2009-03-24 16:28:03 cvsbert Exp $";

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "veldesc.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "survinfo.h"
#include "timedepthconv.h"
#include "emsurfacetr.h"

#include "uiveldesc.h"
#include "uimsg.h"
#include "uistrattreewin.h"
#include "uiprestackimpmute.h"
#include "uiprestackexpmute.h"
#include "uibatchprestackproc.h"
#include "uivelocityfunctionimp.h"

#include "uipickpartserv.h"
#include "uivispartserv.h"
#include "uimpepartserv.h"
#include "uiattribpartserv.h"
#include "uiemattribpartserv.h"
#include "uiempartserv.h"
#include "uinlapartserv.h"
#include "uiseispartserv.h"
#include "uitaskrunner.h"
#include "uiwellpartserv.h"
#include "uiwellattribpartserv.h"


bool uiODApplService::eventOccurred( const uiApplPartServer* ps, int evid )
{
    return applman_.handleEvent( ps, evid );
}


void* uiODApplService::getObject( const uiApplPartServer* ps, int evid )
{
    return applman_.deliverObject( ps, evid );
}


uiODApplMgrVelSel::uiODApplMgrVelSel( uiParent* p )
    : uiDialog(p,Setup("Velocity model",
		"Select velocity model to base scene on","0.4.7"))
    , ctio_(*new CtxtIOObj(uiVelSel::ioContext()))
    , trans_(0)
{
    ctio_.ctxt.forread = true;
    uiSeisSel::Setup su( false, false ); su.seltxt("Velocity model");
    velsel_ = new uiVelSel( this, ctio_, su );
}

uiODApplMgrVelSel::~uiODApplMgrVelSel()
{
    delete ctio_.ioobj; delete &ctio_;
    if ( trans_ ) trans_->unRef();
}

#define mErrRet(s) { uiMSG().error(s); return false; }
bool uiODApplMgrVelSel::acceptOK( CallBacker* )
{
    if ( !velsel_->commitInput() )
	mErrRet("Please select a velocity model")
    VelocityDesc desc;
    if ( !desc.usePar( ctio_.ioobj->pars() ) )
	mErrRet("Cannot read velocity information for selected model")

    if ( SI().zIsTime() ) 
	// TODO: Should really depend on z-domain of model, not the survey.
    {
	if ( desc.type_ != VelocityDesc::Interval &&
	     desc.type_ != VelocityDesc::RMS )
	    mErrRet("Only RMS and Interval allowed for time based models")

	trans_ = new Time2DepthStretcher();
	zscale_ = SurveyInfo::defaultXYtoZScale( SurveyInfo::Meter,
						 SI().xyUnit() );
    }
    else
    {
	if ( desc.type_ != VelocityDesc::Interval )
	    mErrRet("Only Interval velocity allowed for time based models")

	trans_ = new Depth2TimeStretcher();
	zscale_ = SurveyInfo::defaultXYtoZScale( SurveyInfo::Second,
						 SI().xyUnit() );
    }

    if ( !trans_->setVelData( ctio_.ioobj->key() ) || !trans_->isOK() )
	mErrRet("Internal: Could not initialize transform")

    return true;
}


ZAxisTransform* uiODApplMgrVelSel::transform()
{
    return trans_;
}


#define mCase(val) case uiODApplMgr::val

void uiODApplMgrBasicDispatcher::doOperation( int iot, int iat, int opt )
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
	mCase(Man):	am_.seisserv_->manageSeismics();	break;
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
    }
}


void uiODApplMgrBasicDispatcher::manPreLoad( int iot )
{
    const uiODApplMgr::ObjType ot = (uiODApplMgr::ObjType)iot;
    switch ( ot )
    {
	case uiODApplMgr::Seis:
	    am_.seisserv_->managePreLoad();
	break;
	default:
	break;
    }
}


void uiODApplMgrBasicDispatcher::processPreStack()
{
    PreStack::uiBatchProcSetup dlg( par_, false );
    dlg.go();
}
