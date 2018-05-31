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
#include "uicolseqdisp.h"
#include "uipixmap.h"
#include "uistrings.h"

#include "coltabmapper.h"
#include "coltabseqmgr.h"


uiMapperRangeEditor::uiMapperRangeEditor( uiParent* p, int id, bool fixdrawrg )
    : uiHistogramSel(p,id,fixdrawrg)
    , mapper_(new ColTab::Mapper)
    , ctseq_(ColTab::SeqMGR().getDefault())
{
    initPixmaps();
}


uiMapperRangeEditor::~uiMapperRangeEditor()
{
    detachAllNotifiers();
    delete leftcoltab_; delete centercoltab_; delete rightcoltab_;
}


void uiMapperRangeEditor::setMapper( ColTab::Mapper& mpr )
{
    uiAxisHandler* axhndler = histogramdisp_->xAxis();
    if ( !axhndler )
	return;

    StepInterval<float> axrange = axhndler->range();

    const Interval<float> rg = mpr.getRange();
    if ( !rg.includes(axrange.start,true) || !rg.includes(axrange.stop,true) )
    {
	axrange.include( rg );
	histogramdisp_->setup().xrg( axrange );
	histogramdisp_->gatherInfo();
	histogramdisp_->draw();
    }

    replaceMonitoredRef( mapper_, mpr, this );
    mapper_->setup().setFixedRange( rg );
    cliprg_.start = rg.isRev() ? rg.stop : rg.start;
    cliprg_.stop = rg.isRev() ? rg.start : rg.stop;
    drawAgain();
}


void uiMapperRangeEditor::setColTabSeq( const ColTab::Sequence& cseq )
{
    replaceMonitoredRef( ctseq_, cseq, this );
    drawAgain();
}


void uiMapperRangeEditor::initPixmaps()
{
    uiGraphicsScene& scene = histogramdisp_->scene();
    const int zval = 4;

    leftcoltab_ = scene.addItem( new uiPixmapItem() );
    leftcoltab_->setZValue( zval );
    centercoltab_ = scene.addItem( new uiPixmapItem() );
    centercoltab_->setZValue( zval );
    rightcoltab_ = scene.addItem( new uiPixmapItem() );
    rightcoltab_->setZValue( zval );

    mAttachCB( mapper_->objectChanged(), uiMapperRangeEditor::mapperChg );
    mAttachCB( ctseq_->objectChanged(), uiMapperRangeEditor::colSeqChg );
}


void uiMapperRangeEditor::drawPixmaps()
{
    if ( !ctseq_ || mIsUdf(startpix_) || mIsUdf(stoppix_) )
	return;

    const int disph = histogramdisp_->height();
    const int pmh = 20;
    const int datastartpix = xax_->getPix( datarg_.start );
    const int datastoppix = xax_->getPix( datarg_.stop );

    const Interval<float> mapperrg = mapper_->getRange();
    uiPixmap leftpixmap( startpix_-datastartpix, pmh );
    leftpixmap.fill( ctseq_->color( mapperrg.width()>0 ? 0.f :1.f ) );
    leftcoltab_->setPixmap( leftpixmap );
    leftcoltab_->setOffset( datastartpix, disph-pmh-1 );

    uiPixmap* pm = ColTab::getuiPixmap( *ctseq_, stoppix_-startpix_, pmh,
					mapper_ );
    if ( pm )
	centercoltab_->setPixmap( *pm );
    centercoltab_->setOffset( startpix_, disph-pmh-1 );
    delete pm;

    uiPixmap rightpixmap( datastoppix-stoppix_, pmh );
    rightpixmap.fill( ctseq_->color( mapperrg.width()>0 ? 1.f : 0.f ) );
    rightcoltab_->setPixmap( rightpixmap );
    rightcoltab_->setOffset( stoppix_, disph-pmh-1 );
}


void uiMapperRangeEditor::drawAgain()
{
    uiHistogramSel::drawAgain();
    drawPixmaps();
}


void uiMapperRangeEditor::colSeqChg( CallBacker* cb )
{
    drawAgain();
}


void uiMapperRangeEditor::mapperChg( CallBacker* cb )
{
    const Interval<float> rg = mapper_->getRange();
    cliprg_.start = rg.isRev() ? rg.stop : rg.start;
    cliprg_.stop = rg.isRev() ? rg.start : rg.stop;
    drawAgain();
}


void uiMapperRangeEditor::useClipRange()
{
    Interval<float> newrg;
    newrg.start =
	mapper_->getRange().isRev() ? cliprg_.stop : cliprg_.start;
    newrg.stop =
	mapper_->getRange().isRev() ? cliprg_.start : cliprg_.stop;
    mapper_->setup().setFixedRange( newrg );
}
