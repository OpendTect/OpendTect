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
    : fwdfac_(0.8)
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
	current_ += -1;
    }

    while ( viewerdata_.size()>nrviewers )
    {
	delete viewerdata_.pop();
	current_.pop();
    }

    for ( int idx=0; idx<viewerdata_.size(); idx++ )
    {
	viewerdata_[idx]->zooms_.erase();
    }

    current_.setAll( -1 );
}


int FlatView::ZoomMgr::nrZooms( int vieweridx ) const
{ return viewerdata_[vieweridx]->zooms_.size(); }


bool FlatView::ZoomMgr::atStart( int vieweridx ) const
{
    if ( vieweridx != -1 ) return current_[vieweridx] < 1;

    for ( int idx=0; idx<viewerdata_.size(); idx++ )
    {
	if ( !atStart(idx) ) return false;
    }

    return true;
}


FlatView::ZoomMgr::Size	FlatView::ZoomMgr::initialSize( int vieweridx ) const
{ return nrZooms(vieweridx) ? viewerdata_[vieweridx]->zooms_[0] : 0; }


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

    current_.setAll( 0 );
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

    const TypeSet<int> oldcur = current_;
    init( wrs );

    for ( int idx=0; idx<oldviewerdata.size(); idx++ )
    {
	const Size zoom0 = viewerdata_[idx]->zooms_[0];
	for ( int idy=1; idy<oldviewerdata[idx]->zooms_.size(); idy++ )
	{
	    const Size zoom = oldviewerdata[idx]->zooms_[idy];
	    if ( zoom.width()<zoom0.width() && zoom.height()<zoom0.height() )
	    {
		viewerdata_[idx]->zooms_ += oldviewerdata[idx]->zooms_[idy];
		if ( idy == oldcur[idx] )
    		    current_[idx] = nrZooms(idx)-1;
	    }
	}
    }

    deepErase( oldviewerdata );
}


void FlatView::ZoomMgr::add( const TypeSet<FlatView::ZoomMgr::Size>& newzooms )
{
    if ( viewerdata_.size()!=newzooms.size() )
    {
	pErrMsg("Wrong number of viewers");
	return;
    }

    if ( current_.isPresent(-1) )
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

    for ( int idx=0; idx<viewerdata_.size(); idx++ )
    {
	add( newzooms[idx], idx );
    }
}


void FlatView::ZoomMgr::add( FlatView::ZoomMgr::Size newzoom, int vieweridx )
{
    if ( vieweridx==-1 || current_.isPresent(-1) )
    {
	if ( vieweridx>0 )
	{
	    pErrMsg("ZoomMgr is not initialized");
	    return;
	}

	TypeSet<FlatView::ZoomMgr::Size> newzooms( 1, newzoom );
	add( newzooms ); return;
    }

    bool found = false;
    for ( int idx=0; idx<viewerdata_[vieweridx]->zooms_.size(); idx++ )
    {
	const Size zoom = viewerdata_[vieweridx]->zooms_[idx];
	const Size eps( newzoom.width() * 1e-6, newzoom.height() * 1e-6 );
	
	if ( mIsEqual(newzoom.width(),zoom.width(), eps.width() ) &&
	     mIsEqual(newzoom.height(),zoom.height(), eps.height() ) )
	{
	    current_[vieweridx] = idx;
	    found = true; break;
	}
    }
    
    if ( !found )
    {
	for ( int idx=viewerdata_[vieweridx]->zooms_.size()-1; idx>=0; idx-- )
	{
	    const Size zoom = viewerdata_[vieweridx]->zooms_[idx];
	    const Size eps( newzoom.width() * 1e-6, newzoom.height() * 1e-6 );
	    
	    if ( newzoom.width() > zoom.width() + eps.width()
		|| newzoom.height() > zoom.height() + eps.height() )
		viewerdata_[vieweridx]->zooms_.removeSingle(idx);
	}
	
	viewerdata_[vieweridx]->zooms_ += newzoom;
	current_[vieweridx] = nrZooms(vieweridx)-1;
    }
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::current( int vieweridx ) const
{
    const int cur = current_[vieweridx];
    return cur < 0 ? Size(1,1) : viewerdata_[vieweridx]->zooms_[cur];
}


void FlatView::ZoomMgr::back( int vieweridx ) const
{
    if ( vieweridx == -1 )
    {
	for ( int idx=0; idx<viewerdata_.size(); idx++ )
	    back( idx );
	return;
    }
    
    int& cur = current_[vieweridx];
    if ( cur > 0 ) cur--;
}


void FlatView::ZoomMgr::forward( int vieweridx ) const
{
    if ( vieweridx == -1 )
    {
	for ( int idx=0; idx<viewerdata_.size(); idx++ )
	    forward( idx );
	return;
    }

    int& cur = current_[vieweridx];
    if ( cur < 0 ) return;

    if ( cur < nrZooms(vieweridx)-1 )
    {
	cur++;
	return;
    }
    
    FlatView::ZoomMgr::Size newsize( viewerdata_[vieweridx]->zooms_[cur] );
    newsize.setWidth( newsize.width() * fwdfac_ );
    newsize.setHeight( newsize.height() * fwdfac_ );
    const_cast<FlatView::ZoomMgr*>(this)->add( newsize, vieweridx );
}


void FlatView::ZoomMgr::toStart( int vieweridx ) const
{
    if ( vieweridx != -1 )
    {
	if ( current_[vieweridx] > 0 ) current_[vieweridx] = 0;
	return;
    }

    if ( !current_.isPresent(-1) ) current_.setAll( 0 );
}
