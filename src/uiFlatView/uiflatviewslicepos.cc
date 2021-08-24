/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		April 2009
________________________________________________________________________

-*/

#include "uiflatviewslicepos.h"

#include "survinfo.h"
#include "uispinbox.h"


uiSlicePos2DView::uiSlicePos2DView( uiParent* p, const ZDomain::Info& zinfo )
    : uiSlicePos( p )
    , zdomaininfo_(zinfo)
{
    zfactor_ = zinfo.userFactor();
}


static OD::SliceType getSliceType( const TrcKeyZSampling& cs )
{
    if ( cs.defaultDir() == TrcKeyZSampling::Crl )
	return OD::CrosslineSlice;
    if ( cs.defaultDir() == TrcKeyZSampling::Z )
	return OD::ZSlice;

    return OD::InlineSlice;
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
