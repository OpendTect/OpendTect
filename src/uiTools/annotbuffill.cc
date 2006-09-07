/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          September 2006
 RCS:           $Id: annotbuffill.cc,v 1.2 2006-09-07 15:15:24 cvshelene Exp $
 ________________________________________________________________________

-*/

#include "annotbuffill.h"
#include "arrayrgb.h"
#include "uiworld2ui.h"


void AnnotBufferFiller::fillBuffer( const uiWorldRect& worldareatofill,
				    ArrayRGB& buffer ) const
{
    //just for testing purpose
    const_cast<AnnotBufferFiller*>(this)->dummytest();
    
    for ( int idx=0; idx<lines_.size(); idx++ )
    {
	if ( isLineOutside(lines_[idx], worldareatofill) )
	    continue;
	else
	    fillInterWithBufArea( worldareatofill, idx, buffer );
    }
}


//assumes that buffer size corresponds to imagebuffer.w2u_ size and pos
void AnnotBufferFiller::fillInterWithBufArea( const uiWorldRect& worldarea, 
				              int lineidx, ArrayRGB& buf) const
{
    TypeSet<dPoint> pts = lines_[lineidx]->pts_;
    if ( pts.size() == 1 )
    {
	iPoint ipt = w2u_->transform( pts[0] );
	buf.set( ipt.x(), ipt.y(), lines_[lineidx]->linestyle_.color );
    }
    else
    {
	for ( int idx=1; idx<pts.size(); idx++ )
	{
	    bool isprevoutside = worldarea.isOutside( pts[idx-1] );
	    bool iscuroutside = worldarea.isOutside( pts[idx] );
	    dPoint segstart = pts[idx-1];
	    dPoint segstop = pts[idx];
	    if ( isprevoutside && iscuroutside )
		continue;
	    else if ( isprevoutside )
		segstart = computeIntersect( pts[idx-1], pts[idx], worldarea );
	    else if ( iscuroutside )
		segstop = computeIntersect( pts[idx-1], pts[idx], worldarea );

	    iPoint coordsegstart = w2u_->transform( segstart );
	    iPoint coordsegstop = w2u_->transform( segstop );
	    setLine( coordsegstart, coordsegstop, lineidx, buf );
	}
    }	
}


void AnnotBufferFiller::setLine( const iPoint& startpt, const iPoint& stoppt,
				 int lidx, ArrayRGB& buffer ) const
{
    //TODO
    const int deltax = abs( stoppt.x() - startpt.x() );
    const int deltay = abs( stoppt.y() - startpt.y() );
    const int signx = stoppt.x()>=startpt.x() ? 1 : -1;
    const int signy = stoppt.y()>=startpt.y() ? 1 : -1;
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
	buffer.set( curpt.y(), curpt.x(), lines_[lidx]->linestyle_.color );
	discriminator += discriminator<0 ? dincnegd : dincposd;
	curpt.setX( curpt.x() + (discriminator<0 ? xincnegd : xincposd) );
	curpt.setY( curpt.y() + (discriminator<0 ? yincnegd : yincposd) );
    }
}


bool AnnotBufferFiller::isLineOutside( const LineInfo* linfo,
				       const uiWorldRect& warea ) const
{
    bool found = false;
    for ( int idx=0; idx<linfo->pts_.size(); idx++ )
    {
	if ( !warea.isOutside( linfo->pts_[idx] ) )
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


