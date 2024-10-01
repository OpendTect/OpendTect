/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiaxisdata.h"
#include "dataclipper.h"
#include "envvars.h"
#include "axislayout.h"
#include "settings.h"

float uiAxisData::AutoScalePars::defclipratio_ = -1;

uiAxisData::uiAxisData( uiRect::Side s )
    : axis_(0)
    , defaxsu_(s)
    , rg_(0,1)
    , isreset_(false)
{
    stop();
}


uiAxisData::~uiAxisData()
{
    delete axis_;
}


void uiAxisData::stop()
{
    if ( isreset_ ) return;

    isreset_ = true;
    needautoscale_ = false;
    if ( !axis_ ) return;

    defaxsu_ = axis_->setup();
    delete axis_; axis_ = 0;
}


void uiAxisData::renewAxis( const uiString& newcaption, uiGraphicsScene* scene,
			    int width, int height,
			    const Interval<float>* newrg )
{
    stop();
    defaxsu_.width_ = width;
    defaxsu_.height_ = height;
    delete axis_;
    axis_ = new uiAxisHandler( scene, defaxsu_ );
    axis_->setCaption( newcaption );
    needautoscale_ = true;
    isreset_ = false;
    if ( newrg )
	setRange( *newrg );
}


void uiAxisData::newDevSize()
{
    if ( axis_ ) axis_->newDevSize();
}


void uiAxisData::handleAutoScale( const Stats::RunCalc<float>& rc )
{
    if ( !axis_ || !needautoscale_ || !autoscalepars_.doautoscale_ )
	return;

    rg_ = Interval<float>( rc.min(), rc.max() );
    if ( !mIsZero(autoscalepars_.clipratio_,1e-5) )
    {
	rg_.start_ = rc.clipVal( autoscalepars_.clipratio_, false );
	rg_.stop_ = rc.clipVal( autoscalepars_.clipratio_, true );
    }

    AxisLayout<float> al( rg_ );
    axis_->setRange( al.getSampling() );
    needautoscale_ = false;
}


void uiAxisData::handleAutoScale( const DataClipper& dtclip )
{
    if ( !axis_ || !needautoscale_ || !autoscalepars_.doautoscale_ )
	return;

    dtclip.getRange( autoscalepars_.clipratio_, rg_ );
    AxisLayout<float> al( rg_ );
    axis_->setRange( al.getSampling() );
    needautoscale_ = false;
}


uiAxisData::AutoScalePars::AutoScalePars()
    : doautoscale_(true)
{
    if ( defclipratio_ == -1 )
    {
	const BufferString res =
			    Settings::common().find( "AxisData.Clip Ratio" );
	const float val = (float) ( !res.isEmpty() ? res.toFloat()
			: GetEnvVarDVal( "OD_DEFAULT_AXIS_CLIPRATIO", 0 ) );
	defclipratio_ = val < 0 || val >= 1 ? 0 : val;
    }
    clipratio_ = defclipratio_;
}
