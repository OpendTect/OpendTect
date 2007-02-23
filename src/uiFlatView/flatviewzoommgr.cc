/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:		Feb 2007
 RCS:           $Id: flatviewzoommgr.cc,v 1.1 2007-02-23 09:35:33 cvsbert Exp $
________________________________________________________________________

-*/

#include "flatviewzoommgr.h"


void FlatView::ZoomMgr::init( const Geom::Rectangle<double>& wr )
{
    zooms_.erase();
    zooms_ += wr.size();
    center_ = wr.centre();
    cur_ = 0;
}


void FlatView::ZoomMgr::add( FlatView::ZoomMgr::Size zoom )
{
    if ( cur_ < 0 )
    {
	init( Geom::Rectangle<double>(	-zoom.width()/2,zoom.height()/2,
					zoom.width()/2,-zoom.height()/2 ) );
	return;
    }

    const int oldsz = zooms_.size();
    if ( oldsz > 1 && cur_ != oldsz-1 )
    {
	while ( zooms_.size() >= cur_ )
	    zooms_.remove( zooms_.size()-1 );
    }

    zooms_ += zoom;
    cur_ = zooms_.size() - 1;
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::current() const
{
    return cur_ < 0 ? Size(1,1) : zooms_[cur_];
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::back() const
{
    if ( cur_ > 0 )
	cur_--;
    return current();
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::forward() const
{
    if ( cur_ < 0 ) return Size(1,1);

    if ( cur_ < zooms_.size()-1 )
    {
	cur_++;
	return current();
    }

    Size zoom( zooms_[cur_] );
    zoom.setWidth( zoom.width() * fwdfac_ );
    zoom.setHeight( zoom.height() * fwdfac_ );
    const_cast<FlatView::ZoomMgr*>(this)->add( zoom );
    return zoom;
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::toStart() const
{
    if ( cur_ > 0 )
	cur_ = 0;
    return current();
}
