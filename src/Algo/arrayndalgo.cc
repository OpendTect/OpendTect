/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/


#include "arrayndalgo.h"
#include "qrsolv.h"
#include "windowfunction.h"


mDefineEnumUtils( ArrayNDWindow, WindowType, "Windowing type")
{ "Box", "Hamming", "Hanning", "Blackman", "Bartlett",
  "CosTaper5", "CosTaper10", "CosTaper20", 0 };



ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& sz, bool rectangular,
			      ArrayNDWindow::WindowType type )
    : size_(sz)
    , rectangular_(rectangular)
    , window_(0)
    , paramval_(0)
{
    setType( type );
}


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& sz, bool rectangular,
			      const char* wintypenm, float paramval )
    : size_(sz)
    , rectangular_(rectangular)
    , windowtypename_(wintypenm)
    , paramval_(paramval)
    , window_(0)
{
    buildWindow( wintypenm, paramval );
}


ArrayNDWindow::~ArrayNDWindow()
{
    delete [] window_;
}


bool ArrayNDWindow::resize( const ArrayNDInfo& info )
{
    size_ = info;
    return buildWindow( windowtypename_, paramval_ );
}


bool ArrayNDWindow::setType( ArrayNDWindow::WindowType wintype )
{
    BufferString winnm = ArrayNDWindow::WindowTypeNames()[wintype];
    float paramval = mUdf(float);

    switch( wintype )
    {
	case ArrayNDWindow::CosTaper5:
            winnm = "CosTaper";
            paramval = 0.95;
            break;
	case ArrayNDWindow::CosTaper10:
            winnm = "CosTaper";
            paramval = 0.90;
            break;
	case ArrayNDWindow::CosTaper20:
            winnm = "CosTaper";
            paramval = 0.80;
            break;
        default:
            break;

    }

    return setType( winnm, paramval );
}


bool ArrayNDWindow::setType( const char* winnm, float val )
{
    if ( !buildWindow(winnm,val) )
	return false;

    windowtypename_ = winnm;
    paramval_ = val;
    return true;
}


bool ArrayNDWindow::buildWindow( const char* winnm, float val )
{
    const od_int64 totalsz = size_.getTotalSz();
    if ( totalsz <= 0 ) return false;
    window_ = new float[totalsz];
    const int ndim = size_.getNDim();
    ArrayNDIter position( size_ );

    WindowFunction* windowfunc = WINFUNCS().create( winnm );
    if ( !windowfunc ) { delete [] window_; window_ = 0; return false; }

    if ( windowfunc->hasVariable() && !windowfunc->setVariable(val) )
    { delete [] window_; window_ = 0; delete windowfunc; return false; }

    if ( !rectangular_ )
    {
	for ( unsigned long off=0; off<totalsz; off++ )
	{
	    float dist = 0;

	    for ( int idx=0; idx<ndim; idx++ )
	    {
		int sz =  size_.getSize(idx);
		int halfsz = sz / 2;
		float distval = (halfsz==0) ? 0 :
				( (float) (position[idx] - halfsz) / halfsz );
		dist += distval * distval;
	    }

	    dist = Math::Sqrt( dist );

	    window_[off] = windowfunc->getValue( dist );
	    position.next();
	}
    }
    else
    {
	for ( unsigned long off=0; off<totalsz; off++ )
	{
	    float windowval = 1;

	    for ( int idx=0; idx<ndim; idx++ )
	    {
		int sz =  size_.getSize(idx);
		int halfsz = sz / 2;
		float distval = ((float) (position[idx] - halfsz) / halfsz);
		windowval *= windowfunc->getValue( distval );
	    }

	    window_[off] = windowval;
	    position.next();
	}
    }


    delete windowfunc;
    return true;
}



mDefineEnumUtils( PolyTrend, Order, "Polynomial Order")
{ "None", "Order0", "Order1", "Order2", 0 };



PolyTrend::PolyTrend()
    : order_(None)
{}


bool PolyTrend::operator==( const PolyTrend& p ) const
{
    if ( order_ != p.order_ )
	return false;

    return mIsEqual(f0_,p.f0_,mDefEps) && mIsEqual(f1_,p.f1_,mDefEps) &&
	   mIsEqual(f2_,p.f2_,mDefEps) && mIsEqual(f11_,p.f11_,mDefEps) &&
	   mIsEqual(f12_,p.f12_,mDefEps) && mIsEqual(f22_,p.f22_,mDefEps) &&
	   posc_ == p.posc_;
}


bool PolyTrend::getOrder( int nrpoints, Order& ord, uiString* msg )
{
    if ( ord == None || ord == Order0 )
	return true;

    bool isvalid = true;
    if ( ord == Order1 && nrpoints < 3 )
    {
	if ( msg )
	    *msg =tr("De-trending with Order1 requires at least three points.");
	isvalid = false;
    }
    else if ( ord == Order2 && nrpoints < 6 )
    {
	if ( msg )
	    *msg = tr("De-trending with Order2 requires at least six points.");
	isvalid = false;
    }

    if ( isvalid )
	return true;

    if ( msg )
	msg->append( tr("Will revert to Order0 (Average removal)."), true );
    ord = Order0;

    return false;
}


void PolyTrend::initOrder0( const TypeSet<double>& vals )
{
    const int sz = vals.size();
    f0_ = 0.;
    for ( int idx=0; idx<sz; idx++ )
	f0_ += vals[idx];

    if ( sz < 2 )
	return;

    f0_ /= (double)sz;
    order_ = Order0;
}


void PolyTrend::initOrder1( const TypeSet<Coord>& pos,
			    const TypeSet<double>& vals )
{
    initCenter( pos );

    const int sz = vals.size();
    if ( sz < 3 )
	initOrder0( vals );

    Array2DImpl<double> a( sz, 3 );
    Array1DImpl<double> b( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const double dx = pos[idx].x - posc_.x;
	const double dy = pos[idx].y - posc_.y;
	a.set( idx, 0, 1. );
	a.set( idx, 1, dx );
	a.set( idx, 2, dy );
	b.set( idx, vals[idx] );
    }

    QRSolver<double> solver( a );
    if ( !solver.isFullRank() )
    {
	f1_ = f2_ = 0;
	initOrder0( vals );
	return;
    }

    const Array1DImpl<double>* f = solver.apply( b );
    f0_ = f->get( 0 );
    f1_ = f->get( 1 );
    f2_ = f->get( 2 );
    delete f;
    order_ = Order1;
}


void PolyTrend::initOrder2( const TypeSet<Coord>& pos,
			    const TypeSet<double>& vals )
{
    initCenter( pos );

    const int sz = vals.size();
    if ( sz < 6 )
	initOrder1( pos, vals );

    Array2DImpl<double> a( sz, 6 );
    Array1DImpl<double> b( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const double dx = pos[idx].x - posc_.x;
	const double dy = pos[idx].y - posc_.y;
	a.set( idx, 0, 1. );
	a.set( idx, 1, dx );
	a.set( idx, 2, dy );
	a.set( idx, 3, dx*dx );
	a.set( idx, 4, dx*dy );
	a.set( idx, 5, dy*dy );
	b.set( idx, vals[idx] );
    }

    QRSolver<double> solver( a );
    if ( !solver.isFullRank() )
    {
	f11_ = f12_ = f22_ = 0.;
	initOrder1( pos, vals );
	return;
    }

    const Array1DImpl<double>* f = solver.apply( b );
    f0_ = f->get( 0 );
    f1_ = f->get( 1 );
    f2_ = f->get( 2 );
    f11_ = f->get( 3 );
    f12_ = f->get( 4 );
    f22_ = f->get( 5 );
    delete f;
    order_ = Order2;
}


void PolyTrend::initCenter( const TypeSet<Coord>& pos )
{
    const int sz = pos.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	posc_.x += pos[idx].x;
	posc_.y += pos[idx].y;
    }

    if ( sz < 2 )
	return;

    posc_.x /= (double)sz;
    posc_.y /= (double)sz;
}


void convertUndefinedIndexList( const TrcKeyZSampling& inptkzs,
				const TrcKeyZSampling& outtkzs,
				LargeValVec<od_uint64>& undefidxs )
{
    if ( inptkzs == outtkzs )
	return;

    const Array3DInfoImpl infoin( inptkzs.hsamp_.nrLines(),
				  inptkzs.hsamp_.nrTrcs(), inptkzs.nrZ() );
    const int nrz = outtkzs.nrZ();
    int pos[3];
    for ( LargeValVec<od_uint64>::size_type idx=undefidxs.size()-1; idx>=0;
								    idx-- )
    {
	infoin.getArrayPos( undefidxs[idx], pos );
	const TrcKey tk( inptkzs.hsamp_.trcKeyAt( pos[0], pos[1] ) );
	if ( !outtkzs.hsamp_.includes(tk) )
	{
	    undefidxs.removeSingle( idx );
	    continue;
	}

	const float z = inptkzs.zsamp_.atIndex( pos[2] );
	if ( !outtkzs.zsamp_.includes(z,false) )
	{
	    undefidxs.removeSingle( idx );
	    continue;
	}

	const od_int64 hidx = outtkzs.hsamp_.globalIdx( tk );
	const int zidx = outtkzs.zsamp_.nearestIndex( z );
	undefidxs[idx] = hidx*nrz + zidx;
    }
}
