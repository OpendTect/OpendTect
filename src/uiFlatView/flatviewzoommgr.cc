/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "flatviewzoommgr.h"


FlatView::ZoomMgr::ZoomMgr()
    : cur_(-1)
    , fwdfac_(0.8)
{ setNrViewers( 1 ); }


FlatView::ZoomMgr::~ZoomMgr()
{ deepErase( viewerdata_ ); }


void FlatView::ZoomMgr::setNrViewers( int nrviewers )
{
    if ( nrviewers<1 )
    {
	pErrMsg("Number of viewers must be more than 0");
	return;
    }

    while ( viewerdata_.size()<nrviewers )
    {
	viewerdata_ += new ViewerZoomData;
    }

    while ( viewerdata_.size()>nrviewers )
    {
	delete viewerdata_.pop();
    }

    for ( int idx=0; idx<viewerdata_.size(); idx++ )
    {
	viewerdata_[idx]->zooms_.erase();
    }

    cur_ = -1;
}


int FlatView::ZoomMgr::nrZooms() const
{ return viewerdata_[0]->zooms_.size(); }


FlatView::ZoomMgr::Size	FlatView::ZoomMgr::initialSize( int vieweridx ) const
{ return nrZooms() ? viewerdata_[vieweridx]->zooms_[0] : 0; }


FlatView::ZoomMgr::Point FlatView::ZoomMgr::initialCenter( int vieweridx ) const
{ return viewerdata_[vieweridx]->center_; }


void FlatView::ZoomMgr::init( const Geom::Rectangle<double>& wr )
{
    TypeSet<Geom::Rectangle<double> > wrr( 1, wr );
    init( wrr );
}


void FlatView::ZoomMgr::init(const TypeSet<Geom::PosRectangle<double> >& wrs)
{
    TypeSet<Geom::Rectangle<double> > rects;
    for ( int idx=0; idx<wrs.size(); idx++ )
    {
	mDynamicCastGet(const Geom::Rectangle<double>&,wr,wrs[idx]);
	rects += wr;
    }

    init( rects );
}


void FlatView::ZoomMgr::init( const TypeSet<Geom::Rectangle<double> >& wrs )
{
    if ( viewerdata_.size()!=wrs.size() )
    {
	pErrMsg("Wrong number of viewers");
	return;
    }

    for ( int idx=0; idx<viewerdata_.size(); idx++ )
    {
	viewerdata_[idx]->zooms_.erase();
	viewerdata_[idx]->zooms_ += wrs[idx].size();
	viewerdata_[idx]->center_ = wrs[idx].centre();
    }

    cur_ = 0;
}


void FlatView::ZoomMgr::reInit( const Geom::Rectangle<double>& wr )
{
    TypeSet<Geom::Rectangle<double> > wrr( 1, wr );
    reInit( wrr );
}


void FlatView::ZoomMgr::reInit( const TypeSet<Geom::PosRectangle<double> >& wrs )
{
    TypeSet<Geom::Rectangle<double> > rects;
    for ( int idx=0; idx<wrs.size(); idx++ )
    {
	mDynamicCastGet(const Geom::Rectangle<double>&,wr,wrs[idx]);
	rects += wr;
    }

    reInit( rects );
}


void FlatView::ZoomMgr::reInit( const TypeSet<Geom::Rectangle<double> >& wrs )
{
    if ( viewerdata_.size()!=wrs.size() )
    {
	pErrMsg("Wrong number of viewers");
	return;
    }

    ObjectSet<ViewerZoomData> oldviewerdata;
    deepCopy( oldviewerdata, viewerdata_ );

    const int oldcur = cur_;
    init( wrs );

    for ( int idx=0; idx<oldviewerdata[0]->zooms_.size(); idx++ )
    {
	bool keep = true;
	for ( int idy=0; idy<oldviewerdata.size(); idy++ )
	{
	    const Size zoom0 = oldviewerdata[idy]->zooms_[0];
	    const Size zoom = oldviewerdata[idy]->zooms_[idx];

	    if ( zoom.width()>=zoom0.width() && zoom.height()>=zoom0.height() )
	    {
		keep = false;
		break;
	    }
	}

	if ( keep )
	{
	    for ( int idy=0; idy<oldviewerdata.size(); idy++ )
		viewerdata_[idy]->zooms_ += oldviewerdata[idy]->zooms_[idx];
	}

	if ( idx == oldcur )
	    cur_ = viewerdata_[0]->zooms_.size() - 1;
    }

    deepErase( oldviewerdata );
}


void FlatView::ZoomMgr::add( FlatView::ZoomMgr::Size newzoom )
{
    TypeSet<FlatView::ZoomMgr::Size> newzooms( 1, newzoom );
    add( newzooms );
}


void FlatView::ZoomMgr::add( const TypeSet<FlatView::ZoomMgr::Size>& newzooms )
{
    if ( viewerdata_.size()!=newzooms.size() )
    {
	pErrMsg("Wrong number of viewers");
	return;
    }

    if ( cur_ < 0 )
    {
	TypeSet<Geom::Rectangle<double> > rectangles;
	for ( int idx=0; idx<newzooms.size(); idx++ )
	{
	    rectangles += Geom::Rectangle<double>(
		-newzooms[idx].width()/2,newzooms[idx].height()/2,
		newzooms[idx].width()/2,-newzooms[idx].height()/2 );
	}

	init( rectangles );
	return;
    }

    for ( int idx=0; idx<viewerdata_[0]->zooms_.size(); idx++ )
    {
	bool identical = true;
	for ( int idy=0; idy<viewerdata_.size(); idy++ )
	{
	    const Size zoom = viewerdata_[idy]->zooms_[idx];
	    const Size newzoom = newzooms[idy];
	    const Size eps( newzoom.width() * 1e-6, newzoom.height() * 1e-6 );

	    if ( !mIsEqual(newzoom.width(),zoom.width(), eps.width() ) ||
		 !mIsEqual(newzoom.height(),zoom.height(), eps.height() ) )
	    {
		identical = false;
		break;
	    }
	}

	if ( identical )
	{
	    cur_ = idx;
	    return;
	}
    }

    for ( int idx=viewerdata_[0]->zooms_.size()-1; idx>=0; idx-- )
    {
	bool doremove = false;
	for ( int idy=0; idy<viewerdata_.size(); idy++ )
	{
	    const Size zoom = viewerdata_[idy]->zooms_[idx];
	    const Size newzoom = newzooms[idy];
	    const Size eps( newzoom.width() * 1e-6, newzoom.height() * 1e-6 );

	    if ( newzoom.width() > zoom.width() + eps.width()
		|| newzoom.height() > zoom.height() + eps.height() )
	    {
		doremove = true;
		break;
	    }
	}

	if ( doremove )
	{
	    for ( int idy=0; idy<viewerdata_.size(); idy++ )
		viewerdata_[idy]->zooms_.removeSingle(idx);
	}
    }

    for ( int idy=0; idy<viewerdata_.size(); idy++ )
	viewerdata_[idy]->zooms_ += newzooms[idy];

    cur_ = viewerdata_[0]->zooms_.size() - 1;
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::current( int vieweridx ) const
{
    return cur_ < 0 ? Size(1,1) : viewerdata_[vieweridx]->zooms_[cur_];
}


void FlatView::ZoomMgr::back() const
{
    if ( cur_ > 0 )
	cur_--;
}


void FlatView::ZoomMgr::forward() const
{
    if ( cur_ < 0 ) return;

    if ( cur_ < nrZooms()-1 )
    {
	cur_++;
	return;
    }

    TypeSet<FlatView::ZoomMgr::Size> newsizes;
    for ( int idy=0; idy<viewerdata_.size(); idy++ )
    {
	Size zoom( viewerdata_[idy]->zooms_[cur_] );
	zoom.setWidth( zoom.width() * fwdfac_ );
	zoom.setHeight( zoom.height() * fwdfac_ );
	newsizes += zoom;
    }

    const_cast<FlatView::ZoomMgr*>(this)->add( newsizes );
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::toStart() const
{
    if ( cur_ > 0 )
	cur_ = 0;
    return current();
}
