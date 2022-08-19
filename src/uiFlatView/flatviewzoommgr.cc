/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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


void FlatView::ZoomMgr::setFwdFac( double fac )
{
    if ( mIsZero(fac,mDefEps) || fac<0 || fac>=1 )
	{ pErrMsg("Fwdfac should be greater than 0 and less than 1"); return; }

    fwdfac_ = fac;
}


void FlatView::ZoomMgr::init( const Geom::PosRectangle<double>& wr )
{
    TypeSet<Geom::PosRectangle<double> > wrr( 1, wr );
    init( wrr );
}


void FlatView::ZoomMgr::init( const TypeSet<Geom::PosRectangle<double> >& wrs )
{
    if ( viewerdata_.size()!=wrs.size() )
        { pErrMsg("Wrong number of viewers"); return; }

    for ( int idx=0; idx<viewerdata_.size(); idx++ )
    {
	viewerdata_[idx]->zooms_.erase();
	viewerdata_[idx]->zooms_ += wrs[idx].size();
	viewerdata_[idx]->center_ = wrs[idx].centre();
    }

    current_.setAll( 0 );
}


void FlatView::ZoomMgr::reInit( const Geom::PosRectangle<double>& wr )
{
    TypeSet<Geom::PosRectangle<double> > wrr( 1, wr );
    reInit( wrr );
}


void FlatView::ZoomMgr::reInit(const TypeSet<Geom::PosRectangle<double> >& wrs)
{
    if ( viewerdata_.size()!=wrs.size() )
        { pErrMsg("Wrong number of viewers"); return; }

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


void FlatView::ZoomMgr::add( FlatView::ZoomMgr::Size newzoom, int vieweridx )
{
    if ( current_.isPresent(-1) )
    	return;
    const Size zoom0 = viewerdata_[vieweridx]->zooms_[0];
    if ( newzoom.width() > zoom0.width() )
	newzoom.setWidth( zoom0.width() );
    if ( newzoom.height() > zoom0.height() )
	newzoom.setHeight( zoom0.height() );

    const Size eps( newzoom.width() * 1e-6, newzoom.height() * 1e-6 );
    bool found = false;
    for ( int idx=0; idx<viewerdata_[vieweridx]->zooms_.size(); idx++ )
    {
	const Size zoom = viewerdata_[vieweridx]->zooms_[idx];
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


FlatView::ZoomMgr::Size FlatView::ZoomMgr::back( int vieweridx,
						 bool onlyvertical,
						 bool usefwdfac ) const
{
    int& cur = current_[vieweridx];
    if ( cur>0 && !usefwdfac && !onlyvertical )
	{ cur--; return current( vieweridx ); }

    FlatView::ZoomMgr::Size newsize( viewerdata_[vieweridx]->zooms_[cur] );
    if ( !onlyvertical )
	newsize.setWidth( newsize.width() / fwdfac_ );
    newsize.setHeight( newsize.height() / fwdfac_ );
    const_cast<FlatView::ZoomMgr*>(this)->add( newsize, vieweridx );
    // NOTE: newsize returned can be greater than the initialSize. This is not
    // added to zooms_ in such a case, but is needed to maintain constant
    // aspect ratio while zooming out.
    return newsize;
}


FlatView::ZoomMgr::Size FlatView::ZoomMgr::forward( int vieweridx,
						    bool onlyvertical,
						    bool usefwdfac ) const
{
    int& cur = current_[vieweridx];
    if ( cur < 0 ) return current( vieweridx );

    if ( !usefwdfac && cur<nrZooms(vieweridx)-1 && !onlyvertical )
	{ cur++; return current( vieweridx ); }
    
    FlatView::ZoomMgr::Size newsize( viewerdata_[vieweridx]->zooms_[cur] );
    if ( !onlyvertical )
	newsize.setWidth( newsize.width() * fwdfac_ );
    newsize.setHeight( newsize.height() * fwdfac_ );
    const_cast<FlatView::ZoomMgr*>(this)->add( newsize, vieweridx );
    return current( vieweridx );
}


void FlatView::ZoomMgr::toStart( int vieweridx ) const
{
    if ( vieweridx != -1 )
    {
	if ( current_[vieweridx] > 0 ) current_[vieweridx] = 0;
	return;
    }

    if ( !current_.isPresent(-1) )
	current_.setAll( 0 );
}
