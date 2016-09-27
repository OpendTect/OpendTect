/*+
 ________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          September 2006
 ________________________________________________________________________

-*/

#include "annotbuffill.h"
#include "uirgbarray.h"
#include "uiworld2ui.h"


AnnotBufferFiller::AnnotBufferFiller( const uiWorld2Ui* w )
    : w2u_(w)
    , dispannotlines_(true)
    , disphorgdlines_(true)
    , dispvertgdlines_(true)
{
}


AnnotBufferFiller::~AnnotBufferFiller()
{
    deepErase( annotlines_ );
    deepErase( horgdlines_ );
    deepErase( vertgdlines_ );
}


//TODO check for intersect before starting interpol, as for now it's enough
void AnnotBufferFiller::fillBuffer( const uiWorldRect& worldareatofill,
				    uiRGBArray& buffer ) const
{
    ObjectSet<LineInfo> lines;
    if ( dispannotlines_ )
	lines.append( annotlines_ );
    if ( disphorgdlines_ )
	lines.append( horgdlines_ );
    if ( dispvertgdlines_ )
	lines.append( vertgdlines_ );
    
    for ( int idx=0; idx<lines.size(); idx++ )
	fillInterWithBufArea( worldareatofill, lines[idx], buffer );
}


//assumes that buffer size corresponds to imagebuffer.w2u_ size and pos
void AnnotBufferFiller::fillInterWithBufArea( const uiWorldRect& worldarea, 
				  const LineInfo* line, uiRGBArray& buf) const
{
    TypeSet<dPoint> pts = line->pts_;
    if ( pts.size() == 1 )
    {
	if ( worldarea.isOutside( pts[0], 1e-6 ) )
	    return;
	iPoint ipt = w2u_->transform( pts[0] );
	buf.set( ipt.x_, ipt.y_, line->linestyle_.color_ );
    }
    else
    {
	for ( int idx=1; idx<pts.size(); idx++ )
	{
	    iPoint coordsegstart = w2u_->transform( pts[idx-1] );
	    iPoint coordsegstop = w2u_->transform( pts[idx] );
	    setLine( coordsegstart, coordsegstop, line, buf );
	}
    }	
}


void AnnotBufferFiller::setLine( const iPoint& startpt, const iPoint& stoppt,
				 const LineInfo* line, uiRGBArray& buffer )const
{
    const int deltax = abs( stoppt.x_ - startpt.x_ );
    const int deltay = abs( stoppt.y_ - startpt.y_ );
    const int signx = stoppt.x_>=startpt.x_ ? 1 : -1;
    const int signy = stoppt.y_>=startpt.y_ ? 1 : -1;
    const bool ismaindirx = deltax >= deltay;
    const int nrpoints = ismaindirx ? deltax + 1 : deltay + 1;
    int discriminator = ismaindirx ? 2*deltay - deltax : 2*deltax - deltay;
    int dincnegd = ismaindirx ? 2*deltay : 2*deltax;
    int dincposd = ismaindirx ? 2*(deltay-deltax) : 2*(deltax-deltay);
    int xincnegd = ismaindirx ? signx : 0;
    int xincposd = signx;
    int yincnegd = ismaindirx ? 0 : signy;
    int yincposd = signy;

    iPoint curpt = startpt;
    //TODO later on : use line size 
    for ( int idx=0; idx<nrpoints; idx++ )
    {
	if ( curpt.x_>0 && curpt.x_<=buffer.getSize(true) &&
	     curpt.y_>0 && curpt.y_<=buffer.getSize(false) )
	    buffer.set( curpt.x_, curpt.y_, line->linestyle_.color_ );
	discriminator += discriminator<0 ? dincnegd : dincposd;
	int xcoord = curpt.x_ + (discriminator<0 ? xincnegd : xincposd);
	int ycoord = curpt.y_ + (discriminator<0 ? yincnegd : yincposd);
	curpt.setXY( xcoord, ycoord );
    }
}


bool AnnotBufferFiller::isLineOutside( const LineInfo* linfo,
				       const uiWorldRect& warea ) const
{
    bool found = false;
    for ( int idx=0; idx<linfo->pts_.size(); idx++ )
    {
	if ( !warea.isOutside( linfo->pts_[idx], 1e-6 ) )
	    found = true;
    }

    return !found;
}


dPoint AnnotBufferFiller::computeIntersect( const dPoint& pt1, 
					    const dPoint& pt2,
					    const uiWorldRect& ) const
{
    //TODO
    
   return pt1;
}


void AnnotBufferFiller::addLineInfo( const OD::LineStyle& ls,
				     TypeSet<dPoint> ptset,
				     bool ishgrid, bool isvgrid )
{
    LineInfo* linfo = new LineInfo();
    linfo->linestyle_ = ls;
    linfo->pts_ = ptset;
    if ( ishgrid )
	horgdlines_ += linfo;
    else if ( isvgrid )
	vertgdlines_ += linfo;
    else
	annotlines_ += linfo;
}


void AnnotBufferFiller::dummytest()
{
    LineInfo* linfo = new LineInfo();
    
    OD::LineStyle lstyle( OD::LineStyle::Solid, 1, Color::DgbColor() );
    linfo->linestyle_ = lstyle;
    linfo->pts_ += dPoint( 125, 0.5 );
    linfo->pts_ += dPoint( 200, 1.8 );
    annotlines_ += linfo;
}


void AnnotBufferFiller::eraseGridLines( bool hor )
{
    if ( hor )
    {
	int sz=horgdlines_.size();
	for ( int idx=sz-1; idx>=0; idx-- )
	{
	    delete horgdlines_[idx];
	    horgdlines_.removeSingle(idx);
	}
    }
    else
    {
	int sz=vertgdlines_.size();
	for ( int idx=sz-1; idx>=0; idx-- )
	{
	    delete vertgdlines_[idx];
	    vertgdlines_.removeSingle(idx);
	}
    }
}
