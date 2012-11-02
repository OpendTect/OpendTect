/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uimapperrangeeditor.h"

#include "uiaxishandler.h"
#include "uihistogramdisplay.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"
#include "mousecursor.h"
#include "pixmap.h"
#include <math.h>


uiMapperRangeEditor::uiMapperRangeEditor( uiParent* p, int id, bool fixdrawrg )
    : uiGroup( p, "Mapper with color slider group" )
    , id_(id)
    , ctmapper_(new ColTab::MapperSetup)
    , ctseq_(new ColTab::Sequence)
    , startpix_(mUdf(int))
    , stoppix_(mUdf(int))
    , mousedown_(false)
    , rangeChanged(this)
{
    uiHistogramDisplay::Setup hsu;
    hsu.border( uiBorder(20,20,20,40) );
    hsu.fixdrawrg(fixdrawrg);
    histogramdisp_ = new uiHistogramDisplay( this, hsu, true );
    histogramdisp_->getMouseEventHandler().buttonPressed.notify(
	    		     mCB(this,uiMapperRangeEditor,mousePressed) );
    histogramdisp_->getMouseEventHandler().buttonReleased.notify(
	    		     mCB(this,uiMapperRangeEditor,mouseReleased) );
    histogramdisp_->getMouseEventHandler().movement.notify(
	    		     mCB(this,uiMapperRangeEditor,mouseMoved) );
    histogramdisp_->reSize.notify(
	    		     mCB(this,uiMapperRangeEditor,histogramResized));
    xax_ = histogramdisp_->xAxis();

    init();
}


uiMapperRangeEditor::~uiMapperRangeEditor()
{
    delete minline_; delete maxline_;
    delete leftcoltab_; delete centercoltab_; delete rightcoltab_;
    delete minvaltext_; delete maxvaltext_;
    delete ctseq_; delete ctmapper_;
}


bool uiMapperRangeEditor::setDataPackID( DataPack::ID dpid,
					 DataPackMgr::ID dmid )
{
    const bool retval = histogramdisp_->setDataPackID( dpid, dmid );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    if ( retval ) drawAgain();
    return retval;
}


void uiMapperRangeEditor::setData( const Array2D<float>* data )
{
    histogramdisp_->setData( data );
    const bool nodata = histogramdisp_->xVals().isEmpty();
    datarg_.start = nodata ? 0 : histogramdisp_->xVals().first();
    datarg_.stop = nodata ? 1 : histogramdisp_->xVals().last();
    drawAgain();
}


void uiMapperRangeEditor::setMarkValue( float val, bool forx )
{
    if ( histogramdisp_ )
	histogramdisp_->setMarkValue( val, forx );
}


void uiMapperRangeEditor::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    uiAxisHandler* axhndler = histogramdisp_->xAxis();
    if ( !axhndler ) return;

    StepInterval<float> axrange = axhndler->range();

    if ( !ms.range_.includes(axrange.start,true) ||
	 !ms.range_.includes(axrange.stop,true) )
    {
	axrange.include( ms.range_ );
	histogramdisp_->setup().xrg( axrange );
	histogramdisp_->gatherInfo();
	histogramdisp_->draw();
    }

    *ctmapper_ = ms;
    ctmapper_->type_ = ColTab::MapperSetup::Fixed;
    const Interval<float> rg = ctmapper_->range_;
    cliprg_.start = rg.isRev() ? rg.stop : rg.start;
    cliprg_.stop = rg.isRev() ? rg.start : rg.stop;
    drawAgain();
}


void uiMapperRangeEditor::setColTabSeq( const ColTab::Sequence& cseq )
{ 
    *ctseq_ = cseq;
    drawAgain();
}


void uiMapperRangeEditor::init()
{
    uiGraphicsScene& scene = histogramdisp_->scene();
    const int zval = 4;

    leftcoltab_ = scene.addItem( new uiPixmapItem() );
    leftcoltab_->setZValue( zval );
    centercoltab_ = scene.addItem( new uiPixmapItem() );
    centercoltab_->setZValue( zval );
    rightcoltab_ = scene.addItem( new uiPixmapItem() );
    rightcoltab_->setZValue( zval );

    minvaltext_ = scene.addItem( new uiTextItem("",Alignment::Right) );
    maxvaltext_ = scene.addItem( new uiTextItem() );

    MouseCursor cursor( MouseCursor::SizeHor );
    LineStyle ls( LineStyle::Solid, 2, Color(0,255,0) );
    minline_ = scene.addItem( new uiLineItem() );
    minline_->setPenStyle( ls );
    minline_->setCursor( cursor );
    minline_->setZValue( zval+2 );

    maxline_ = scene.addItem( new uiLineItem() );
    maxline_->setPenStyle( ls );
    maxline_->setCursor( cursor );
    maxline_->setZValue( zval+2 );
}


void uiMapperRangeEditor::drawText()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int posy = histogramdisp_->height() / 3;
    minvaltext_->setText( toString(cliprg_.start) );
    minvaltext_->setPos( uiPoint(startpix_-2,posy) );

    maxvaltext_->setText( toString(cliprg_.stop) );
    maxvaltext_->setPos( uiPoint(stoppix_+2,posy) );
}


void uiMapperRangeEditor::drawPixmaps()
{
    if ( !ctseq_ || mIsUdf(startpix_) || mIsUdf(stoppix_) )
       	return;

    const int disph = histogramdisp_->height();
    const int pmh = 20;
    const int datastartpix = xax_->getPix( datarg_.start );
    const int datastoppix = xax_->getPix( datarg_.stop );

    ioPixmap leftpixmap( startpix_-datastartpix, pmh );
    leftpixmap.fill( ctseq_->color(
			    mCast(float,ctmapper_->range_.width()>0 ? 0:1)) );
    leftcoltab_->setPixmap( leftpixmap );
    leftcoltab_->setOffset( datastartpix, disph-pmh-1 );

    ColTab::Sequence ctseq( *ctseq_ );
    if ( ctmapper_->range_.width() < 0 )
	ctseq.flipColor();

    ioPixmap centerpixmap( ctseq, stoppix_-startpix_, pmh, true );
    centercoltab_->setPixmap( centerpixmap );
    centercoltab_->setOffset( startpix_, disph-pmh-1 );

    ioPixmap rightpixmap( datastoppix-stoppix_, pmh );
    rightpixmap.fill( ctseq_->color(
			     mCast(float,ctmapper_->range_.width()>0 ? 1:0)) );
    rightcoltab_->setPixmap( rightpixmap );
    rightcoltab_->setOffset( stoppix_, disph-pmh-1 );
}


void uiMapperRangeEditor::drawLines()
{
    if ( mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int height = histogramdisp_->height();
    minline_->setLine( startpix_, 0, startpix_, height );
    maxline_->setLine( stoppix_, 0, stoppix_, height );
}


void uiMapperRangeEditor::drawAgain()
{
    startpix_ = xax_->getPix( cliprg_.start );
    stoppix_ = xax_->getPix( cliprg_.stop );

    drawText();
    drawLines();
    drawPixmaps();
}


void uiMapperRangeEditor::histogramResized( CallBacker* cb )
{ drawAgain(); }


bool uiMapperRangeEditor::changeLinePos( bool firstclick )
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

    const int mousepix = ev.pos().x;
    const float mouseposval = xax_->getVal( ev.pos().x );

    const bool insiderg = datarg_.includes(mouseposval,true) &&
        histogramdisp_->setup().xrg_.includes(mouseposval,true);
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
	if ( (ctmapper_->type_==ColTab::MapperSetup::Fixed) &&
	     !mIsUdf(ctmapper_->symmidval_) )
	    cliprg_.stop = 2*ctmapper_->symmidval_ - cliprg_.start;
    }
    else
    {
	const bool faraway = (mousepix > stoppix_+clickrg) ||
			     (mousepix < stoppix_-clickrg);
	if ( firstclick && faraway )
	    return false;

	cliprg_.stop = mouseposval;
	if ( (ctmapper_->type_==ColTab::MapperSetup::Fixed) &&
	     !mIsUdf(ctmapper_->symmidval_) )
	    cliprg_.start = 2*ctmapper_->symmidval_ - cliprg_.stop;
    }

    return true;
}


void uiMapperRangeEditor::mousePressed( CallBacker* cb )
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


void uiMapperRangeEditor::mouseMoved( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || !mousedown_ ) return;

    if ( changeLinePos() )
	drawAgain();
    meh.setHandled( true );
}


void uiMapperRangeEditor::mouseReleased( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getMouseEventHandler();
    if ( meh.isHandled() || !mousedown_ ) return;

    mousedown_ = false;
    ctmapper_->range_.start = ctmapper_->range_.isRev() ? cliprg_.stop
						       : cliprg_.start;
    ctmapper_->range_.stop = ctmapper_->range_.isRev() ? cliprg_.start
						      : cliprg_.stop;
    drawAgain();
    rangeChanged.trigger();
    meh.setHandled( true );
}
