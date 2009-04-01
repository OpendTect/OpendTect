/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditor.cc,v 1.11 2009-04-01 14:35:39 cvsbert Exp $
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

uiMapperRangeEditor::uiMapperRangeEditor( uiParent* p, int id )
    : uiGroup( p, "Mapper with color slider group" )
    , id_(id)
    , ctbmapper_(0)
    , ctbseq_(0)
    , minline_(0)
    , maxline_(0)
    , leftcoltab_(0)
    , centercoltab_(0)
    , rightcoltab_(0)
    , lefttminval_(mUdf(float))
    , rightmaxval_(mUdf(float))
    , minlinebasepos_(mUdf(float))
    , maxlinebasepos_(mUdf(float))
    , minlinecurpos_(mUdf(float))
    , maxlinecurpos_(mUdf(float))
    , minlinevaltext_(0)
    , maxlinevaltext_(0)
    , mousedown_(0)
    , rangeChanged(this)
{
    uiHistogramDisplay::Setup hsu;
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
}


uiMapperRangeEditor::~uiMapperRangeEditor()
{
    delete minline_; delete maxline_;
    delete leftcoltab_; delete centercoltab_; delete rightcoltab_;
    delete minlinevaltext_; delete maxlinevaltext_;
    delete ctbseq_;
    delete ctbmapper_;
}


bool uiMapperRangeEditor::setDataPackID( DataPack::ID dpid,
					 DataPackMgr::ID dmid )
{
    bool returnval = histogramdisp_->setDataPackID( dpid, dmid );
    initSetUp();
    return returnval;
}


void uiMapperRangeEditor::setMarkValue( float val, bool forx )
{
    if ( histogramdisp_ )
	histogramdisp_->setMarkValue( val, forx );
}


void uiMapperRangeEditor::setColTabMapperSetup( const ColTab::MapperSetup& ms )
{
    if ( ctbmapper_ )
    { delete ctbmapper_; ctbmapper_ = 0; }

    ctbmapper_ = new ColTab::MapperSetup();
    *ctbmapper_ = ms;
    ctbmapper_->type_ = ColTab::MapperSetup::Fixed;
    initSetUp();
}


void uiMapperRangeEditor::setColTabSeq( const ColTab::Sequence& cseq )
{ 
    if ( ctbseq_ )
    { delete ctbseq_; ctbseq_ = 0; }
    
    ctbseq_ = new ColTab::Sequence();
    *ctbseq_ = cseq;
    initSetUp();
}


void uiMapperRangeEditor::initSetUp()
{
    const bool nodata = histogramdisp_->xVals().isEmpty();
    lefttminval_ = nodata ? 0 : histogramdisp_->xVals().first();
    rightmaxval_ = nodata ? 1 : histogramdisp_->xVals().last();

    if ( ctbmapper_ )
    {
	minlinebasepos_ = minlinecurpos_ = ctbmapper_->start_;
	maxlinebasepos_ = maxlinecurpos_ = ctbmapper_->start_ + 
	    					ctbmapper_->width_;
    }
    
    draw();
}


void uiMapperRangeEditor::draw()
{
    drawText();
    drawLines();
    drawPixmaps();
}


void uiMapperRangeEditor::drawText()
{
    if ( mIsUdf(minlinecurpos_) || mIsUdf(maxlinecurpos_) )
	return;

    if ( !minlinevaltext_ )
	minlinevaltext_ = histogramdisp_->scene().addText(
		xax_->getPix(minlinecurpos_),
		histogramdisp_->height()/3, toString(minlinecurpos_),
		Alignment::Right );

    if ( !maxlinevaltext_ )
	maxlinevaltext_ = histogramdisp_->scene().addText(
		xax_->getPix(maxlinecurpos_),
		histogramdisp_->height()/3, toString(maxlinecurpos_),
		Alignment::Left );

    fixTextPos();
}


void uiMapperRangeEditor::fixTextPos()
{
    if ( mIsUdf(minlinecurpos_) || mIsUdf(maxlinecurpos_) )
	return;

    BufferString bsleft( toString(minlinecurpos_), " " );
    minlinevaltext_->setText( bsleft.buf() );
    minlinevaltext_->setPos( xax_->getPix(minlinecurpos_),
			     histogramdisp_->height()/3 );
    minlinevaltext_->setAlignment( Alignment::Right );

    BufferString bsright( toString(maxlinecurpos_), " " );
    maxlinevaltext_->setText( bsright );
    maxlinevaltext_->setPos( xax_->getPix(maxlinecurpos_),
			     histogramdisp_->height()/3 );
    maxlinevaltext_->setAlignment( Alignment::Left );
}


void uiMapperRangeEditor::drawLines()
{
    if ( mIsUdf(minlinecurpos_) || mIsUdf(maxlinecurpos_) )
	return;

    MouseCursor cursor;
    cursor.shape_ = MouseCursor::SizeHor;
    if ( !minline_ )
    {
	int ltlnpix = xax_->getPix( minlinecurpos_ );
	minline_ = histogramdisp_->scene().addLine( ltlnpix, 0,
       					ltlnpix, histogramdisp_->height() );
	minline_->setPenStyle( LineStyle(LineStyle::Solid,2,Color(0,255,0)) );
	minline_->setCursor( cursor );
	minline_->setZValue( 4 );
    }

    if ( !maxline_ )
    {
	int rtlnpix = xax_->getPix( maxlinecurpos_ );
	maxline_ = histogramdisp_->scene().addLine( rtlnpix, 0,
				            rtlnpix, histogramdisp_->height() );
	maxline_->setPenStyle( LineStyle(LineStyle::Solid,2,Color(0,255,0)) );
	maxline_->setCursor( cursor );
	maxline_->setZValue( 4 );
    }

    fixLinesPos();
}


void uiMapperRangeEditor::drawPixmaps()
{
    if ( !ctbseq_ || mIsUdf(minlinecurpos_) || mIsUdf(maxlinecurpos_) )
       	return;

    //TODO this memory management sux... go for proper scaling.
    if ( leftcoltab_)
    { delete leftcoltab_; leftcoltab_ = 0; }

    if ( centercoltab_ )
    { delete centercoltab_; centercoltab_ = 0; }

    if ( rightcoltab_ )
    { delete rightcoltab_; rightcoltab_ = 0; }

    const int ltlnpix = xax_->getPix( minlinecurpos_ );
    const int rtlnpix = xax_->getPix( maxlinecurpos_ );

    if ( !leftcoltab_ )
    {
	ioPixmap pixmapleft( ltlnpix-
		            xax_->getPix(lefttminval_), 20 );
	pixmapleft.fill( ctbseq_->color(0) );
	leftcoltab_ = histogramdisp_->scene().addPixmap( pixmapleft );
    }

    if ( !centercoltab_ )
    {
	ioPixmap pixmapseq( *ctbseq_, rtlnpix-ltlnpix, 20 );
	centercoltab_ = histogramdisp_->scene().addPixmap( pixmapseq );
    }

    if ( !rightcoltab_ )
    {
	ioPixmap pixmapright( xax_->getPix(rightmaxval_)-rtlnpix, 20 );
	pixmapright.fill( ctbseq_->color(1) );
	rightcoltab_ = histogramdisp_->scene().addPixmap( pixmapright );
    }

    fixPixmapsPos();
}


void uiMapperRangeEditor::fixPixmapsPos()
{
    const int ltminpix = xax_->getPix( lefttminval_ );
    const int lfbspospix = xax_->getPix( minlinebasepos_ );
    const int lflnpix = xax_->getPix( minlinecurpos_ );

    const int rtlnpix = xax_->getPix( maxlinecurpos_ );
    const int rtbspospix = xax_->getPix( maxlinebasepos_ );
    const int rtmxapix = xax_->getPix( rightmaxval_ );

    // TODO: make scaling alive 
    if ( leftcoltab_ )
    {
	leftcoltab_->setOffset( ltminpix, histogramdisp_->height()-20 );
	//leftcoltab_->scaleArdXY( (lflnpix-ltminpix)/(lfbspospix-ltminpix), 1,
	//		       ltminpix, histogramdisp_->height()-20 );
    }

    if ( centercoltab_ )
    {
	centercoltab_->setOffset( lflnpix, histogramdisp_->height()-20 );
	//centercoltab_->scaleArdXY((rtlnpix-lflnpix)/(rtbspospix-lfbspospix),
	//		       ,1, lflnpix, histogramdisp_->height()-20 );
    }

    if ( rightcoltab_ )
    {
	rightcoltab_->setOffset( rtlnpix, histogramdisp_->height()-20 );
	//rightcoltab_->scaleArdXY( (rtmxapix-rtlnpix)/(rtmxapix-rtbspospix), 1,
	//		       rtlnpix, histogramdisp_->height()-20 );
    }
}


void uiMapperRangeEditor::drawAgain()
{
    fixTextPos();
    fixLinesPos();
    drawPixmaps();
}


void uiMapperRangeEditor::fixLinesPos()
{
    if ( mIsUdf(minlinecurpos_) || mIsUdf(maxlinecurpos_) )
	return;

    minline_->setLine( xax_->getPix(minlinecurpos_), 0,
		       xax_->getPix(minlinecurpos_), histogramdisp_->height());
    maxline_->setLine( xax_->getPix(maxlinecurpos_), 0,
	  	       xax_->getPix(maxlinecurpos_), histogramdisp_->height());
}


void uiMapperRangeEditor::histogramResized( CallBacker* cb )
{ drawAgain(); }


#define mGetMousePos()  \
    if ( histogramdisp_->getMouseEventHandler().isHandled() ) \
	return false; \
    const MouseEvent& ev = histogramdisp_->getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::LeftButton ) || \
	  (ev.buttonState() & OD::MidButton ) || \
	  (ev.buttonState() & OD::RightButton ) ) \
	return false; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm = !isctrl && !isoth

bool uiMapperRangeEditor::changeLinePos( bool pressedonly )
{
    mGetMousePos();
     const int diff = xax_->getPix(maxlinecurpos_) - xax_->getPix(minlinecurpos_);
    if ( !pressedonly && fabs(float(diff)) <= 1 )
	return false;

    const float pointedpos = xax_->getVal(ev.pos().x);

    if ( pointedpos < (minlinecurpos_+maxlinecurpos_)/2 )
    {
	if ( pointedpos < lefttminval_ ) return false;

	if ( !(xax_->getPix(pointedpos) > (xax_->getPix(minlinecurpos_)-10) &&
	       xax_->getPix(pointedpos) < (xax_->getPix(minlinecurpos_)+10)) )
	    return false;

	minlinecurpos_ = pointedpos;
	if ( !mIsUdf(ctbmapper_->symmidval_) )
	    maxlinecurpos_ = ctbmapper_->symmidval_ + 
			     ( ctbmapper_->symmidval_ - minlinecurpos_ );
    }
    else
    {
	if ( pointedpos > rightmaxval_ ) return false;

	if ( !(xax_->getPix(pointedpos) > (xax_->getPix(maxlinecurpos_)-10) &&
	       xax_->getPix(pointedpos) < (xax_->getPix(maxlinecurpos_)+10)) )
	    return false;

	maxlinecurpos_ = pointedpos;
	if ( !mIsUdf(ctbmapper_->symmidval_) )
	    minlinecurpos_ = ctbmapper_->symmidval_ -
			     ( maxlinecurpos_ - ctbmapper_->symmidval_ );
    }

    return true;
}


void uiMapperRangeEditor::mousePressed( CallBacker* cb )
{
    if ( mousedown_ ) return;
    
    mousedown_ = true;
    if ( changeLinePos(true ) )
	drawAgain();
}


void uiMapperRangeEditor::mouseMoved( CallBacker* )
{
    if ( !mousedown_ ) return;

    if ( changeLinePos() )
	drawAgain();
}


void uiMapperRangeEditor::mouseReleased( CallBacker* )
{
    if ( !mousedown_ ) return;
    
    mousedown_ = false;
    ctbmapper_->start_ = minlinecurpos_;
    ctbmapper_->width_ = maxlinecurpos_ - ctbmapper_->start_;

    drawAgain();
    rangeChanged.trigger();
}
