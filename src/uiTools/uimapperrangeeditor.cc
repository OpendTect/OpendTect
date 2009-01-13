/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Dec 2008
 RCS:		$Id: uimapperrangeeditor.cc,v 1.5 2009-01-13 05:54:59 cvsumesh Exp $
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
    , minlineval_(0)
    , maxlineval_(0)
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
}


uiMapperRangeEditor::~uiMapperRangeEditor()
{
    delete minline_; delete maxline_;
    delete leftcoltab_; delete centercoltab_; delete rightcoltab_;
    delete minlineval_; delete maxlineval_;
}


bool uiMapperRangeEditor::setDataPackID( DataPack::ID dpid,
					    DataPackMgr::ID dmid )
{
    return histogramdisp_->setDataPackID( dpid, dmid );
}


void uiMapperRangeEditor::setMarkValue( float val, bool forx )
{
    if ( histogramdisp_ )
	histogramdisp_->setMarkValue( val, forx );
}


void uiMapperRangeEditor::setColTabMapperSetupWthSeq(
		const ColTab::MapperSetup& ms, const ColTab::Sequence& cseq )
{
    ctbmapper_ = new ColTab::MapperSetup();
    ctbmapper_->cliprate_ = ms.cliprate_;
    ctbmapper_->type_ = ColTab::MapperSetup::Fixed;
    ctbmapper_->autosym0_ = ms.autosym0_;
    ctbmapper_->symmidval_ = ms.symmidval_;
    ctbmapper_->maxpts_ = ms.maxpts_;
    ctbmapper_->nrsegs_ = ms.nrsegs_;
    ctbmapper_->start_ = ms.start_;
    ctbmapper_->width_ = ms.width_;

    ctbseq_ = &cseq;

    lefttminval_ = histogramdisp_->xVals()[0];
    rightmaxval_ = histogramdisp_->xVals()[ histogramdisp_->xVals().size() - 1];

    minlinebasepos_ = minlinecurpos_ = ms.start_;
    maxlinebasepos_ = maxlinecurpos_ = ms.start_ + ms.width_;

    drawText();
    drawLines();
    drawPixmaps();
}


void uiMapperRangeEditor::drawText()
{
    if ( !minlineval_ )
	minlineval_ = histogramdisp_->scene().addText(
		histogramdisp_->xAxis()->getPix(minlinecurpos_),
		histogramdisp_->height()/3, toString(minlinecurpos_),
		OD::AlignRight );

    if ( !maxlineval_ )
	maxlineval_ = histogramdisp_->scene().addText(
		histogramdisp_->xAxis()->getPix(maxlinecurpos_),
		histogramdisp_->height()/3, toString(maxlinecurpos_),
		OD::AlignLeft );
}


void uiMapperRangeEditor::fixTextPos()
{
    if ( mIsUdf(minlinecurpos_) || mIsUdf(maxlinecurpos_) )
	return;
    BufferString bsleft;
    bsleft += toString(minlinecurpos_);
    bsleft += " ";

    minlineval_->setText( bsleft.buf() );
    minlineval_->setPos( histogramdisp_->xAxis()->getPix(minlinecurpos_),
	    	      histogramdisp_->height()/3 );
    minlineval_->setAlignment( OD::AlignRight );

    BufferString bsright;
    bsright += toString(maxlinecurpos_);
    bsright += " ";
    maxlineval_->setText( bsright );
    maxlineval_->setPos( histogramdisp_->xAxis()->getPix(maxlinecurpos_),
	    	      histogramdisp_->height()/3 );
    maxlineval_->setAlignment( OD::AlignLeft );
}


void uiMapperRangeEditor::drawLines()
{
    MouseCursor cursor;
    cursor.shape_ = MouseCursor::SizeHor;
    if ( !minline_ )
    {
	int ltlnpix = histogramdisp_->xAxis()->getPix( minlinecurpos_ );
	minline_ = histogramdisp_->scene().addLine( ltlnpix, 0,
       					ltlnpix, histogramdisp_->height() );
	minline_->setPenStyle( LineStyle(LineStyle::Solid,2,Color(0,255,0)) );
	minline_->setCursor( cursor );
	minline_->setZValue( 4 );
    }

    if ( !maxline_ )
    {
	int rtlnpix = histogramdisp_->xAxis()->getPix( maxlinecurpos_ );
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
    //TODO this memory management sux... go for proper scaling.
    if ( leftcoltab_)
    { delete leftcoltab_; leftcoltab_ = 0; }

    if ( centercoltab_ )
    { delete centercoltab_; centercoltab_ = 0; }

    if ( rightcoltab_ )
    { delete rightcoltab_; rightcoltab_ = 0; }

    const int ltlnpix = histogramdisp_->xAxis()->getPix( minlinecurpos_ );
    const int rtlnpix = histogramdisp_->xAxis()->getPix( maxlinecurpos_ );

    if ( !leftcoltab_ )
    {
	ioPixmap pixmapleft( ltlnpix-
		            histogramdisp_->xAxis()->getPix(lefttminval_), 20 );
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
	ioPixmap pixmapright( 
		    histogramdisp_->xAxis()->getPix(rightmaxval_)-rtlnpix, 20 );
	pixmapright.fill( ctbseq_->color(1) );
	rightcoltab_ = histogramdisp_->scene().addPixmap( pixmapright );
    }

    fixPixmapsPos();
}


void uiMapperRangeEditor::fixPixmapsPos()
{
    const int ltminpix = histogramdisp_->xAxis()->getPix( lefttminval_ );
    const int lfbspospix = histogramdisp_->xAxis()->getPix( minlinebasepos_ );
    const int lflnpix = histogramdisp_->xAxis()->getPix( minlinecurpos_ );

    const int rtlnpix = histogramdisp_->xAxis()->getPix( maxlinecurpos_ );
    const int rtbspospix = histogramdisp_->xAxis()->getPix( maxlinebasepos_ );
    const int rtmxapix = histogramdisp_->xAxis()->getPix( rightmaxval_ );

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
    minline_->setLine( histogramdisp_->xAxis()->getPix(minlinecurpos_), 0,
    	  histogramdisp_->xAxis()->getPix(
	      			     minlinecurpos_), histogramdisp_->height());
    maxline_->setLine( histogramdisp_->xAxis()->getPix(maxlinecurpos_), 0,
	  			histogramdisp_->xAxis()->getPix(maxlinecurpos_),
				histogramdisp_->height());
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

    if ( !pressedonly && fabs(histogramdisp_->xAxis()->getPix(maxlinecurpos_)-
			  histogramdisp_->xAxis()->getPix(minlinecurpos_)) <=1 )
	return false;

    const float pointedpos = histogramdisp_->xAxis()->getVal(ev.pos().x);

    if ( pointedpos < (minlinecurpos_+maxlinecurpos_)/2 )
    {
	if ( pointedpos < lefttminval_ ) return false;

	if ( !(histogramdisp_->xAxis()->getPix(pointedpos) >
		    (histogramdisp_->xAxis()->getPix(minlinecurpos_)-10) &&
	       histogramdisp_->xAxis()->getPix(pointedpos) <
	            (histogramdisp_->xAxis()->getPix(minlinecurpos_)+10)) )
	    return false;

	minlinecurpos_ = pointedpos;
	if ( !mIsUdf(ctbmapper_->symmidval_) )
	    maxlinecurpos_ = ctbmapper_->symmidval_ + 
			     ( ctbmapper_->symmidval_ - minlinecurpos_ );
    }
    else
    {
	if ( pointedpos > rightmaxval_ ) return false;

	if ( !(histogramdisp_->xAxis()->getPix(pointedpos) >
		    (histogramdisp_->xAxis()->getPix(maxlinecurpos_)-10) &&
	       histogramdisp_->xAxis()->getPix(pointedpos) <
	       	    (histogramdisp_->xAxis()->getPix(maxlinecurpos_)+10)) )
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


void uiMapperRangeEditor::mouseMoved( CallBacker* cb )
{
    if ( !mousedown_ ) return;

    if ( changeLinePos() )
	drawAgain();
}


void uiMapperRangeEditor::mouseReleased( CallBacker* cb )
{
    if ( !mousedown_ ) return;
    
    mousedown_ = false;
    ctbmapper_->start_ = minlinecurpos_;
    ctbmapper_->width_ = maxlinecurpos_ - ctbmapper_->start_;

    drawAgain();
    rangeChanged.trigger();
}
