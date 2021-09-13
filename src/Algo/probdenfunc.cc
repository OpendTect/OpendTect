/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/



#include "sampledprobdenfunc.h"
#include "probdenfuncdraw.h"
#include "idxable.h"
#include "interpol1d.h"
#include "interpol2d.h"
#include "interpolnd.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"
#include "od_iostream.h"
#include "perthreadrepos.h"
#include "statrand.h"

static const float snappos = 1e-5;
const char* ProbDenFunc::sKeyNrDim()	{ return "Nr dimensions"; }

#define mRandSDPos(sd,indx) \
    (sd).atIndex( (float)(indx) + Stats::randGen().get() - 0.5 );


ProbDenFuncDraw::~ProbDenFuncDraw()
{
    // A place for debug code, like:
    // od_cout() << "Destroy drawer ";
    // od_cout().addPtr( this ) << "for " << pdf_.name() << od_endl;
}

void ProbDenFuncDraw::reset()
{
    pdf_.prepareRandDrawing();
    usecount_.setSize( pdf_.nrDims(), 0 );
    usecount_.setAll( -1 );
    reDraw();
}


void ProbDenFuncDraw::reDraw()
{
    pdf_.drawRandomPos( vals_ );
    usecount_.setAll( 0 );
}


float ProbDenFuncDraw::value( int ival, bool redrw ) const
{
    if ( redrw && usecount_[ival] )
    {
	const_cast<ProbDenFuncDraw*>(this)->reDraw();
	if ( usecount_[ival] < 0 )
	    usecount_.setAll( 0 );
    }
    usecount_[ival]++;
    return vals_[ival];
}


// ProbDenFunc

ProbDenFunc::ProbDenFunc()
{}


ProbDenFunc::ProbDenFunc( const ProbDenFunc& pdf )
    : NamedObject(pdf.name())
{
    *this = pdf;
}


ProbDenFunc::~ProbDenFunc()
{
}


const char* ProbDenFunc::dimName( int dim ) const
{
    if ( dimnms_.validIdx(dim) )
	return dimnms_.get( dim ).buf ();

    mDeclStaticString( ret );
    ret.set( "Dim" ).add( dim );
    return ret.buf();
}


const char* ProbDenFunc::getUOMSymbol( int dim ) const
{
    return uoms_.validIdx(dim) ? uoms_.get(dim).buf() : nullptr;
}


void ProbDenFunc::setDimName( int dim, const char* nm )
{
    while ( dimnms_.size() <= dim )
	dimnms_.add( BufferString::empty() );

    dimnms_.get(dim).set( nm );
}


void ProbDenFunc::setUOMSymbol( int dim, const char* symbol )
{
    while ( uoms_.size() <= dim )
	uoms_.add( "" );

    uoms_.get(dim).set( symbol );
}


void ProbDenFunc::copyFrom( const ProbDenFunc& oth )
{
    NamedObject::operator=( oth );
    dimnms_ = oth.dimnms_;
    uoms_ = oth.uoms_;
}


bool ProbDenFunc::isEqual( const ProbDenFunc& oth ) const
{
    if ( FixedString(getTypeStr()) != oth.getTypeStr() ||
	 name() != oth.name() )
	return false;

    if ( dimnms_ != oth.dimnms_ || uoms_ != oth.uoms_ )
	return false;

    return isEq( oth );
}


void ProbDenFunc::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), getTypeStr() );
    const int nrdim = nrDims();
    par.set( sKeyNrDim(), nrdim );
    for ( int idx=0; idx<nrdim; idx++ )
    {
	par.set( IOPar::compKey(sKey::Name(),idx), dimName(idx) );
	par.set( IOPar::compKey(sKey::Unit(),idx), getUOMSymbol(idx) );
    }
}


bool ProbDenFunc::usePar( const IOPar& par )
{
    int nrdim = 0;
    par.get( sKeyNrDim(), nrdim );
    if ( nrdim < 1 )
	return false;

    for ( int idx=0; idx<nrdim; idx++ )
    {
	BufferString dimnm, uomstr;
	if ( par.get(IOPar::compKey(sKey::Name(),idx),dimnm) &&
	     !dimnm.isEmpty() )
	    setDimName( idx, dimnm );
	if ( par.get(IOPar::compKey(sKey::Unit(),idx),uomstr) )
	    setUOMSymbol( idx, uomstr );
    }

    return true;
}


void ProbDenFunc::getIndexTableFor( const ProbDenFunc& pdf,
				    TypeSet<int>& tbl ) const
{
    tbl.erase();
    for ( int idim=0; idim<nrDims(); idim++ )
	tbl += -1;

    for ( int ipdfdim=0; ipdfdim<nrDims(); ipdfdim++ )
    {
	BufferString nm( pdf.dimName(ipdfdim) );
	int& tblidx = tbl[ipdfdim];
	for ( int idim=0; idim<nrDims(); idim++ )
	{
	    if ( nm == dimName(idim) )
		{ tblidx = idim; break; }
	}
    }
}


bool ProbDenFunc::isCompatibleWith( const ProbDenFunc& pdf ) const
{
    if ( nrDims() != pdf.nrDims() ) return false;

    TypeSet<int> tbl; getIndexTableFor( pdf, tbl );
    for ( int idx=0; idx<tbl.size(); idx++ )
	if ( tbl[idx] < 0 ) return false;

    return true;
}


// ProbDenFunc1D
ProbDenFunc1D::ProbDenFunc1D( const char* vnm )
    : ProbDenFunc()
{
    setDimName( 0, vnm );
}


// ProbDenFunc2D
ProbDenFunc2D::ProbDenFunc2D( const char* vnm0, const char* vnm1 )
    : ProbDenFunc()
{
    setDimName( 0, vnm0 );
    setDimName( 1, vnm1 );
}


// ArrayNDProbDenFunc
ArrayNDProbDenFunc::ArrayNDProbDenFunc( int nrdims )
{
    for ( int idx=0; idx<nrdims; idx++ )
	sds_.add( SamplingData<float>( 0.f, 1.f ) );
}


ArrayNDProbDenFunc::~ArrayNDProbDenFunc()
{
    delete [] cumbins_;
}


ArrayNDProbDenFunc& ArrayNDProbDenFunc::operator =(
			const ArrayNDProbDenFunc& oth )
{
    if ( this != &oth )
    {
	sds_ = oth.sds_;
	deleteAndZeroArrPtr( cumbins_ );
	if ( oth.cumbins_ )
	    fillCumBins();

	avgpos_.erase();
    }

    return *this;
}


bool ArrayNDProbDenFunc::setSize( const TypeSet<int>& szs )
{
    return setSize( szs.arr(), szs.size() );
}


bool ArrayNDProbDenFunc::setSize( const int* sizes, int sz )
{
    PtrMan<ArrayNDInfo> arrinfo = ArrayNDInfoImpl::create( sizes, sz );
    return arrinfo && getData().setInfo( *arrinfo );
}


ArrayND<float>* ArrayNDProbDenFunc::getArrClone() const
{
    return ArrayNDImpl<float>::clone( getArrND() );
}


SamplingData<float>& ArrayNDProbDenFunc::sampling( int dim )
{
    const auto& sd =
		const_cast<const ArrayNDProbDenFunc&>( *this ).sampling( dim );
    return const_cast<SamplingData<float>&>( sd );
}


const SamplingData<float>& ArrayNDProbDenFunc::getSampling( int dim ) const
{
    if ( sds_.validIdx(dim) )
	return sds_.get( dim );

    static SamplingData<float> ret( 0.f, 1.f );
    return ret;
}


Interval<float> ArrayNDProbDenFunc::getRange( int dim ) const
{
    StepInterval<float> rg = sampling( dim ).interval( size(dim) );
    rg.start -= rg.step / 2.f;
    rg.stop += rg.step / 2.f;
    return Interval<float>( rg );
}


void ArrayNDProbDenFunc::setRange( int dim, const StepInterval<float>& rg )
{
    SamplingData<float>& sd = sampling( dim );
    sd.start = rg.start + rg.step / 2;
    sd.step = rg.step;
}


void ArrayNDProbDenFunc::fillPar( IOPar& par ) const
{
    const int nrdim = nrDims_();
    for ( int idx=0; idx<nrdim; idx++ )
    {
	par.set( IOPar::compKey(sKey::Size(),idx), size(idx) );
	par.set( IOPar::compKey(sKey::Sampling(),idx), sampling(idx) );
    }
}


bool ArrayNDProbDenFunc::usePar( const IOPar& par )
{
    const int nrdim = nrDims_();
    for ( int idx=0; idx<nrdim; idx++ )
    {
	SamplingData<float> sd;
	if ( par.get(IOPar::compKey(sKey::Sampling(),idx),sd) )
	    sampling( idx ) = sd;
    }

    return true;
}

#define mDefArrVars(retval,constspec) \
    constspec ArrayND<float>& array = getData(); \
    const od_int64 totalsz = array.info().getTotalSz(); \
    if ( totalsz < 1 ) \
	return retval; \
    constspec float* values = array.getData(); \
    if ( !values ) return retval

bool ArrayNDProbDenFunc::gtIsEq( const ProbDenFunc& pdf ) const
{
    mDynamicCastGet(const ArrayNDProbDenFunc&,oth,pdf)
    mDefArrVars(true,const);

    if ( totalsz != oth.getData().info().getTotalSz() )
	return false;

    const float* othvalues = oth.getData().getData();
    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	if ( !isFPEqual(values[idx],othvalues[idx],mDefEpsF) )
	    return false;
    }

    return true;
}

void ArrayNDProbDenFunc::writeBulkData( od_ostream& strm, bool binary ) const
{
    mDefArrVars(,const);
    const ArrayNDInfo& info = array.info();

    const od_int64 rowsz = info.getSize( info.getNDim()-1 );
    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	if ( binary )
	    strm.addBin( values[idx] );
	else
	{
	    strm << values[idx];
	    if ( (idx+1)%rowsz == 0 )
		strm << '\n';
	    else
		strm << '\t';
	}
    }
}


bool ArrayNDProbDenFunc::readBulkData( od_istream& strm, bool binary )
{
    mDefArrVars(false,);

    float val;
    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	if ( binary )
	    strm.getBin( val );
	else
	    strm >> val;

	values[idx] = val;
    }

    return true;
}


float ArrayNDProbDenFunc::getNormFac() const
{
    mDefArrVars(1,const);

    float sumval = 0;
    for ( od_int64 idx=0; idx<totalsz; idx++ )
	sumval += values[idx];

    return 1.f / sumval;
}


void ArrayNDProbDenFunc::doScale( float fac )
{
    mDefArrVars(,);

    for ( od_int64 idx=0; idx<totalsz; idx++ )
	values[idx] *= fac;
}


void ArrayNDProbDenFunc::prepRndDrw() const
{
    fillCumBins();
}


void ArrayNDProbDenFunc::fillCumBins() const
{
    if ( cumbins_ )
	return;

    const od_uint64 sz = totalSize();
    deleteAndZeroArrPtr( cumbins_ );
    if ( sz < 1 ) return;

    const float* vals = getData().getData();
    cumbins_ = new float[sz];
    cumbins_[0] = vals[0];
    for ( od_uint64 idx=1; idx<sz; idx++ )
	cumbins_[idx] = cumbins_[idx-1] + vals[idx];
}


od_uint64 ArrayNDProbDenFunc::getRandBin() const
{
    if ( !cumbins_ )
	fillCumBins();

    return getBinPos( (float) ( Stats::randGen().get() ) );
}


od_uint64 ArrayNDProbDenFunc::getBinPos( float pos ) const
{
    const od_int64 sz = (od_int64)totalSize();
    if ( sz < 2 ) return 0;

    const float cumpos = pos * cumbins_[sz-1];
    od_int64 ibin; static const od_int64 beforefirst = -1;
    IdxAble::findPos( cumbins_, sz, cumpos, beforefirst, ibin );
    if ( ibin < sz-1 ) // ibin == sz-1 is hugely unlikely, but still check ...
	ibin++;
    return (od_uint64)ibin;
}


float ArrayNDProbDenFunc::findAveragePos( const float* arr, int sz,
					  float grandtotal )
{
    float sum = 0, prevsum = 0;
	const float halfway = grandtotal * .5f;

    for ( int idx=0; idx<sz; idx++ )
    {
	sum += arr[idx];
	if ( sum >= halfway )
	{
	    const float frac = (sum-halfway) / (sum-prevsum);
	    return idx - frac + 0.5f;
	}
	prevsum = sum;
    }
    return sz-0.5f; // not normal
}


float ArrayNDProbDenFunc::getAveragePos( int tardim ) const
{
    if ( avgpos_.isEmpty() )
	avgpos_.setSize( getArrND().nrDims(), mUdf(float) );

    if ( avgpos_.validIdx(tardim) && !mIsUdf(avgpos_[tardim]) )
	return avgpos_[tardim];

    const int tardimsz = size( tardim );
    TypeSet<float> integrvals( tardimsz, 0 );
    const ArrayND<float>& arrnd = getData();
    const ArrayNDInfo& arrndinfo = arrnd.info();
    const od_uint64 totsz = arrndinfo.getTotalSz();
    const float* arr = arrnd.getData();
    float grandtotal = 0;
    TypeSet<int> dimidxsts( arrndinfo.getNDim(), 0 );
    int* dimidxs = dimidxsts.arr();

    for ( od_uint64 idx=0; idx<totsz; idx++ )
    {
	arrndinfo.getArrayPos( idx, dimidxs );
	const int tardimidx = dimidxs[tardim];
	integrvals[tardimidx] += arr[idx];
	grandtotal += arr[idx];
    }

    const float avgpos = findAveragePos( integrvals.arr(), integrvals.size(),
					 grandtotal );
    const float val = sampling(tardim).atIndex( avgpos );
    avgpos_[tardim] = val;
    return val;
}



// Sampled1DProbDenFunc
Sampled1DProbDenFunc::Sampled1DProbDenFunc( const char* vnm )
    : ProbDenFunc1D(vnm)
    , ArrayNDProbDenFunc(1)
    , bins_(1)
{}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const Array1D<float>& a1d )
    : ProbDenFunc1D("")
    , ArrayNDProbDenFunc(a1d.info().getNDim())
    , bins_(a1d)
{
}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const TypeSet<float>& vals )
    : ProbDenFunc1D("")
    , ArrayNDProbDenFunc(1)
    , bins_(vals.size())
{
    for ( int idx=0; idx<vals.size(); idx++ )
	bins_.set( idx, vals[idx] );
}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const float* vals, int sz )
    : ProbDenFunc1D("")
    , ArrayNDProbDenFunc(1)
    , bins_(sz)
{
    for ( int idx=0; idx<sz; idx++ )
	bins_.set( idx, vals[idx] );
}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const Sampled1DProbDenFunc& spdf )
    : ProbDenFunc1D(spdf)
    , ArrayNDProbDenFunc(spdf)
    , bins_(spdf.bins_)
{
}


Sampled1DProbDenFunc& Sampled1DProbDenFunc::operator =(
					const Sampled1DProbDenFunc& spdf )
{
    if ( this != &spdf )
    {
	ProbDenFunc1D::operator =( spdf );
	ArrayNDProbDenFunc::operator =( spdf );
	bins_ = spdf.bins_;
    }
    return *this;
}


void Sampled1DProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const Sampled1DProbDenFunc*,spdf1d,&pdf)
    if ( spdf1d )
	*this = *spdf1d;
    else
	ProbDenFunc1D::copyFrom( pdf );
}


const SamplingData<float>& Sampled1DProbDenFunc::sampling() const
{
    return ArrayNDProbDenFunc::sampling(0);
}


SamplingData<float>& Sampled1DProbDenFunc::sampling()
{
    return ArrayNDProbDenFunc::sampling(0);
}


float Sampled1DProbDenFunc::gtAvgPos() const
{
    if ( !cumbins_ )
	fillCumBins();

    const int sz = size( 0 );
    const float avgpos = findAveragePos( getData().getData(), sz,
					 cumbins_[sz-1] );
    return sampling().atIndex( avgpos );
}


float Sampled1DProbDenFunc::gtVal( float pos ) const
{
    const int sz = size( 0 );
    if ( sz < 1 ) return 0;
    const float fidx = sampling().getfIndex( pos );
    const int nidx = mNINT32(fidx);
    if ( mIsZero(nidx-fidx,snappos) )
    {
	if ( nidx < 0 || nidx > sz-1 )
	    return 0;
	return bins_.get( nidx );
    }

    const int idx = (int)Math::Floor(fidx);
    if ( idx < -1 || idx > sz-1 )
	return 0;

    float v[4];
    v[0] = idx < 1	? 0 : bins_.get( idx-1 );
    v[1] = idx < 0	? 0 : bins_.get( idx );
    v[2] = idx > sz-2	? 0 : bins_.get( idx + 1 );
    v[3] = idx > sz-3	? 0 : bins_.get( idx + 2 );

    const float val = Interpolate::PolyReg1D<float>(v).apply( fidx - idx );
    return val < 0 ? 0 : val;
}


void Sampled1DProbDenFunc::drwRandPos( float& pos ) const
{
    const od_uint64 ibin = getRandBin();
    pos = mRandSDPos( sampling(), ibin );
}


void Sampled1DProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc1D::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
}


bool Sampled1DProbDenFunc::usePar( const IOPar& par )
{
    ProbDenFunc1D::usePar( par );
    ArrayNDProbDenFunc::usePar( par );

    int sz = -1;
    par.get( IOPar::compKey(sKey::Size(),0), sz );
    bins_.setSize( sz );

    return sz>0;
}


void Sampled1DProbDenFunc::writeBulk( od_ostream& strm, bool binary ) const
{ ArrayNDProbDenFunc::writeBulkData( strm, binary ); }

bool Sampled1DProbDenFunc::readBulk( od_istream& strm, bool binary )
{ return ArrayNDProbDenFunc::readBulkData( strm, binary ); }


// Sampled2DProbDenFunc
Sampled2DProbDenFunc::Sampled2DProbDenFunc( const char* vnm0, const char* vnm1 )
    : ProbDenFunc2D(vnm0,vnm1)
    , ArrayNDProbDenFunc(2)
    , bins_(1,1)
{
}


Sampled2DProbDenFunc::Sampled2DProbDenFunc( const Array2D<float>& a2d )
    : ProbDenFunc2D("","")
    , ArrayNDProbDenFunc(a2d.info().getNDim())
    , bins_(a2d)
{
}


Sampled2DProbDenFunc::Sampled2DProbDenFunc( const Sampled2DProbDenFunc& spdf )
    : ProbDenFunc2D(spdf)
    , ArrayNDProbDenFunc(spdf)
    , bins_(spdf.bins_)
{
}


Sampled2DProbDenFunc& Sampled2DProbDenFunc::operator =(
					const Sampled2DProbDenFunc& spdf )
{
    if ( this != &spdf )
    {
	ProbDenFunc2D::operator =( spdf );
	ArrayNDProbDenFunc::operator =( spdf );
	bins_ = spdf.bins_;
    }
    return *this;
}


void Sampled2DProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const Sampled2DProbDenFunc*,spdf2d,&pdf)
    if ( spdf2d )
	*this = *spdf2d;
    else
	ProbDenFunc2D::copyFrom( pdf );
}


float Sampled2DProbDenFunc::gtVal( float px, float py ) const
{
    const int szx = size( 0 ); const int szy = size( 1 );
    if ( szx < 1 || szy < 1 ) return 0;

    const float fidxx = sampling(0).getfIndex( px );
    const float fidxy = sampling(1).getfIndex( py );
    const int nidxx = mNINT32(fidxx); const int nidxy = mNINT32(fidxy);
    if ( mIsZero(nidxx-fidxx,snappos) && mIsZero(nidxy-fidxy,snappos) )
    {
	if ( nidxx < 0 || nidxy < 0 || nidxx > szx-1 || nidxy > szy-1 )
	    return 0;
	return bins_.get( nidxx, nidxy );
    }

    const int idxx = (int)Math::Floor(fidxx);
    const int idxy = (int)Math::Floor(fidxy);
    if ( idxx < -1 || idxx > szx-1 || idxy < -1 || idxy > szy-1 )
	return 0;

    float v[4];
    v[0] = idxx < 0 || idxy < 0		? 0 : bins_.get( idxx, idxy );
    v[1] = idxy > szy-2 || idxx < 0	? 0 : bins_.get( idxx, idxy+1 );
    v[2] = idxx > szx-2 || idxy < 0	? 0 : bins_.get( idxx+1, idxy );
    v[3] = idxx > szx-2 || idxy > szy-2	? 0 : bins_.get( idxx+1, idxy+1 );

    const float xpos = fidxx - idxx; const float ypos = fidxy - idxy;
    const float val = Interpolate::LinearReg2D<float>(v).apply( xpos, ypos );
    return val < 0 ? 0 : val;
}


void Sampled2DProbDenFunc::drwRandPos( float& p0, float& p1 ) const
{
    const od_uint64 ibin = getRandBin();
    const od_uint64 dimsz1 = (od_uint64)size( 1 );
    const od_uint64 i0 = ibin / dimsz1;
    const od_uint64 i1 = ibin - i0 * dimsz1;
    p0 = mRandSDPos( sampling(0), i0 );
    p1 = mRandSDPos( sampling(1), i1 );
}


void Sampled2DProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc2D::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
}


bool Sampled2DProbDenFunc::usePar( const IOPar& par )
{
    ProbDenFunc2D::usePar( par );
    ArrayNDProbDenFunc::usePar( par );

    int sz0 = -1; int sz1 = -1;
    par.get( IOPar::compKey(sKey::Size(),0), sz0 );
    par.get( IOPar::compKey(sKey::Size(),1), sz1 );
    bins_.setSize( sz0, sz1 );

    return sz0>0 && sz1>0;
}


void Sampled2DProbDenFunc::writeBulk( od_ostream& strm, bool binary ) const
{ ArrayNDProbDenFunc::writeBulkData( strm, binary ); }

bool Sampled2DProbDenFunc::readBulk( od_istream& strm, bool binary )
{ return ArrayNDProbDenFunc::readBulkData( strm, binary ); }



// SampledNDProbDenFunc
SampledNDProbDenFunc::SampledNDProbDenFunc( int nrdims )
    : ProbDenFunc()
    , ArrayNDProbDenFunc(nrdims)
    , bins_(ArrayNDInfoImpl(nrdims))
{
}


SampledNDProbDenFunc::SampledNDProbDenFunc( const ArrayND<float>& arr )
    : ProbDenFunc()
    , ArrayNDProbDenFunc(arr.info().getNDim())
    , bins_(arr)
{
}


SampledNDProbDenFunc::SampledNDProbDenFunc( const SampledNDProbDenFunc& spdf )
    : ProbDenFunc(spdf)
    , ArrayNDProbDenFunc(spdf)
    , bins_(spdf.bins_)
{
}


SampledNDProbDenFunc::SampledNDProbDenFunc()
    : ProbDenFunc()
    , ArrayNDProbDenFunc(0)
    , bins_(ArrayNDImpl<float>(ArrayNDInfoImpl(0)))
{
}


SampledNDProbDenFunc& SampledNDProbDenFunc::operator =(
					const SampledNDProbDenFunc& spdf )
{
    if ( this != &spdf )
    {
	ProbDenFunc::operator=( spdf );
	ArrayNDProbDenFunc::operator=( spdf );
	bins_ = spdf.bins_;
    }

    return *this;
}


void SampledNDProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const SampledNDProbDenFunc*,spdfnd,&pdf)
    if ( spdfnd )
	*this = *spdfnd;
    else
	ProbDenFunc::copyFrom( pdf );
}


float SampledNDProbDenFunc::value( const TypeSet<float>& vals ) const
{
    const int nrdims = nrDims();
    if ( vals.size() < nrdims )
	return 0.f;

    TypeSet<int> szs;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	const int sz = size( idim );
	if ( sz < 1 ) return 0.f;
	szs += sz;
    }

    // Hopefully we are at a bin
    TypeSet<int> idxs(nrdims,0); bool atbin = true;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	const float fidx = sampling(idim).getfIndex( vals[idim] );
	const int nidx = mNINT32(fidx);
	if ( nidx < -1 || nidx > szs[idim] )
	    return 0;
	if ( nidx == -1 || nidx == szs[idim] || !mIsZero(nidx-fidx,snappos) )
	    { atbin = false; break; }
	idxs[idim] = nidx;
    }
    if ( atbin )
	return bins_.getND( idxs.arr() );

    // No, Need to interpolate.
    TypeSet<float> relpos( nrdims, 0 );
    for ( int idim=0; idim<nrdims; idim++ )
    {
	const float fidx = sampling(idim).getfIndex( vals[idim] );
	const int idx = (int)Math::Floor(fidx);
	if ( idx < -1 || idx > szs[idim]-1 )
	    return 0;
	relpos[idim] = fidx - idx; idxs[idim] = idx;
    }

    const od_int64 nrpts = Math::IntPowerOf( ((od_int64)2), nrdims );
    float* hcvals = new float[nrpts];
    for ( od_int64 ipt=0; ipt<nrpts; ipt++ )
    {
	TypeSet<int> curidxs( idxs );
	od_int64 curbits = ipt;
	bool isoutside = false;
	for ( int idim=0; idim<nrdims; idim++ )
	{
	    if ( curbits & 1 ) curidxs[idim]++;
	    curbits >>= 1;
	    if ( curidxs[idim] < 0 || curidxs[idim] >= szs[idim] )
		{ isoutside = true; break; }
	}
	hcvals[ipt] = isoutside ? 0 : bins_.getND( curidxs.arr() );
    }

    const float res = Interpolate::linearRegND( nrdims, hcvals, relpos.arr() );
    delete [] hcvals;
    return res;
}


void SampledNDProbDenFunc::drawRandomPos( TypeSet<float>& poss ) const
{
    const int nrdims = nrDims();
    poss.setSize( nrdims, 0 );

    od_uint64 ibin = getRandBin();
    od_uint64 dimsz = totalSize();
    for ( int idim=0; idim<nrdims; idim++ )
    {
	dimsz /= size( idim );
	const od_uint64 indx = ibin / dimsz;
	poss[idim] = mRandSDPos( sampling(idim), indx );
	ibin -= indx * dimsz;
    }
}


void SampledNDProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
    if ( nrDims() == 1 )
	par.set( sKey::Type(), Sampled1DProbDenFunc::typeStr() );
    else if ( nrDims() == 2 )
	par.set( sKey::Type(), Sampled2DProbDenFunc::typeStr() );
}


bool SampledNDProbDenFunc::usePar( const IOPar& par )
{
    int nrdims = nrDims();
    int newnrdims = nrdims;
    par.get( sKeyNrDim(), newnrdims );
    if ( newnrdims < 1 )
	return false;
    else if ( newnrdims != nrdims )
    {
	bins_.copyFrom( ArrayNDImpl<float>( ArrayNDInfoImpl(newnrdims) ) );
	nrdims = newnrdims;
    }

    TypeSet<int> szs( nrdims, 0 );
    for ( int idx=0; idx<nrdims; idx++ )
	par.get( IOPar::compKey(sKey::Size(),idx), szs[idx] );

    ProbDenFunc::usePar( par );
    ArrayNDProbDenFunc::usePar( par );

    bins_.setSize( szs.arr() );
    return bins_.info().getTotalSz() > 0;
}


void SampledNDProbDenFunc::writeBulk( od_ostream& strm, bool binary ) const
{ ArrayNDProbDenFunc::writeBulkData( strm, binary ); }

bool SampledNDProbDenFunc::readBulk( od_istream& strm, bool binary )
{ return ArrayNDProbDenFunc::readBulkData( strm, binary ); }
