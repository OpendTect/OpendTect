/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "sampledprobdenfunc.h"
#include "sampledprobdenfunc.h"
#include "probdenfuncdraw.h"
#include "interpol1d.h"
#include "interpol2d.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"
#include "interpolnd.h"
#include "idxable.h"
#include "statrand.h"
#include <math.h>
#include <iostream>

static const float snappos = 1e-5;
const char* ProbDenFunc::sKeyNrDim()	{ return "Nr dimensions"; }

#define mRandSDPos(sd,indx) \
    (sd).atIndex( (float)(indx) + Stats::RandGen::get() - 0.5 );


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


ProbDenFunc::ProbDenFunc( const ProbDenFunc& pdf )
    : NamedObject(pdf.name())
{
}


void ProbDenFunc::fillPar( IOPar& par ) const
{
    par.set( sKey::Type(), getTypeStr() );
    const int nrdim = nrDims();
    par.set( sKeyNrDim(), nrdim );
    for ( int idx=0; idx<nrdim; idx++ )
	par.set( IOPar::compKey(sKey::Name(),idx), dimName(idx) );
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


ProbDenFunc1D& ProbDenFunc1D::operator =( const ProbDenFunc1D& pd )
{
    if ( this != & pd )
	{ varnm_ = pd.varnm_; setName( pd.name() ); }
    return *this;
}


ProbDenFunc2D& ProbDenFunc2D::operator =( const ProbDenFunc2D& pd )
{
    if ( this != & pd )
	{ dim0nm_ = pd.dim0nm_; dim1nm_ = pd.dim1nm_; setName( pd.name() ); }
    return *this;
}


const char* ProbDenFunc2D::dimName( int idim ) const
{
    const BufferString* bs = idim < 1 ? &dim0nm_ : &dim1nm_;
    if ( bs->isEmpty() )
    {
	static BufferString ret;
	ret = "Dim"; ret += idim ? "1" : "0";
	bs = &ret;
    }
    return bs->buf();
}


ArrayNDProbDenFunc& ArrayNDProbDenFunc::operator =(
			const ArrayNDProbDenFunc& oth )
{
    if ( this != &oth )
    {
	delete cumbins_;
	if ( !oth.cumbins_ )
	    cumbins_ = 0;
	else
	    fillCumBins();
    }
    return *this;
}


void ArrayNDProbDenFunc::fillPar( IOPar& par ) const
{
    const int nrdim = getData().info().getNDim();
    for ( int idx=0; idx<nrdim; idx++ )
    {
	par.set( IOPar::compKey(sKey::Size(),idx), size(idx) );
	par.set( IOPar::compKey(sKey::Sampling(),idx), sampling(idx) );
    }
}

#define mDefArrVars(retval,constspec) \
    constspec ArrayND<float>& array = getData(); \
    const od_int64 totalsz = array.info().getTotalSz(); \
    if ( totalsz < 1 ) \
	return retval; \
    constspec float* values = array.getData(); \
    if ( !values ) return retval

void ArrayNDProbDenFunc::dump( std::ostream& strm, bool binary ) const
{
    mDefArrVars(,const);
    const ArrayNDInfo& info = array.info();

    const od_int64 rowsz = info.getSize( info.getNDim()-1 );
    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	if ( binary )
	    strm.write( (const char*)(&values[idx]), sizeof(float) );
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


bool ArrayNDProbDenFunc::obtain( std::istream& strm, bool binary )
{
    mDefArrVars(false,);

    float val;
    for ( od_int64 idx=0; idx<totalsz; idx++ )
    {
	if ( binary )
	    strm.read( (char*)(&val), sizeof(float) );
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
    Stats::RandGen::init();
}


void ArrayNDProbDenFunc::fillCumBins() const
{
    const od_uint64 sz = totalSize();
    delete [] cumbins_; cumbins_ = 0;
    if ( sz < 1 ) return;

    const float* vals = getData().getData();
    cumbins_ = new float [ sz ];
    cumbins_[0] = vals[0];
    for ( od_uint64 idx=1; idx<sz; idx++ )
	cumbins_[idx] = cumbins_[idx-1] + vals[idx];
}


od_uint64 ArrayNDProbDenFunc::getRandBin() const
{
    if ( !cumbins_ ) fillCumBins();
    return getBinPos( (float) ( Stats::RandGen::get() ) );
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
    return sampling(tardim).atIndex( avgpos );
}



// 1D

Sampled1DProbDenFunc::Sampled1DProbDenFunc( const Array1D<float>& a1d )
    : ProbDenFunc1D("")
    , sd_(0,1)
    , bins_(a1d)
{
}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const TypeSet<float>& vals )
    : ProbDenFunc1D("")
    , sd_(0,1)
    , bins_(vals.size())
{
    for ( int idx=0; idx<vals.size(); idx++ )
	bins_.set( idx, vals[idx] );
}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const float* vals, int sz )
    : ProbDenFunc1D("")
    , sd_(0,1)
    , bins_(sz)
{
    for ( int idx=0; idx<sz; idx++ )
	bins_.set( idx, vals[idx] );
}


Sampled1DProbDenFunc::Sampled1DProbDenFunc( const Sampled1DProbDenFunc& spdf )
    : ProbDenFunc1D(spdf)
    , ArrayNDProbDenFunc(spdf)
    , sd_(spdf.sd_)
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
	sd_ = spdf.sd_;
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


float Sampled1DProbDenFunc::gtAvgPos() const
{
    if ( !cumbins_ ) fillCumBins();
    const int sz = size( 0 );
    const float avgpos = findAveragePos( getData().getData(), sz,
	    				 cumbins_[sz-1] );
    return sd_.atIndex( avgpos );
}


float Sampled1DProbDenFunc::gtVal( float pos ) const
{
    const int sz = size( 0 );
    if ( sz < 1 ) return 0;
    const float fidx = sd_.getfIndex( pos );
    const int nidx = mNINT32(fidx);
    if ( mIsZero(nidx-fidx,snappos) )
    {
	if ( nidx < 0 || nidx > sz-1 )
	    return 0;
	return bins_.get( nidx );
    }

    const int idx = (int)floor(fidx);
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
    pos = mRandSDPos( sd_, ibin );
}


void Sampled1DProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
}


bool Sampled1DProbDenFunc::usePar( const IOPar& par )
{
    int sz = -1;
    par.get( IOPar::compKey(sKey::Size(),0), sz );
    bins_.setSize( sz );

    par.get( IOPar::compKey(sKey::Sampling(),0), sd_ );
    par.get( IOPar::compKey(sKey::Name(),0), varnm_ );
    return sz>0;
}


void Sampled1DProbDenFunc::dump( std::ostream& strm, bool binary ) const
{ ArrayNDProbDenFunc::dump( strm, binary ); }

bool Sampled1DProbDenFunc::obtain( std::istream& strm, bool binary )
{ return ArrayNDProbDenFunc::obtain( strm, binary ); }


// 2D

Sampled2DProbDenFunc::Sampled2DProbDenFunc( const Array2D<float>& a2d )
    : ProbDenFunc2D("","")
    , sd0_(0,1)
    , sd1_(0,1)
    , bins_(a2d)
{
}


Sampled2DProbDenFunc::Sampled2DProbDenFunc( const Sampled2DProbDenFunc& spdf )
    : ProbDenFunc2D(spdf)
    , sd0_(spdf.sd0_)
    , sd1_(spdf.sd1_)
    , bins_(spdf.bins_)
{
}


Sampled2DProbDenFunc& Sampled2DProbDenFunc::operator =(
					const Sampled2DProbDenFunc& spdf )
{
    if ( this != &spdf )
    {
	ProbDenFunc2D::operator =( spdf );
	sd0_ = spdf.sd0_;
	sd1_ = spdf.sd1_;
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

    const float fidxx = sd0_.getfIndex( px );
    const float fidxy = sd1_.getfIndex( py );
    const int nidxx = mNINT32(fidxx); const int nidxy = mNINT32(fidxy);
    if ( mIsZero(nidxx-fidxx,snappos) && mIsZero(nidxy-fidxy,snappos) )
    {
	if ( nidxx < 0 || nidxy < 0 || nidxx > szx-1 || nidxy > szy-1 )
	    return 0;
	return bins_.get( nidxx, nidxy );
    }

    const int idxx = (int)floor(fidxx); const int idxy = (int)floor(fidxy);
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
    p0 = mRandSDPos( sd0_, i0 );
    p1 = mRandSDPos( sd1_, i1 );
}


void Sampled2DProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    ArrayNDProbDenFunc::fillPar( par );
}


bool Sampled2DProbDenFunc::usePar( const IOPar& par )
{
    int sz0 = -1; int sz1 = -1;
    par.get( IOPar::compKey(sKey::Size(),0), sz0 );
    par.get( IOPar::compKey(sKey::Size(),1), sz1 );
    bins_.setSize( sz0, sz1 );

    par.get( IOPar::compKey(sKey::Sampling(),0), sd0_ );
    par.get( IOPar::compKey(sKey::Sampling(),1), sd1_ );

    par.get( IOPar::compKey(sKey::Name(),0), dim0nm_ );
    par.get( IOPar::compKey(sKey::Name(),1), dim1nm_ );

    return sz0>0 && sz1>0;
}


void Sampled2DProbDenFunc::dump( std::ostream& strm, bool binary ) const
{ ArrayNDProbDenFunc::dump( strm, binary ); }

bool Sampled2DProbDenFunc::obtain( std::istream& strm, bool binary )
{ return ArrayNDProbDenFunc::obtain( strm, binary ); }


// ND

SampledNDProbDenFunc::SampledNDProbDenFunc( const ArrayND<float>& arr )
    : bins_(arr)
{
    for ( int idx=0; idx<arr.info().getNDim(); idx++ )
    {
	sds_ += SamplingData<float>(0,1);
	dimnms_.add( BufferString("Dim ",idx) );
    }
}


SampledNDProbDenFunc::SampledNDProbDenFunc( const SampledNDProbDenFunc& spdf )
    : ProbDenFunc(spdf)
    , bins_(spdf.bins_)
    , sds_(spdf.sds_)
    , dimnms_(spdf.dimnms_)
{
}


SampledNDProbDenFunc::SampledNDProbDenFunc()
    : bins_(ArrayNDImpl<float>(ArrayNDInfoImpl(0)))
{
}


SampledNDProbDenFunc& SampledNDProbDenFunc::operator =(
					const SampledNDProbDenFunc& spdf )
{
    if ( this != &spdf )
    {
	setName( spdf.name() );
	sds_ = spdf.sds_;
	bins_ = spdf.bins_;
	dimnms_ = spdf.dimnms_;
    }
    return *this;
}


void SampledNDProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const SampledNDProbDenFunc*,spdfnd,&pdf)
    if ( spdfnd )
	*this = *spdfnd;
    else
    {
	setName( pdf.name() );
	for ( int idx=0; idx<nrDims(); idx++ )
	    setDimName( idx, pdf.dimName(idx) );
    }
}


const char* SampledNDProbDenFunc::dimName( int dim ) const
{
    if ( dim >= 0 && dim < dimnms_.size() )
	return dimnms_.get( dim ).buf();

    static BufferString ret; ret = "Dim";
    ret += dim;
    return ret.buf();
}


float SampledNDProbDenFunc::value( const TypeSet<float>& vals ) const
{
    const int nrdims = sds_.size();
    if ( vals.size() < nrdims )
	return 0;
    TypeSet<int> szs;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	const int sz = size( idim );
	if ( sz < 1 ) return 0;
	szs += sz;
    }

    // Hopefully we are at a bin
    TypeSet<int> idxs(nrdims,0); bool atbin = true;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	const float fidx = sds_[idim].getfIndex( vals[idim] );
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
	const float fidx = sds_[idim].getfIndex( vals[idim] );
	const int idx = (int)floor(fidx);
	if ( idx < -1 || idx > szs[idim]-1 )
	    return 0;
	relpos[idim] = fidx - idx; idxs[idim] = idx;
    }

    const od_int64 nrpts = Math::IntPowerOf( 2, nrdims );
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
	poss[idim] = mRandSDPos( sds_[idim], indx );
	ibin -= indx * dimsz;
    }
}


void SampledNDProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    if ( nrDims() == 1 )
	par.set( sKey::Type(), Sampled1DProbDenFunc::typeStr() );
    else if ( nrDims() == 2 )
	par.set( sKey::Type(), Sampled2DProbDenFunc::typeStr() );

    ArrayNDProbDenFunc::fillPar( par );
}


bool SampledNDProbDenFunc::usePar( const IOPar& par )
{
    int nrdims = nrDims();
    if ( nrdims < 1 )
    {
	par.get( sKeyNrDim(), nrdims );
	if ( nrdims < 1 )
	    return false;

	bins_.copyFrom( ArrayNDImpl<float>( ArrayNDInfoImpl(nrdims) ) );
    }

    TypeSet<int> szs( nrdims, 0 );
    for ( int idx=0; idx<nrdims; idx++ )
    {
	par.get( IOPar::compKey(sKey::Size(),idx), szs[idx] );

	SamplingData<float> sd;
	par.get( IOPar::compKey(sKey::Sampling(),idx), sd );
	sds_ += sd;

	BufferString dimnm;
	par.get( IOPar::compKey(sKey::Name(),idx), dimnm );
	dimnms_.add( dimnm );
    }

    bins_.setSize( szs.arr() );
    return bins_.info().getTotalSz() > 0;
}


void SampledNDProbDenFunc::dump( std::ostream& strm, bool binary ) const
{ ArrayNDProbDenFunc::dump( strm, binary ); }

bool SampledNDProbDenFunc::obtain( std::istream& strm, bool binary )
{ return ArrayNDProbDenFunc::obtain( strm, binary ); }

