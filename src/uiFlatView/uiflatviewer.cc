/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          Sep 2006
 RCS:           $Id: uiflatviewer.cc,v 1.1 2007-02-19 16:41:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiflatviewer.h"
#include "uirgbarraycanvas.h"
#include "uirgbarray.h"
#include "flatdispbitmapmgr.h"
#include "flatdispbmp2rgb.h"
#include "iodrawtool.h"
#include "array2dbitmapimpl.h"
#include "uimsg.h"

uiFlatViewer::uiFlatViewer( uiParent* p )
    : uiGroup(p,"Flat viewer")
    , canvas_(*new uiRGBArrayCanvas(this,*new uiRGBArray))
    , reportedchange_(All)
    , userSelection(this)
    , userselaux_(*new MouseEvent)
    , dim0extfac_(0.5)
    , wvabmpmgr_(0)
    , vdbmpmgr_(0)
    , extraborders_(0,0,0,0)
    , annotsz_(100,15) //TODO: should be dep on font size
{
    bmp2rgb_ = new FlatDisp::BitMap2RGB( context(), canvas_.rgbArray() );
    canvas_.newFillNeeded.notify( mCB(this,uiFlatViewer,canvasNewFill) );
    canvas_.postDraw.notify( mCB(this,uiFlatViewer,canvasPostDraw) );
}


uiFlatViewer::~uiFlatViewer()
{
    delete &canvas_.rgbArray();
    delete &canvas_;
    delete wvabmpmgr_;
    delete vdbmpmgr_;
    delete bmp2rgb_;
}


uiRGBArray& uiFlatViewer::rgbArray()
{
    return canvas_.rgbArray();
}


Color uiFlatViewer::color( bool foreground ) const
{
    return context().darkbg_ != foreground ? Color::White : Color::Black;
}


void uiFlatViewer::initView()
{
    setView( boundingBox() );
}


void uiFlatViewer::setExtraBorders( const uiSize& lfttp, const uiSize& rghtbt )
{
    extraborders_.setLeft( lfttp.width() );
    extraborders_.setRight( rghtbt.width() );
    extraborders_.setTop( lfttp.height() );
    extraborders_.setBottom( rghtbt.height() );
}


uiWorldRect uiFlatViewer::getBoundingBox( bool wva ) const
{
    const FlatPosData& pd = wva	? context().wvaposdata_
					: context().vdposdata_;
    StepInterval<double> rg0( pd.range(true) ); rg0.sort( true );
    StepInterval<double> rg1( pd.range(false) ); rg1.sort( true );
    rg0.start -= dim0extfac_ * rg0.step; rg0.stop += dim0extfac_ * rg0.step;
    return uiWorldRect( rg0.start, rg1.start, rg0.stop, rg1.stop );
}


uiWorldRect uiFlatViewer::boundingBox() const
{
    const FlatDisp::Context& ctxt = context();
    uiWorldRect wr1 = getBoundingBox( ctxt.ddpars_.dispwva_ );
    if ( ctxt.ddpars_.dispwva_ && ctxt.ddpars_.dispvd_ )
    {
	uiWorldRect wr2 = getBoundingBox( false );
	if ( wr1.left() > wr2.left() )
	    wr1.setLeft( wr2.left() );
	if ( wr1.right() < wr2.right() )
	    wr1.setRight( wr2.right() );
	if ( wr1.top() > wr2.top() )
	    wr1.setTop( wr2.top() );
	if ( wr1.bottom() < wr2.bottom() )
	    wr1.setBottom( wr2.bottom() );
    }

    return wr1;
}


void uiFlatViewer::setView( uiWorldRect wr )
{
    wr_ = wr;
    handleChange( All );
}


void uiFlatViewer::handleChange( DataChangeType dct )
{
    reportedchange_ = dct;
    const FlatDisp::Annotation& annot = context().annot_;
    int l = extraborders_.left(); int r = extraborders_.right();
    int t = extraborders_.top(); int b = extraborders_.bottom();
    if ( annot.haveTitle() )
	t += annotsz_.height();
    if ( annot.haveGridLines(true) )
	{ l += annotsz_.width(); r += annotsz_.width(); }
    if ( annot.haveGridLines(false) )
	{ b += annotsz_.height(); t += annotsz_.height(); }
    canvas_.setBorders( uiSize(l,t), uiSize(r,b) );
    canvas_.forceNewFill();
}


#define mDefuiSize \
    uiRGBArray& rgbarr = canvas_.rgbArray(); \
    uiSize uisz( rgbarr.getSize(true), rgbarr.getSize(false) )


void uiFlatViewer::drawBitMaps()
{
    canvas_.setBGColor( color(false) );
    //TODO: use prev bmp data and changetype to optimise
    delete wvabmpmgr_; wvabmpmgr_ = new FlatDisp::BitMapMgr(*this,true);
    delete vdbmpmgr_; vdbmpmgr_ = new FlatDisp::BitMapMgr(*this,false);

    mDefuiSize;
    if ( !wvabmpmgr_->generate(wr_,uisz) || !vdbmpmgr_->generate(wr_,uisz) )
    {
	uiMSG().error( "No memory for bitmaps" );
	delete wvabmpmgr_; wvabmpmgr_ = 0; delete vdbmpmgr_; vdbmpmgr_ = 0;
	return;
    }

    bmp2rgb_->draw( wvabmpmgr_->bitMap(), vdbmpmgr_->bitMap() );
}


#define mDefW2U \
    mDefuiSize; \
    uiWorld2Ui w2u( wr_, uisz )

void uiFlatViewer::drawAnnot()
{
    const FlatDisp::Annotation& annot = context().annot_;
    ioDrawTool& dt = *canvas_.drawTool();
    const uiRect datarect( canvas_.arrArea() );
    dt.beginDraw();

    dt.setPenColor( color(true) );
    dt.drawRect( datarect );

    if ( annot.color_ != Color::NoColor )
    {
	dt.setPenColor( annot.color_ );
	if ( annot.showx1gridlines_ )
	    drawGridAnnot( true );
	if ( annot.showx2gridlines_ )
	    drawGridAnnot( false );
    }

    for ( int idx=0; idx<annot.auxdata_.size(); idx++ )
	drawAux( *annot.auxdata_[idx] );

    if ( !annot.title_.isEmpty() )
    {
	dt.setPenColor( color(true) );
	dt.drawText( uiPoint(datarect.centre().x,2), annot.title_,
		     Alignment(Alignment::Middle,Alignment::Start) );
    }

    dt.endDraw();
    reportedchange_ = None;
}


void uiFlatViewer::drawGridAnnot( bool dir1 )
{
    const FlatDisp::Annotation& annot = context().annot_;
    pErrMsg( "TODO: implement Grid annotation" );
}


void uiFlatViewer::drawAux( const FlatDisp::Annotation::AuxData& ad )
{
    pErrMsg( "TODO: implement Aux data draw" );
}
