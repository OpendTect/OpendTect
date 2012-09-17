/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivisslicepos3d.cc,v 1.25 2012/03/07 08:22:57 cvsbruno Exp $";

#include "uivisslicepos3d.h"

#include "survinfo.h"
#include "uitoolbutton.h"
#include "uispinbox.h"
#include "visvolorthoslice.h"
#include "visplanedatadisplay.h"
#include "visvolumedisplay.h"
#include "vissurvscene.h"
#include "uivispartserv.h"

#define Plane  visSurvey::PlaneDataDisplay
#define Volume visSurvey::VolumeDisplay
#define Object visSurvey::SurveyObject


uiSlicePos3DDisp::uiSlicePos3DDisp( uiParent* p, uiVisPartServer* server )
    : uiSlicePos( p )
    , curpdd_(0)
    , curvol_(0) 
    , vispartserv_(server)
{
    const bool isobj = curpdd_ || curvol_;
    sliceposbox_->setSensitive( isobj );
    slicestepbox_->setSensitive( isobj );
    prevbut_->setSensitive( isobj );
    nextbut_->setSensitive( isobj );
}


void uiSlicePos3DDisp::setDisplay( int dispid )
{
    CallBack movecb( mCB(this,uiSlicePos3DDisp,updatePos) );
    CallBack manipcb( mCB(this,uiSlicePos3DDisp,updatePos) );
    visSurvey::SurveyObject* prevso = curpdd_ ? 
	(visSurvey::SurveyObject*)curpdd_ : (visSurvey::SurveyObject*)curvol_;
    if ( prevso )
    {
	prevso->getMovementNotifier()->remove( movecb );
	prevso->getManipulationNotifier()->remove( manipcb );
    }
    if ( curpdd_ ) { curpdd_->unRef(); curpdd_ = 0; }
    if ( curvol_ ) { curvol_->unRef(); curvol_ = 0; }

    mDynamicCastGet(Object*,so,vispartserv_->getObject(dispid));
    mDynamicCastGet(Plane*,pdd,so);
    mDynamicCastGet(Volume*,vol,so);
    const bool isvalidso = ( pdd && pdd->isSelected() )
			    || ( vol && vol->getSelectedSlice() );

    sliceposbox_->setSensitive( isvalidso );
    slicestepbox_->setSensitive( isvalidso );
    prevbut_->setSensitive( isvalidso );
    nextbut_->setSensitive( isvalidso );

    if ( !isvalidso ) return;

    curpdd_ = pdd; 
    curvol_ = vol;

    if ( curpdd_ ) curpdd_->ref();
    if ( curvol_ ) curvol_->ref();
    so->getMovementNotifier()->notify( movecb );
    so->getManipulationNotifier()->notify( manipcb );

    zfactor_ = so->getScene() ? so->getScene()->zDomainUserFactor() : 1;
    setBoxLabel( getOrientation() ); 
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


int uiSlicePos3DDisp::getDisplayID() const
{
    return curpdd_ ? curpdd_->id() : curvol_ ? curvol_->id() : -1; 
}


void uiSlicePos3DDisp::setBoxRanges()
{
    if ( !curpdd_ && !curvol_ ) return;

    const CubeSampling& survey = curpdd_ && curpdd_->getScene() ?
				    curpdd_->getScene()->getCubeSampling() 
				  : curvol_ ? curvol_->getCubeSampling( 0 ) 
				  : SI().sampling( true );
    laststeps_[2] = (int)survey.zrg.step;
    setBoxRg( getOrientation(), survey );
}


void uiSlicePos3DDisp::setPosBoxValue()
{
    if ( !curpdd_ && !curvol_ ) return;

    setPosBoxVal( getOrientation(), getSampling() );
}


void uiSlicePos3DDisp::setStepBoxValue()
{
    if ( !curpdd_ && !curvol_ ) return;

    const uiSlicePos::Orientation orientation = getOrientation();
    slicestepbox_->setValue( laststeps_[(int)orientation] );
    sliceStepChg( 0 );
}


void uiSlicePos3DDisp::slicePosChg( CallBacker* )
{
    if ( !curpdd_ && !curvol_ ) return;

    slicePosChanged( getOrientation(), getSampling() );
}


void uiSlicePos3DDisp::sliceStepChg( CallBacker* )
{
    if ( !curpdd_ && !curvol_ ) return;

    sliceStepChanged( getOrientation() );
}


uiSlicePos::Orientation uiSlicePos3DDisp::getOrientation() const 
{
    if ( curpdd_ )
	return (uiSlicePos::Orientation) curpdd_->getOrientation();
    else if ( curvol_ && curvol_->getSelectedSlice() )
    {
	const int dim = curvol_->getSelectedSlice()->getDim();
	if ( dim == Volume::cInLine() )
	    return uiSlicePos::Inline;
	else if ( dim == Volume::cCrossLine() )
	    return uiSlicePos::Crossline;
	else if ( dim == Volume::cTimeSlice() )
	    return uiSlicePos::Zslice;
    }
    return uiSlicePos::Inline;
}


CubeSampling uiSlicePos3DDisp::getSampling() const
{
    return curpdd_ ? curpdd_->getCubeSampling( true, true ) 
		   : curvol_->sliceSampling( curvol_->getSelectedSlice() );
}
