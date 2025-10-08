/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewslicepos.h"

#include "flatview.h"
#include "survinfo.h"

#include "uispinbox.h"


uiSlicePos2DView::uiSlicePos2DView( uiParent* p, const ZDomain::Info& zinfo,
				    const ZDomain::Info* dispzdinfo )
    : uiSlicePos( p )
{
    dispzdominfo_ = zdominfo_ = &zinfo;
    if ( dispzdinfo )
	dispzdominfo_ = dispzdinfo;
    else if ( zinfo.isDepth() )
	dispzdominfo_ = &ZDomain::DefaultDepth();

    zfactor_ = FlatView::Viewer::userFactor( *zdominfo_, dispzdominfo_ );
}


uiSlicePos2DView::~uiSlicePos2DView()
{}


void uiSlicePos2DView::setIsPreloaded( bool yn )
{
    ispreloaded_ = yn;
}


static OD::SliceType getSliceType( const TrcKeyZSampling& cs )
{
    if ( cs.defaultDir() == TrcKeyZSampling::Crl )
	return OD::SliceType::Crossline;
    if ( cs.defaultDir() == TrcKeyZSampling::Z )
	return OD::SliceType::Z;

    return OD::SliceType::Inline;
}


void uiSlicePos2DView::setTrcKeyZSampling( const TrcKeyZSampling& cs )
{
    curorientation_ = getSliceType( cs );
    curcs_ = cs;

    setBoxLabel( curorientation_ );
    setBoxRanges();
    setPosBoxValue();
    setStepBoxValue();
}


void uiSlicePos2DView::setLimitSampling( const TrcKeyZSampling& cs )
{
    limitscs_ = cs;
    setTrcKeyZSampling( curcs_ );
}


void uiSlicePos2DView::setBoxRanges()
{
    setBoxRg( curorientation_, limitscs_, limitscs_ );
}


void uiSlicePos2DView::setPosBoxValue()
{
    setPosBoxVal( curorientation_, curcs_ );
}


void uiSlicePos2DView::setStepBoxValue()
{
    slicestepbox_->setValue( laststeps_[(int)curorientation_] );
    sliceStepChg( 0 );
}


void uiSlicePos2DView::slicePosChg( CallBacker* )
{
    TrcKeyZSampling oldcs = curcs_;
    slicePosChanged( curorientation_, oldcs );
}


void uiSlicePos2DView::sliceStepChg( CallBacker* )
{
    sliceStepChanged( curorientation_ );
}


void uiSlicePos2DView::sliderPosChanged()
{
    NotifyStopper ns( sliceposbox_->valueChanging, this );
    if ( ispreloaded_ && getOrientation()!=OD::SliceType::Z )
	ns.enableNotification();

    uiSlicePos::sliderPosChanged();
}
