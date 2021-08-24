/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Dec 2008
________________________________________________________________________

-*/

#include "uimapperrangeeditor.h"

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uihistogramdisplay.h"
#include "uipixmap.h"
#include "uistrings.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "datapackbase.h"
#include "mousecursor.h"
#include <math.h>


uiMapperRangeEditor::uiMapperRangeEditor( uiParent* p, int id, bool fixdrawrg )
    : uiHistogramSel( p, uiHistogramDisplay::Setup()
	    		.border( uiBorder(20,20,20,40)).fixdrawrg(fixdrawrg),id)
    , ctmapper_(new ColTab::MapperSetup)
    , ctseq_(new ColTab::Sequence)
    , sequenceChanged(this)
{
    initPixmaps();
}


uiMapperRangeEditor::~uiMapperRangeEditor()
{
    delete ctminline_; delete ctmaxline_;
    delete leftcoltab_; delete centercoltab_; delete rightcoltab_;
    delete ctseq_; delete ctmapper_;
}


void uiMapperRangeEditor::setEmpty()
{
    uiHistogramSel::setEmpty();
    ctminline_->hide(); ctmaxline_->hide();
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


static uiLineItem* createLineItem( uiGraphicsScene& scene, int zval )
{
    uiLineItem* line = scene.addItem( new uiLineItem() );
    line->setPenStyle(
	OD::LineStyle(OD::LineStyle::Solid,2,Color(0,255,0)) );
    line->setZValue( zval+2 );
    return line;
}


void uiMapperRangeEditor::initPixmaps()
{
    uiGraphicsScene& scene = histogramdisp_->scene();
    const int zval = 40;

    leftcoltab_ = scene.addItem( new uiPixmapItem() );
    leftcoltab_->setZValue( zval );
    centercoltab_ = scene.addItem( new uiPixmapItem() );
    centercoltab_->setZValue( zval );
    rightcoltab_ = scene.addItem( new uiPixmapItem() );
    rightcoltab_->setZValue( zval );

    ctminline_ = createLineItem( scene, zval );
    ctmaxline_ = createLineItem( scene, zval );
}


void uiMapperRangeEditor::drawPixmaps()
{
    if ( !ctseq_ || mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int disph = histogramdisp_->viewHeight();
    const int pmh = 20;
    const int datastartpix = xax_->getPix( datarg_.start );
    const int datastoppix = xax_->getPix( datarg_.stop );

    ColTab::Sequence ctseq( *ctseq_ );
    if ( ctmapper_->flipseq_ || ctmapper_->range_.width() < 0 )
	ctseq.flipColor();

    uiPixmap leftpixmap( startpix_-datastartpix, pmh );
    leftpixmap.fill( ctseq.color(
			    mCast(float,ctmapper_->range_.width()>0 ? 0:1)) );
    leftcoltab_->setPixmap( leftpixmap );
    leftcoltab_->setOffset( datastartpix, disph-pmh-1 );

    uiPixmap centerpixmap( stoppix_-startpix_, pmh );
    centerpixmap.fill( ctseq, true );
    centercoltab_->setPixmap( centerpixmap );
    centercoltab_->setOffset( startpix_, disph-pmh-1 );

    uiPixmap rightpixmap( datastoppix-stoppix_, pmh );
    rightpixmap.fill( ctseq.color(
			     mCast(float,ctmapper_->range_.width()>0 ? 1:0)) );
    rightcoltab_->setPixmap( rightpixmap );
    rightcoltab_->setOffset( stoppix_, disph-pmh-1 );
}


void uiMapperRangeEditor::drawLines()
{
    uiHistogramSel::drawLines();

    const int disph = histogramdisp_->viewHeight();
    const int pmh = 20;
    const int y0pix = disph - pmh;
    const int y1pix = y0pix + pmh-2;
    ctminline_->setLine( startpix_, y0pix, startpix_, y1pix );
    ctmaxline_->setLine( stoppix_, y0pix, stoppix_, y1pix );
    ctminline_->show();
    ctmaxline_->show();
}


void uiMapperRangeEditor::makeSymmetricalIfNeeded( bool isstartfixed )
{
    if ( ctmapper_->type_==ColTab::MapperSetup::Fixed &&
	     !mIsUdf(ctmapper_->symmidval_) )
    {
	if ( isstartfixed )
	    cliprg_.stop = 2*ctmapper_->symmidval_ - cliprg_.start;
	else
	    cliprg_.start = 2*ctmapper_->symmidval_ - cliprg_.stop;
    }
}


void uiMapperRangeEditor::useClipRange()
{
    ctmapper_->range_.start =
	ctmapper_->range_.isRev() ? cliprg_.stop : cliprg_.start;
    ctmapper_->range_.stop =
	ctmapper_->range_.isRev() ? cliprg_.start : cliprg_.stop;
    rangeChanged.trigger();
}


void uiMapperRangeEditor::wheelMoved( CallBacker* )
{
    MouseEventHandler& meh = histogramdisp_->getNavigationMouseEventHandler();
    if ( meh.isHandled() ) return;

    const bool up = meh.event().angle() > 0;
    BufferStringSet nms;
    ColTab::SM().getSequenceNames( nms );
    nms.sort();
    const int curidx = nms.indexOf( ctseq_->name() );
    const int newidx = up ? curidx-1 : curidx+1;
    if ( !nms.validIdx(newidx) )
	return;

    const ColTab::Sequence newseq( nms.get(newidx) );
    setColTabSeq( newseq );
    sequenceChanged.trigger();
}
