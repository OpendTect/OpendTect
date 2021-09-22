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

template<>
void EnumDefImpl<ArrayNDWindow::WindowType>::init()
{
    uistrings_ += mEnumTr("Box",0);
    uistrings_ += mEnumTr("Hamming",0);
    uistrings_ += mEnumTr("Hanning",0);
    uistrings_ += mEnumTr("Blackman",0);
    uistrings_ += mEnumTr("Bartlett",0);
    uistrings_ += mEnumTr("CosTaper5",0);
    uistrings_ += mEnumTr("CosTaper10",0);
    uistrings_ += mEnumTr("CosTaper20",0);
}


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& inf, bool rectangular,
			      ArrayNDWindow::WindowType type )
    : arrinfo_(inf)
    , rectangular_(rectangular)
    , window_(0)
{
    setType( type );
}


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& inf, bool rectangular,
			      const char* wintypenm, float paramval )
    : arrinfo_(inf)
    , rectangular_(rectangular)
    , windowtypename_(wintypenm)
    , window_(0)
{
    buildWindow( wintypenm, paramval );
}


ArrayNDWindow::ArrayNDWindow( const ArrayNDInfo& inf, bool rectangular,
			      const char* wintypenm,
			      const TypeSet<float>& paramval )
    : arrinfo_(inf)
    , rectangular_(rectangular)
    , windowtypename_(wintypenm)
    , paramval_(paramval)
    , window_(0)
{
    buildWindow( wintypenm );
}


ArrayNDWindow::~ArrayNDWindow()
{
    delete [] window_;
}


bool ArrayNDWindow::resize( const ArrayNDInfo& info )
{
    arrinfo_ = info;
    paramval_.setEmpty();
    paramval_.setSize( arrinfo_.nrDims(), paramval_[0] );
    return buildWindow( windowtypename_ );
}


bool ArrayNDWindow::setType( ArrayNDWindow::WindowType wintype )
{
    BufferString winnm = ArrayNDWindow::WindowTypeDef().getKey(wintype);
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


void ArrayNDWindow::setParamVal( int dim, float paramval )
{
    if ( !paramval_.validIdx(dim) )
	return;

    paramval_[dim] = paramval;
    buildWindow( windowtypename_ );
}


void ArrayNDWindow::setParamVals( const TypeSet<float>& paramvals )
{
    if ( arrinfo_.nrDims()==paramvals.size() )
    {
	paramval_ = paramvals;
	buildWindow( windowtypename_ );
    }
}


bool ArrayNDWindow::setType( const char* winnm, float val )
{
    if ( !buildWindow(winnm,val) )
	return false;

    windowtypename_ = winnm;
    return true;
}


bool ArrayNDWindow::setType( const char* winnm, const TypeSet<float>& paramvals)
{
    paramval_ = paramvals;
    if ( !buildWindow(winnm) )
	return false;

    windowtypename_ = winnm;
    return true;
}


bool ArrayNDWindow::buildWindow( const char* winnm, float val )
{
    paramval_.setEmpty();
    paramval_.setSize( arrinfo_.nrDims(), val );
    return buildWindow( winnm );
}


bool ArrayNDWindow::buildWindow( const char* winnm )
{
    const auto totalsz = arrinfo_.totalSize();
    if ( totalsz <= 0 )
	return false;
    window_ = new float[totalsz];
    const auto ndim = arrinfo_.nrDims();
    ArrayNDIter position( arrinfo_ );

    WindowFunction* windowfunc = WindowFunction::factory().create( winnm );
    if ( !windowfunc )
	{ delete [] window_; window_ = 0; return false; }

    const bool hasvar = windowfunc->hasVariable();
    if ( hasvar && !windowfunc->setVariable(paramval_[0]) )
	{ delete [] window_; window_ = 0; delete windowfunc; return false; }

    if ( !rectangular_ )
    {
	for ( total_size_type off=0; off<totalsz; off++ )
	{
	    float dist = 0;

	    for ( dim_idx_type idx=0; idx<ndim; idx++ )
	    {
		if ( hasvar )
		    windowfunc->setVariable( paramval_.validIdx(idx) ?
					    paramval_[idx] : paramval_.last() );
		const float halfsz = float(arrinfo_.getSize(idx)-1) / 2.f;
		const float distval = mIsZero(halfsz, mDefEpsF) ? 0.f :
				(float(position[idx]) - halfsz) / halfsz;
		dist += distval * distval;
	    }

	    dist = Math::Sqrt( dist );
	    window_[off] = windowfunc->getValue( dist );
	    position.next();
	}
    }
    else
    {
	for ( total_size_type off=0; off<totalsz; off++ )
	{
	    float windowval = 1;

	    for ( dim_idx_type idx=0; idx<ndim; idx++ )
	    {
		if ( hasvar )
		    windowfunc->setVariable( paramval_.validIdx(idx) ?
					    paramval_[idx] : paramval_.last() );
		const float halfsz = float(arrinfo_.getSize(idx)-1) / 2.f;
		const float distval = mIsZero(halfsz, mDefEpsF) ? 0.f :
				(float(position[idx]) - halfsz) / halfsz;
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

template<>
void EnumDefImpl<PolyTrend::Order>::init()
{
    uistrings_ += uiStrings::sNone();
    uistrings_ += mEnumTr("Order0",0);
    uistrings_ += mEnumTr("Order1",0);
    uistrings_ += mEnumTr("Order2",0);
}

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
	msg->appendPhrase( tr("Will revert to Order0 (Average removal)") );

    ord = Order0;
    return false;
}


void PolyTrend::initOrder0( const TypeSet<double>& vals )
{
    const auto sz = vals.size();
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

    const auto sz = vals.size();
    if ( sz < 3 )
	initOrder0( vals );

    Array2DImpl<double> a( sz, 3 );
    Array1DImpl<double> b( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const double dx = pos[idx].x_ - posc_.x_;
	const double dy = pos[idx].y_ - posc_.y_;
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

    const auto sz = vals.size();
    if ( sz < 6 )
	initOrder1( pos, vals );

    Array2DImpl<double> a( sz, 6 );
    Array1DImpl<double> b( sz );
    for ( int idx=0; idx<sz; idx++ )
    {
	const double dx = pos[idx].x_ - posc_.x_;
	const double dy = pos[idx].y_ - posc_.y_;
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
    const auto sz = pos.size();
    for ( int idx=0; idx<sz; idx++ )
    {
	posc_.x_ += pos[idx].x_;
	posc_.y_ += pos[idx].y_;
    }

    if ( sz < 2 )
	return;

    posc_.x_ /= (double)sz;
    posc_.y_ /= (double)sz;
}
