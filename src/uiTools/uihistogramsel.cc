/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihistogramsel.h"

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uistrings.h"
#include "uitoolbutton.h"

#include "datapackbase.h"
#include "mousecursor.h"
#include "keystrs.h"
#include <math.h>


uiHistogramSel::uiHistogramSel( uiParent* p,
				const uiHistogramDisplay::Setup& su, int id )
    : uiGroup( p, "Histogram with slider" )
    , id_(id)
    , rangeChanged(this)
{
    uiHistogramDisplay::Setup hsu( su );
    histogramdisp_ = new uiHistogramDisplay( this, hsu, true );
    mAttachCB( histogramdisp_->getMouseEventHandler().buttonPressed,
	       uiHistogramSel::mousePressed );
    mAttachCB( histogramdisp_->getMouseEventHandler().buttonReleased,
	       uiHistogramSel::mouseReleased );
    mAttachCB( histogramdisp_->getMouseEventHandler().movement,
	       uiHistogramSel::mouseMoved );
    mAttachCB( histogramdisp_->reSize, uiHistogramSel::histogramResized );
    mAttachCB( histogramdisp_->drawRangeChanged, uiHistogramSel::histDRChanged);
    xax_ = histogramdisp_->xAxis();

    init();
    mAttachCB( postFinalize(), uiHistogramSel::finalizedCB );
}


uiHistogramSel::~uiHistogramSel()
{
    detachAllNotifiers();
    delete minhandle_;
    delete maxhandle_;
    delete minvaltext_;
    delete maxvaltext_;
}


void uiHistogramSel::setEmpty()
{
    histogramdisp_->setEmpty();
    minhandle_->hide();
    maxhandle_->hide();
    minvaltext_->hide();
    maxvaltext_->hide();
}


bool uiHistogramSel::setDataPackID( const DataPackID& dpid,
				    const DataPackMgr::MgrID& dmid, int version)
{
    ConstRefMan<DataPack> dp = DPM(dmid).getDP( dpid );
    return dp ? setDataPack( *dp.ptr(), version ) : false;
}


bool uiHistogramSel::setDataPack( const DataPack& dp, int version )
{
    const bool retval = histogramdisp_->setDataPack( dp, version );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_ = nodata ? Interval<float>(0,1)
			       : histogramdisp_->setup().xrg_;
    if ( nodata )
	cliprg_ = datarg_;

    if ( retval )
	drawAgain();

    return retval;
}


void uiHistogramSel::setData( const Array2D<float>* data )
{
    histogramdisp_->setData( data );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    cliprg_ = datarg_ = nodata ? Interval<float>(0,1)
			       : histogramdisp_->setup().xrg_;
    drawAgain();
}


void uiHistogramSel::setData( const float* array, int sz )
{
    histogramdisp_->setData( array, sz );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    cliprg_ = datarg_ = nodata ? Interval<float>(0,1)
			       : histogramdisp_->setup().xrg_;
    drawAgain();
}


void uiHistogramSel::setMarkValue( float val, bool forx )
{
    if ( histogramdisp_ )
	histogramdisp_->setMarkValue( val, forx );
}


void uiHistogramSel::init()
{
    uiGraphicsScene& scene = histogramdisp_->scene();
    const int zval = 40;

    minvaltext_ = scene.addItem(
	new uiTextItem(uiStrings::sEmptyString(),Alignment::Right) );
    minvaltext_->setZValue( zval+2 );
    maxvaltext_ = scene.addItem( new uiTextItem() );
    maxvaltext_->setZValue( zval+2 );

    MouseCursor cursor( MouseCursor::SizeHor );
    OD::LineStyle ls( OD::LineStyle::Solid, 2, OD::Color(0,255,0) );
    minhandle_ = scene.addItem( new uiLineItem() );
    minhandle_->setPenStyle( ls );
    minhandle_->setCursor( cursor );
    minhandle_->setZValue( zval+2 );

    maxhandle_ = scene.addItem( new uiLineItem() );
    maxhandle_->setPenStyle( ls );
    maxhandle_->setCursor( cursor );
    maxhandle_->setZValue( zval+2 );

    auto* button = new uiToolButton( nullptr, "refresh", tr("Reset selection"),
				     mCB(this,uiHistogramSel,resetPressed) );
    resetbutton_ = scene.addItem( new uiObjectItem(button) );
    resetbutton_->setZValue( zval+2 );
    resetbutton_->setObjectSize( 25, 25 ); // looks best
    resetbutton_->setItemIgnoresTransformations( true );
}


static int reqNrDec( float val )
{
    if ( mIsZero(val,mDefEps) )
	return 0;
    if ( val < 0 )
	val = -val;

    const int logval = (int) Math::Log( val );
    int ret = 2 - logval;
    return ret < 0 ? 0 : (ret > 8 ? 8 : ret);
}


void uiHistogramSel::drawText()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const bool showtext = slidertextpol_ == Always ||
			( slidertextpol_ == OnMove && mousedown_ );
    const int posy = histogramdisp_->height() / 3;
    minvaltext_->setText( toUiString(cliprg_.start_,reqNrDec(cliprg_.start_)) );
    minvaltext_->setPos( uiPoint(startpix_-2,posy) );
    minvaltext_->setVisible( showtext );

    maxvaltext_->setText( toUiString(cliprg_.stop_,reqNrDec(cliprg_.stop_)) );
    maxvaltext_->setPos( uiPoint(stoppix_+2,posy) );
    maxvaltext_->setVisible( showtext );
}


void uiHistogramSel::drawLines()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    uiAxisHandler* yax = histogramdisp_->yAxis(false);
    const int disph = histogramdisp_->viewHeight();
    int y0pix = yax ? yax->pixRange().start_+2 : 0;
    int y1pix = yax ? yax->pixRange().stop_+1 : disph;
    minhandle_->setLine( startpix_, y0pix, startpix_, y1pix );
    minhandle_->setCursor( MouseCursor::SizeHor );
    minhandle_->show();

    maxhandle_->setLine( stoppix_, y0pix, stoppix_, y1pix );
    maxhandle_->setCursor( MouseCursor::SizeHor );
    maxhandle_->show();
}


void uiHistogramSel::drawAgain()
{
    startpix_ = xax_->getPix( cliprg_.start_ );
    stoppix_ = xax_->getPix( cliprg_.stop_ );

    drawText();
    drawLines();
    drawPixmaps();

    resetbutton_->setPos( 2, histogramdisp_->viewHeight() - 27 );
}


void uiHistogramSel::finalizedCB( CallBacker* cb )
{
    setDefaultSelRange( cliprg_ );
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


void uiHistogramSel::setDefaultSelRange( const Interval<float>& rg )
{
    initialcliprg_ = rg;
}


void uiHistogramSel::setSliderTextPolicy( uiHistogramSel::SliderTextPolicy pol )
{
    slidertextpol_ = pol;
}


uiHistogramSel::SliderTextPolicy uiHistogramSel::sliderTextPolicy() const
{
    return slidertextpol_;
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
    const bool insiderg = histxrg.includes(mouseposval,true);
#define clickrg 5
    if ( mouseposval < (cliprg_.start_+cliprg_.stop_)/2 )
    {
	const bool faraway = (mousepix > startpix_+clickrg) ||
			     (mousepix < startpix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.start_ = insiderg ? mouseposval : histxrg.start_;
	makeSymmetricalIfNeeded( true );
    }
    else
    {
	const bool faraway = (mousepix > stoppix_+clickrg) ||
			     (mousepix < stoppix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.stop_ = insiderg ? mouseposval : histxrg.stop_;
	makeSymmetricalIfNeeded( false );
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
    if ( cliprg_.start_<drg.start_ )
	cliprg_.start_ = drg.start_;
    if ( cliprg_.stop_>drg.stop_ )
	cliprg_.stop_ = drg.stop_;

    startpix_ = xax_->getPix( cliprg_.start_ );
    stoppix_ = xax_->getPix( cliprg_.stop_ );

    const int height = histogramdisp_->height();
    minhandle_->setLine( startpix_, 0, startpix_, height );
    maxhandle_->setLine( stoppix_, 0, stoppix_, height );
    useClipRange();
}


void uiHistogramSel::resetPressed( CallBacker* )
{
    setSelRange( initialcliprg_ );
    rangeChanged.trigger();
}
