/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "flatviewzoommgr.h"


void FlatView::ZoomMgr::init( const Geom::Rectangle<double>& wr )
{
    zooms_.erase();
    zooms_ += wr.size();
    center_ = wr.centre();
    cur_ = 0;
}


void FlatView::ZoomMgr::reInit( const Geom::Rectangle<double>& wr )
{
    const TypeSet<Size> oldzooms_ = zooms_;
    const int oldcur = cur_;
    init( wr );

    const Size zoom0 = zooms_[0];
    for ( int idx=1; idx<oldzooms_.size(); idx++ )
    {
	const Size zoom = oldzooms_[idx];
	if ( zoom.width() < zoom0.width() && zoom.height() < zoom0.height() )
	{
	    zooms_ += zoom;
	    if ( idx == oldcur )
		cur_ = zooms_.size() - 1;
	}
    }
}


void FlatView::ZoomMgr::add( FlatView::ZoomMgr::Size newzoom )
{
    if ( cur_ < 0 )
    {
	init( Geom::Rectangle<double>( -newzoom.width()/2,newzoom.height()/2,
				       newzoom.width()/2,-newzoom.height()/2 ));
	return;
    }

    const Size eps( newzoom.width() * 1e-6, newzoom.height() * 1e-6 );
    for ( int idx=0; idx<zooms_.size(); idx++ )
    {
	const Size zoom = zooms_[idx];
	if ( mIsEqual(newzoom.width(),zoom.width(), eps.width() ) &&
	     mIsEqual(newzoom.height(),zoom.height(), eps.height() ) )
	    { cur_ = idx; return; }
    }

    for ( int idx=zooms_.size()-1; idx>0; idx-- )
    {
	const Size zoom = zooms_[idx];
	if ( newzoom.width() > zoom.width() + eps.width()
	  || newzoom.height() > zoom.height() + eps.height() )
	    zooms_.remove( idx );
    }

    zooms_ += newzoom;
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
