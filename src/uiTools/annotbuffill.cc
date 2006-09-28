/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          September 2006
 RCS:           $Id: annotbuffill.cc,v 1.5 2006-09-28 09:56:17 cvshelene Exp $
 ________________________________________________________________________

-*/

#include "annotbuffill.h"
#include "uirgbarray.h"
#include "uiworld2ui.h"

//TODO check for intersect before starting interpol, as for now it's enough
void AnnotBufferFiller::fillBuffer( const uiWorldRect& worldareatofill,
				    uiRGBArray& buffer ) const
{
    //just for testing purpose
    const_cast<AnnotBufferFiller*>(this)->dummytest();
    
    for ( int idx=0; idx<lines_.size(); idx++ )
	fillInterWithBufArea( worldareatofill, idx, buffer );
}


//assumes that buffer size corresponds to imagebuffer.w2u_ size and pos
void AnnotBufferFiller::fillInterWithBufArea( const uiWorldRect& worldarea, 
				              int lineidx, uiRGBArray& buf)const
{
    TypeSet<dPoint> pts = lines_[lineidx]->pts_;
    if ( pts.size() == 1 )
    {
	if ( worldarea.isOutside( pts[0], 1e-6 ) )
	    return;
	iPoint ipt = w2u_->transform( pts[0] );
	buf.set( ipt.x, ipt.y, lines_[lineidx]->linestyle_.color );
    }
    else
    {
	for ( int idx=1; idx<pts.size(); idx++ )
	{
	    iPoint coordsegstart = w2u_->transform( pts[idx-1] );
	    iPoint coordsegstop = w2u_->transform( pts[idx] );
	    setLine( coordsegstart, coordsegstop, lineidx, buf );
	}
    }	
}


void AnnotBufferFiller::setLine( const iPoint& startpt, const iPoint& stoppt,
				 int lidx, uiRGBArray& buffer ) const
{
    const int deltax = abs( stoppt.x - startpt.x );
    const int deltay = abs( stoppt.y - startpt.y );
    const int signx = stoppt.x>=startpt.x ? 1 : -1;
    const int signy = stoppt.y>=startpt.y ? 1 : -1;
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
	if ( curpt.x>0 && curpt.x<=buffer.getSize(true) && 
	     curpt.y>0 && curpt.y<=buffer.getSize(false) )
	    buffer.set( curpt.x, curpt.y, lines_[lidx]->linestyle_.color );
	discriminator += discriminator<0 ? dincnegd : dincposd;
	int xcoord = curpt.x + (discriminator<0 ? xincnegd : xincposd);
	int ycoord = curpt.y + (discriminator<0 ? yincnegd : yincposd);
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


void AnnotBufferFiller::dummytest()
{
    LineInfo* linfo = new LineInfo();
    
    LineStyle lstyle( LineStyle::Solid, 1, Color::DgbColor );
    linfo->linestyle_ = lstyle;
    linfo->pts_ += dPoint( 125, 0.5 );
    linfo->pts_ += dPoint( 200, 1.8 );
    lines_ += linfo;
}


