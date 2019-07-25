/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uihistogramsel.h"

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uistrings.h"

#include "datapackbase.h"
#include "datadistributiontools.h"
#include "mousecursor.h"
#include <math.h>


uiHistogramSel::uiHistogramSel( uiParent* p, int id, bool fixdrawrg )
    : uiGroup( p, "Histogram with slider" )
    , id_(id)
    , startpix_(mUdf(int))
    , stoppix_(mUdf(int))
    , mousedown_(false)
    , rangeChanged(this)
{
    uiHistogramDisplay::Setup hsu;
    hsu.border( uiBorder(20,20,20,20) );
    hsu.fixdrawrg(fixdrawrg);
    histogramdisp_ = new uiHistogramDisplay( this, hsu, true );
    histogramdisp_->getMouseEventHandler().buttonPressed.notify(
			     mCB(this,uiHistogramSel,mousePressed) );
    histogramdisp_->getMouseEventHandler().buttonReleased.notify(
			     mCB(this,uiHistogramSel,mouseReleased) );
    histogramdisp_->getMouseEventHandler().movement.notify(
			     mCB(this,uiHistogramSel,mouseMoved) );
    histogramdisp_->reSize.notify(
			     mCB(this,uiHistogramSel,histogramResized));
    histogramdisp_->drawRangeChanged.notify(
			     mCB(this,uiHistogramSel,histDRChanged));
    xax_ = histogramdisp_->xAxis();

    init();
}


uiHistogramSel::~uiHistogramSel()
{
    detachAllNotifiers();

    delete minhandle_; delete maxhandle_;
    delete minvaltext_; delete maxvaltext_;
}


void uiHistogramSel::setEmpty()
{
    histogramdisp_->setEmpty();
    minhandle_->hide(); maxhandle_->hide();
    minvaltext_->hide(); maxvaltext_->hide();
}


bool uiHistogramSel::setDataPackID(
	DataPack::ID dpid, DataPackMgr::ID dmid, int version )
{
    const bool retval = histogramdisp_->setDataPackID( dpid, dmid,version);
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    cliprg_ = datarg_;
    if ( retval )
	drawAgain();
    return retval;
}


void uiHistogramSel::setData( const Array2D<float>* data )
{
    histogramdisp_->setData( data );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    cliprg_ = datarg_;
    drawAgain();
}


void uiHistogramSel::setData( const float* array, od_int64 sz )
{
    histogramdisp_->setData( array, sz );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    cliprg_ = datarg_;
    drawAgain();
}


bool uiHistogramSel::setData( const IOPar& iop )
{
    RefMan<FloatDistrib> distr = new FloatDistrib;
    DataDistributionChanger<float>( *distr ).usePar( iop );
    if ( distr->isEmpty() )
	return false;

    histogramdisp_->setDistribution( *distr );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    cliprg_ = datarg_;
    drawAgain();
    return true;
}


void uiHistogramSel::setMarkValue( float val, bool forx )
{
    if ( histogramdisp_ )
	histogramdisp_->setMarkValue( val, forx );
}


void uiHistogramSel::init()
{
    uiGraphicsScene& scene = histogramdisp_->scene();
    const int zval = 4;

    minvaltext_ = scene.addItem(
	new uiTextItem(uiString::empty(),OD::Alignment::Right) );
    minvaltext_->setZValue( zval+2 );
    maxvaltext_ = scene.addItem( new uiTextItem() );
    maxvaltext_->setZValue( zval+2 );

    uiManipHandleItem::Setup mhisu;
    mhisu.color_ = Color::DgbColor();
    mhisu.thickness_ = 2;
    mhisu.zval_ = zval+2;
    minhandle_ = scene.addItem( new uiManipHandleItem(mhisu) );
    maxhandle_ = scene.addItem( new uiManipHandleItem(mhisu) );
}


#define mNrPrec 6

void uiHistogramSel::drawText()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int posy = histogramdisp_->height() / 3;
    minvaltext_->setText( toUiString(cliprg_.start,mNrPrec));
    minvaltext_->setPos( uiPoint(startpix_-2,posy) );
    minvaltext_->show();

    maxvaltext_->setText( toUiString(cliprg_.stop,mNrPrec) );
    maxvaltext_->setPos( uiPoint(stoppix_+2,posy) );
    maxvaltext_->show();
}


void uiHistogramSel::drawLines()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    minhandle_->setPixPos( startpix_ );
    maxhandle_->setPixPos( stoppix_ );
}


void uiHistogramSel::drawAgain()
{
    startpix_ = xax_->getPix( cliprg_.start );
    stoppix_ = xax_->getPix( cliprg_.stop );

    drawText();
    drawLines();
}


void uiHistogramSel::histogramResized( CallBacker* cb )
{
    drawAgain();
}


void uiHistogramSel::setDataRange( const Interval<float>& rg )
{
    datarg_ = rg;
    cliprg_ = datarg_;
    drawAgain();
}


void uiHistogramSel::setSelRange( const Interval<float>& rg )
{
    cliprg_ = rg;
    drawAgain();
}


bool uiHistogramSel::changeLinePos( bool firstclick )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() )
	return false;

    const MouseEvent& ev = meh.event();
    if ( !(ev.buttonState() & OD::LeftButton ) ||
	  (ev.buttonState() & OD::MidButton ) ||
	  (ev.buttonState() & OD::RightButton ) )
	return false;

    const int diff = stoppix_ - startpix_;
    if ( !firstclick && fabs(float(diff)) <= 1 )
	return false;

    const int mousepix = ev.pos().x_;
    const float mouseposval = xax_->getVal( ev.pos().x_ );

    const Interval<float> histxrg = histogramdisp_->xAxis()->range();
    const bool insiderg = datarg_.includes(mouseposval,true) &&
			  histxrg.includes(mouseposval,true);
    if ( !firstclick && !insiderg )
	return false;

#define clickrg 5
    if ( mouseposval < (cliprg_.start+cliprg_.stop)/2 )
    {
	const bool faraway = (mousepix > startpix_+clickrg) ||
			     (mousepix < startpix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.start = mouseposval;
    }
    else
    {
	const bool faraway = (mousepix > stoppix_+clickrg) ||
			     (mousepix < stoppix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.stop = mouseposval;
    }

    rangeChanged.trigger();
    return true;
}


void uiHistogramSel::mousePressed( CallBacker* cb )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || mousedown_ ) return;

    mousedown_ = true;
    if ( changeLinePos(true) )
    {
	drawAgain();
	meh.setHandled( true );
    }
    else
	mousedown_ = false;
}


void uiHistogramSel::mouseMoved( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || !mousedown_ ) return;

    if ( changeLinePos() )
    {
	drawAgain();
	useClipRange();
    }

    meh.setHandled( true );
}


void uiHistogramSel::mouseReleased( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || !mousedown_ )
	return;

    mousedown_ = false;
    useClipRange();
    drawAgain();
    meh.setHandled( true );
}


void uiHistogramSel::histDRChanged( CallBacker* cb )
{
    const Interval<float>& drg = histogramdisp_->getDrawRange();
    if ( cliprg_.start<drg.start )
	cliprg_.start = drg.start;
    if ( cliprg_.stop>drg.stop )
	cliprg_.stop = drg.stop;

    startpix_ = xax_->getPix( cliprg_.start );
    stoppix_ = xax_->getPix( cliprg_.stop );

    minhandle_->setPixPos( startpix_ );
    maxhandle_->setPixPos( stoppix_ );
    useClipRange();
}
