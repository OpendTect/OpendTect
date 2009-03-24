/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiodapplmgraux.cc,v 1.1 2009-03-24 15:51:31 cvsbert Exp $";

#include "uiodapplmgraux.h"
#include "uiodapplmgr.h"

#include "uiveldesc.h"
#include "uimsg.h"
#include "veldesc.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "survinfo.h"
#include "timedepthconv.h"


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
