/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Henrique Mageste
 Date:		October 2014
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "basemapgrid.h"

#include "survinfo.h"

namespace Basemap
{

GridObject::GridObject()
    : BaseMapObject(0)
    , inlxgrid_(StepInterval<double>::udf())
    , crlygrid_(StepInterval<double>::udf())
    , nrinlx_(0)
    , nrcrly_(0)
    , isinlcrl_(true)
{
}


GridObject::~GridObject()
{
}


void GridObject::updateGeometry()
{
    changed.trigger();
}


void GridObject::init( const StepInterval<double>& ix,
		       const StepInterval<double>& cy,
		       bool inlchecked, bool crlchecked,
		       const LineStyle& ls )
{
    ls_ = ls;

    if ( inlchecked )
    {
	inlxgrid_ = ix;
	nrinlx_ = inlxgrid_.nrSteps() + 1;
    }
    else
    {
	inlxgrid_.set(0,0,0);
	nrinlx_ = 0;
    }

    if ( crlchecked )
    {
	crlygrid_ = cy;
	nrcrly_ = crlygrid_.nrSteps() + 1;
    }
    else
    {
	crlygrid_.set(0,0,0);
	nrcrly_ = 0;
    }

    updateGeometry();
}


void GridObject::setInlCrlGrid( const StepInterval<double>& ix,
				const StepInterval<double>& cy,
				bool inlchecked, bool crlchecked,
				const LineStyle& ls )
{
    isinlcrl_ = true;
    init( ix, cy, inlchecked, crlchecked, ls );
}


void GridObject::setXYGrid( const StepInterval<double>& ix,
			    const StepInterval<double>& cy,
			    const Geom::PosRectangle<double>& xyarea,
			    bool xchecked, bool ychecked,
			    const LineStyle& ls )
{
    isinlcrl_ = false;
    xyarea_ = xyarea;
    init( ix, cy, xchecked, ychecked, ls );
}


int GridObject::nrShapes() const
{ return nrinlx_+nrcrly_; }


const char* GridObject::getShapeName(int) const
{ return name().buf(); }


void GridObject::getPoints( int shapeidx, TypeSet<Coord>& pts ) const
{
    const bool shapeisinlx = shapeidx < nrinlx_;

    if ( isinlcrl_ == true )
    {
	BinID pt1, pt2;
	if ( shapeisinlx )
	{
	    const StepInterval<int> crlrg = SI().crlRange(false);
	    const int inl = mCast(int,inlxgrid_.atIndex(shapeidx));
	    pt1 = BinID( inl, crlrg.start );
	    pt2 = BinID( inl, crlrg.stop  );
	}
	else
	{
	    const StepInterval<int> inlrg = SI().inlRange(false);
	    const int crly = shapeidx - nrinlx_;
	    const int crl = mCast(int,crlygrid_.atIndex(crly));
	    pt1 = BinID( inlrg.start, crl );
	    pt2 = BinID( inlrg.stop, crl );
	}
	pts.add( SI().transform(pt1) );
	pts.add( SI().transform(pt2) );
    }
    else
    {
	Coord pt1, pt2;
	if ( shapeisinlx )
	{
	    const Interval<double> yrg( xyarea_.bottom(), xyarea_.top() );
	    pt1 = Coord( inlxgrid_.atIndex(shapeidx), yrg.start );
	    pt2 = Coord( inlxgrid_.atIndex(shapeidx), yrg.stop	);
	}
	else
	{
	    const int yline = shapeidx - nrinlx_;
	    const Interval<double> xrg( xyarea_.left(), xyarea_.right() );
	    pt1 = Coord( xrg.start, crlygrid_.atIndex(yline) );
	    pt2 = Coord( xrg.stop, crlygrid_.atIndex(yline) );
	}
	pts.add( pt1 );
	pts.add( pt2 );
    }
}

} // namespace Basemap
