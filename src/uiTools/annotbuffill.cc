/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        H. Huck
 Date:          September 2006
 RCS:           $Id: annotbuffill.cc,v 1.1 2006-09-05 15:22:15 cvshelene Exp $
 ________________________________________________________________________

-*/

#include "annotbuffill.h"
#include "uiworld2ui.h"


AnnotBufferFiller::fillBuffer( const uiRect& areatofill, ArrayRGB& buffer) const
{
    uiWorldRect worldareatofill = w2u_.transform( areatofill );
    for ( idx=0; idx<lines_.size(); idx++ )
    {
	if ( isLineOutside(lines_[idx], worldareatofill) )
	    continue;
	else
	    fillIntersectWithBuffer( worldareatofill, idx, buffer );
    }
}


//assumes that buffer size corresponds to imagebuffer.w2u_ size and pos
AnnotBufferFiller::fillIntersectWithBuffer( const uiWorldRect& worldarea, 
					    int lineidx, ArrayRGB& buf ) const
{
    TypeSet<Point> pts = lines_[lineidx]->pts_;
    if ( pts.size() == 1 )
	setPoint( pts[0], lineidx, buf );
    else
    {
	for ( int idx=1; idx<pts.size(); idx++ )
	{
	    bool isprevoutside = worldarea.isOutside( pts[idx-1] );
	    bool iscuroutside = worldarea.isOutside( pts[idx] );
	    Point segstart = pts[idx-1];
	    Point segstop = pts[idx];
	    if ( isprevoutside && iscuroutside )
		continue;
	    else if ( isprevoutside )
		segstart = computeIntersect( pts[idx-1], pts[idx], worldarea );
	    else if ( iscuroutside )
		segstop = computeIntersect( pts[idx-1], pts[idx], worldarea );

	    iPoint coordsegstart = w2u_.transform( segstart );
	    iPoint coordsegstop = w2u_.transform( segstop );
	    setLine( coordsegstart, coordsegstop, lineidx, buf );
	}
    }	
}


void AnnotBufferFiller::setPoint( const iPoint& coordpt, int lidx,
				  ArrayRGB& buffer ) const
{
    //TODO
}


void AnnotBufferFiller::setLine( const iPoint& startpt, const iPoint& stoppt,
				 int lidx, ArrayRGB& buffer ) const
{
    //TODO
    const int deltax = abs( istoppt.x() - istartpt.x() );
    const int deltay = abs( istoppt.y() - istartpt.y() );
    const int signx = istoppt.x()>istartpt.x() ? 1 : -1;
    const int signy = istoppt.y()>istartpt.y() ? 1 : -1;
    const bool ismaindirx = deltax >= deltay;
    const int nrpoints = ismaindirx ? deltax + 1 : deltay + 1;
    int discriminator = ismaindirx ? 2*deltay - deltax : 2*deltax - deltay;
    int dincnegd = ismaindirx ? 2*deltay : 2*deltax;
    int dincposd = ismaindirx ? 2*(deltay-deltax) : 2*(deltax-deltay);
    int xincnegd = ismaindirx ? signx : 0;
    int xincposd = signx;
    int yincnegd = ismaindirx ? 0 : signy;
    int yincposd = signy;

    iPoint curpt = istartpt;
    //TODO later on : use line size 
    for ( int idx=0; idx<nrpoints; idx++ )
    {
	setPoint( curpt, lines_[lidx].linestyle_.color );
	discriminator += discriminator<0 ? dincnegd : dincposd;
	curpt.x() += discriminator<0 ? xincnegd : xincposd;
	curpt.y() += discriminator<0 ? yincnegd : yincposd;
    }
}


bool AnnotBufferFiller::isLineOutside( const LineInfo& linfo,
				       const uiWorldRect& warea ) const
{
    bool found = false;
    for ( int idx=0; idx<linfo.pts_.size(); idx++ )
    {
	if ( !warea.isOutside( linfo.pts_[idx] ) )
	    found = true;
    }

    return found;
}


dPoint AnnotBufferFiller::computeIntersect( const dPoint&, 
					    const uiWorldRect& ) const
{
    //TODO
    
    
}


