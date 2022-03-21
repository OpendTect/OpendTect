/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/



#include "gaussianprobdenfunc.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"
#include "statrand.h"
#include "separstr.h"
#include "perthreadrepos.h"
#include "array2dmatrix.h"

static const char* sKeyDimStats = "DimStats";
static const char* sKeyCorr = "Corr";


static inline float draw01Normal( const Stats::NormalRandGen& rgen )
{
    return float(rgen.get());
}

static inline float draw01Correlated( const Stats::NormalRandGen& rgen,
					float othdraw, float cc )
{
    const float draw = float(rgen.get());
    return cc * othdraw + Math::Sqrt(1 - cc*cc) * draw;
}


// 1D


Gaussian1DProbDenFunc::~Gaussian1DProbDenFunc()
{
    delete rgen_;
}


Gaussian1DProbDenFunc& Gaussian1DProbDenFunc::operator =(
					const Gaussian1DProbDenFunc& oth )
{
    if ( this != &oth )
    {
	ProbDenFunc1D::copyFrom( oth );
	exp_ = oth.exp_; std_ = oth.std_;
	delete rgen_;
	rgen_ = oth.rgen_ ? new Stats::NormalRandGen() : nullptr;
    }
    return *this;
}


void Gaussian1DProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const Gaussian1DProbDenFunc*,gpdf1d,&pdf)
    if ( gpdf1d )
	*this = *gpdf1d;
    else
	ProbDenFunc1D::copyFrom( pdf );
}


bool Gaussian1DProbDenFunc::isEq( const ProbDenFunc& oth ) const
{
    mDynamicCastGet(const Gaussian1DProbDenFunc&,gpdf1d,oth)
    return isFPEqual(exp_,gpdf1d.exp_,mDefEpsF)
	&& isFPEqual(std_,gpdf1d.std_,mDefEpsF);
}


static const float oneoversqrt2pi = (float)(1 / Math::Sqrt(2 * M_PI));

float Gaussian1DProbDenFunc::gtVal( float pos ) const
{
    const float x = (pos - exp_) / std_;
    return oneoversqrt2pi * Math::Exp( -0.5f*x*x );
}


void Gaussian1DProbDenFunc::drwRandPos( float& pos ) const
{
    if ( !rgen_ )
	rgen_ = new Stats::NormalRandGen;
    pos = rgen_->get( exp_, std_ );
}


void Gaussian1DProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    par.set( sKey::Average(), exp_ );
    par.set( sKey::StdDev(), std_ );
}


bool Gaussian1DProbDenFunc::usePar( const IOPar& par )
{
    par.get( sKey::Average(), exp_ );
    par.get( sKey::StdDev(), std_ );
    par.get( IOPar::compKey(sKey::Name(),0), varnm_ );
    deleteAndZeroPtr( rgen_ );
    readUOMFromPar( par );

    return true;
}

// 2D

Gaussian2DProbDenFunc::~Gaussian2DProbDenFunc()
{
    delete rgen0_;
    delete rgen1_;
}


Gaussian2DProbDenFunc& Gaussian2DProbDenFunc::operator =(
					const Gaussian2DProbDenFunc& oth )
{
    if ( this != &oth )
    {
	ProbDenFunc2D::copyFrom( oth );
	exp0_ = oth.exp0_; exp1_ = oth.exp1_;
	std0_ = oth.std0_; std1_ = oth.std1_;
	cc_ = oth.cc_;
	rgen0_ = oth.rgen0_ ? new Stats::NormalRandGen() : nullptr;
	rgen1_ = oth.rgen1_ ? new Stats::NormalRandGen() : nullptr;
    }
    return *this;
}


void Gaussian2DProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const Gaussian2DProbDenFunc*,gpdf2d,&pdf)
    if ( gpdf2d )
	*this = *gpdf2d;
    else
	ProbDenFunc2D::copyFrom( pdf );
}


bool Gaussian2DProbDenFunc::isEq( const ProbDenFunc& oth ) const
{
    mDynamicCastGet(const Gaussian2DProbDenFunc&,gpdf2d,oth)
    return isFPEqual(exp0_,gpdf2d.exp0_,mDefEpsF)
	&& isFPEqual(std0_,gpdf2d.std0_,mDefEpsF)
	&& isFPEqual(exp1_,gpdf2d.exp1_,mDefEpsF)
	&& isFPEqual(std1_,gpdf2d.std1_,mDefEpsF)
	&& isFPEqual(cc_,gpdf2d.cc_,mDefEpsF);
}


void Gaussian2DProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    par.set( sKey::Average(), exp0_, exp1_ );
    par.set( sKey::StdDev(), std0_, std1_ );
    par.set( "Correlation", cc_ );
}


bool Gaussian2DProbDenFunc::usePar( const IOPar& par )
{
    par.get( sKey::Average(), exp0_, exp1_ );
    par.get( sKey::StdDev(), std0_, std1_ );
    par.get( "Correlation", cc_ );
    par.get( IOPar::compKey(sKey::Name(),0), dim0nm_ );
    par.get( IOPar::compKey(sKey::Name(),1), dim1nm_ );
    deleteAndZeroPtr( rgen0_ );
    deleteAndZeroPtr( rgen1_ );
    readUOMFromPar( par );

    return true;
}


float Gaussian2DProbDenFunc::gtVal( float p0, float p1 ) const
{
    if ( cc_ > 1 - 1e-6f )
	return 0;

    const float x1 = (p0 - exp0_) / std0_;
    const float x2 = (p1 - exp1_) / std1_;
    const float onemccsq = 1 - cc_*cc_;

    const float fac = (float)(0.5 / (M_PI*Math::Sqrt(onemccsq)));
    const float epow = -0.5f * (x1*x1 + x2*x2 - 2*cc_*x1*x2) / onemccsq;
    return fac * Math::Exp( epow );
}


void Gaussian2DProbDenFunc::drwRandPos( float& p0, float& p1 ) const
{
    if ( !rgen0_ )
	rgen0_ = new Stats::NormalRandGen;
    const float x0 = draw01Normal( *rgen0_ );
    p0 = exp0_ + std0_ * x0;

    if ( !rgen1_ )
	rgen1_ = new Stats::NormalRandGen;
    const float x1 = draw01Correlated( *rgen1_, x0, cc_ );
    p1 = exp1_ + std1_ * x1;
}


// ND

GaussianNDProbDenFunc::GaussianNDProbDenFunc( int nrdims )
    : cholesky_(nullptr)
{
    for ( int idx=0; idx<nrdims; idx++ )
	vars_ += VarDef( BufferString("Dim ",idx) );
}


GaussianNDProbDenFunc::~GaussianNDProbDenFunc()
{
    deepErase( rgens_ );
    delete cholesky_;
}


GaussianNDProbDenFunc& GaussianNDProbDenFunc::operator =(
					const GaussianNDProbDenFunc& oth )
{
    if ( this != &oth )
    {
	copyNameFrom( oth );
	copyUOMFrom( oth );
	vars_ = oth.vars_;
	corrs_ = oth.corrs_;
	delete cholesky_;
	cholesky_ = oth.cholesky_ ? new Array2DMatrix<float>( *oth.cholesky_ )
				  : nullptr;
	if ( rgens_.size() != oth.rgens_.size() )
	{
	    deepErase( rgens_ );
	    for ( int idx=0; idx<oth.rgens_.size(); idx++ )
		rgens_.add( new Stats::NormalRandGen() );
	}
    }

    return *this;
}


void GaussianNDProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const GaussianNDProbDenFunc*,gpdfnd,&pdf)
    if ( gpdfnd )
	*this = *gpdfnd;
    else
    {
	copyNameFrom( pdf );
	copyUOMFrom( pdf );
	for ( int idx=0; idx<nrDims(); idx++ )
	    setDimName( idx, pdf.dimName(idx) );
    }
}


bool GaussianNDProbDenFunc::isEq( const ProbDenFunc& oth ) const
{
    mDynamicCastGet(const GaussianNDProbDenFunc&,gpdfnd,oth)

    if ( vars_.size() != gpdfnd.vars_.size()
      || corrs_.size() != gpdfnd.corrs_.size() )
	return false;

    for ( int idx=0; idx<vars_.size(); idx++ )
    {
	const VarDef& myvd = vars_[idx];
	const VarDef& othvd = gpdfnd.vars_[idx];
	if ( !(myvd == othvd) || !isFPEqual(myvd.exp_,othvd.exp_,mDefEpsF)
			      || !isFPEqual(myvd.std_,othvd.std_,mDefEpsF) )
	    return false;
    }

    for ( int idx=0; idx<corrs_.size(); idx++ )
    {
	const Corr& mycorr = corrs_[idx];
	const Corr& othcorr = gpdfnd.corrs_[idx];
	if ( !(mycorr == othcorr)
	  || !isFPEqual(mycorr.cc_,othcorr.cc_,mDefEpsF) )
	    return false;
    }

    return true;
}


const char* GaussianNDProbDenFunc::dimName( int idim ) const
{
    if ( !vars_.validIdx(idim) )
    {
	pErrMsg("bad dim");
	mDeclStaticString( ret );
	ret.set( "Dim " ).add( idim );
	return ret.buf();
    }
    return vars_[idim].name_;
}


void GaussianNDProbDenFunc::setDimName( int idim, const char* nm )
{
    if ( vars_.validIdx(idim) )
	vars_[idim].name_ = nm;
    else
	{ pErrMsg("bad dim"); }
}


float GaussianNDProbDenFunc::averagePos( int idim ) const
{
    if ( !vars_.validIdx(idim) )
	{ pErrMsg("bad dim"); return 0; }
    return vars_[idim].exp_;
}


float GaussianNDProbDenFunc::value( const TypeSet<float>& poss ) const
{
    const int nrdims = nrDims();
    if ( poss.size() < nrdims )
	{ pErrMsg("not enough positions"); return mUdf(float); }

    if ( nrdims == 1 )
    {
	const VarDef& vd = vars_[0];
	Gaussian1DProbDenFunc pdf1d( vd.exp_, vd.std_ );
	return pdf1d.value( poss[0] );
    }
    else if ( nrdims == 2 )
    {
	const VarDef& vd0 = vars_[0]; const VarDef& vd1 = vars_[1];
	Gaussian2DProbDenFunc pdf2d;
	pdf2d.exp0_ = vd0.exp_; pdf2d.std0_ = vd0.std_;
	pdf2d.exp1_ = vd1.exp_; pdf2d.std1_ = vd1.std_;
	if ( !corrs_.isEmpty() )
	    pdf2d.cc_ = corrs_[0].cc_;
	return pdf2d.value( poss[0], poss[1] );
    }
    if ( nrdims > 3 )
    {
	// This is a bogus 'boxcar'; a real implementation is not feasible
	float ret = 1;
	for ( int idim=0; idim<nrdims; idim++ )
	{
	    const float stdev = vars_[idim].std_;
	    const float val = poss[idim] - vars_[idim].exp_;
	    if ( val < -stdev || val > stdev )
		return 0;

	    ret /= 2 * stdev;
	}
    }


    // The Trivariate Gaussian:

    float r12 = 0, r13 = 0, r23 = 0;
    for ( int icorr=0; icorr<corrs_.size(); icorr++ )
    {
	const Corr& corr = corrs_[icorr];
	if ( (corr.idx0_ == 0 && corr.idx1_ == 1)
	  || (corr.idx1_ == 0 && corr.idx0_ == 1) )
	    r12 = corr.cc_;
	else if ( (corr.idx0_ == 0 && corr.idx1_ == 2)
	       || (corr.idx1_ == 0 && corr.idx0_ == 2) )
	    r13 = corr.cc_;
	else if ( (corr.idx0_ == 1 && corr.idx1_ == 2)
	       || (corr.idx1_ == 1 && corr.idx0_ == 2) )
	    r23 = corr.cc_;
    }
    if ( r12 > 1-1e-6f || r13 > 1-1e-6f || r23 > 1-1e-6f )
	return 0;

    const float x1 = (poss[0]-vars_[0].exp_) / vars_[0].std_;
    const float x2 = (poss[1]-vars_[1].exp_) / vars_[1].std_;
    const float x3 = (poss[2]-vars_[2].exp_) / vars_[2].std_;
    const float x1sq = x1*x1, x2sq = x2*x2, x3sq =x3*x3;
    const float r12sq = r12*r12, r13sq = r13*r13, r23sq =r23*r23;

    const float discr = 1 + 2*r12*r13*r23 - r12sq - r13sq - r23sq;
    float dividend =
	  x1sq*(r23sq-1) + x2sq*(r13sq-1) + x3sq*(r12sq-1)
	+ 2*x1*x2*(r12-r13*r23) + 2*x1*x3*(r13-r12*r23) + 2*x2*x3*(r23-r12*r13);

    const float gfac = (float)(1 / (M_PI * Math::Sqrt( 8 * M_PI * discr )));

    return gfac * (discr ? Math::Exp( 0.5f * dividend / discr ) : 1.0f);
}


static void fillCorrMat( Array2DMatrix<float>& mat )
{
    const int sz = mat.size();
    for ( int idx0=0; idx0<sz; idx0++ )
    {
	for ( int idx1=0; idx1<sz; idx1++ )
	{
	    float& val = mat.get( idx0, idx1 );
	    if ( !mIsUdf(val) )
		continue;

	    float corr = 1;
	    int nrcorrs = 0;
	    for ( int idx=0; idx<sz; idx++ )
	    {
#		define mAddCorr(v,i1,i2) \
		if ( i1 != i2 ) \
		{ \
		    const float v = mat.get( i1, i2 ); \
		    if ( !mIsUdf(v) ) \
			{ nrcorrs++; corr *= v; if ( nrcorrs == 2 ) break; } \
		}

		mAddCorr( v0, idx0, idx )
		mAddCorr( v1, idx, idx1 )
	    }

	    val = nrcorrs > 1 ? corr : 0;
	}
    }
}


void GaussianNDProbDenFunc::prepareRandDrawing() const
{
    GaussianNDProbDenFunc& self = *const_cast<GaussianNDProbDenFunc*>( this );
    deleteAndZeroPtr( self.cholesky_ );

    const int nrdims = nrDims();
    if ( corrs_.size() >= nrdims )
    {
	Array2DMatrix<float> corrmat( nrdims );
	corrmat.setAll( mUdf(float) );
	corrmat.setDiagonal( 1 );
	for ( int icorr=0; icorr<corrs_.size(); icorr++ )
	{
	    const Corr& corr = corrs_[icorr];
	    corrmat.set( corr.idx0_, corr.idx1_, corr.cc_ );
	    corrmat.set( corr.idx1_, corr.idx0_, corr.cc_ );
	}
	fillCorrMat( corrmat );
	self.cholesky_ = new Array2DMatrix<float>( nrdims );
	if ( !corrmat.getCholesky(*self.cholesky_) )
	    { deleteAndZeroPtr( self.cholesky_ ); }
    }
}


void GaussianNDProbDenFunc::drawRandomPos( TypeSet<float>& poss ) const
{
    const int nrdims = nrDims();
    poss.setSize( nrdims, 0 );
    if ( rgens_.isEmpty() )
    {
	for ( int idim=0; idim<nrdims; idim++ )
	    rgens_ += new Stats::NormalRandGen;
    }

    if ( cholesky_ )
    {
	Array1DVector vec( nrdims );
	for ( int idim=0; idim<nrdims; idim++ )
	    vec.set( idim, draw01Normal( *rgens_[idim]) );
	Array1DVector corrvec( nrdims );
	cholesky_->getProduct( vec, corrvec );
	for ( int idim=0; idim<nrdims; idim++ )
	    poss[idim] = corrvec.get( idim );
    }
    else
    {
	// No Cholesky ... well, let's honor as many corrs as possible ...

	BoolTypeSet drawn( nrdims, false );
	for ( int icorr=0; icorr<corrs_.size(); icorr++ )
	{
	    const Corr& corr = corrs_[ icorr ];
	    if ( drawn[corr.idx0_] && drawn[corr.idx1_] )
		continue;
	    else if ( !drawn[corr.idx0_] && !drawn[corr.idx1_] )
	    {
		poss[corr.idx0_] = draw01Normal( *rgens_[corr.idx0_] );
		drawn[corr.idx0_] = true;
	    }

	    const int idx2draw = drawn[corr.idx0_] ? corr.idx1_ : corr.idx0_;
	    const int idx2use = drawn[corr.idx0_] ? corr.idx0_ : corr.idx1_;
	    poss[idx2draw] = draw01Correlated( *rgens_[idx2draw],
					poss[idx2use], corr.cc_ );
	    drawn[idx2draw] = true;
	}

	for ( int idim=0; idim<nrdims; idim++ )
	    if ( !drawn[idim] )
		poss[idim] = draw01Normal( *rgens_[idim] );
    }

    for ( int idim=0; idim<vars_.size(); idim++ )
    {
	const VarDef& vd = vars_[idim];
	poss[idim] *= vd.std_; poss[idim] += vd.exp_;
    }
}


const char* GaussianNDProbDenFunc::firstUncorrelated() const
{
    const int nrdims = nrDims();
    BoolTypeSet havecorr( nrdims, false );
    for ( int icorr=0; icorr<corrs_.size(); icorr++ )
    {
	const Corr& corr = corrs_[icorr];
	havecorr[corr.idx0_] = havecorr[corr.idx1_] = true;
    }

    for ( int idim=0; idim<nrdims; idim++ )
	if ( !havecorr[idim] )
	    return vars_[idim].name_.buf();

    return nullptr;
}


void GaussianNDProbDenFunc::fillPar( IOPar& par ) const
{
    const int nrdims = nrDims();
    ProbDenFunc::fillPar( par );
    if ( nrdims == 1 )
    {
	Gaussian1DProbDenFunc pdf1d( vars_[0].exp_, vars_[0].std_ );
	pdf1d.setName( name() );
	pdf1d.varnm_ = vars_[0].name_;
	pdf1d.fillPar( par );
    }
    else if ( nrdims == 2 )
    {
	Gaussian2DProbDenFunc pdf2d;
	pdf2d.setName( name() );
	pdf2d.dim0nm_ = vars_[0].name_; pdf2d.dim1nm_ = vars_[1].name_;
	pdf2d.exp0_ = vars_[0].exp_; pdf2d.exp1_ = vars_[1].exp_;
	pdf2d.std0_ = vars_[0].std_; pdf2d.std1_ = vars_[1].std_;
	pdf2d.cc_ = corrs_.isEmpty() ? 0 : corrs_[0].cc_;
	pdf2d.fillPar( par );
    }

    for ( int ivar=0; ivar<vars_.size(); ivar++ )
	par.set( IOPar::compKey(sKeyDimStats,ivar), vars_[ivar].exp_,
						    vars_[ivar].std_ );
    for ( int icorr=0; icorr<corrs_.size(); icorr++ )
	par.set( IOPar::compKey(sKeyCorr,icorr), corrs_[icorr].idx0_,
				    corrs_[icorr].idx1_, corrs_[icorr].cc_ );
}


bool GaussianNDProbDenFunc::usePar( const IOPar& par )
{
    int nrdims = nrDims();
    int newnrdims = nrdims;
    par.get( sKeyNrDim(), newnrdims );
    if ( newnrdims < 1 )
	return false;

    if ( newnrdims != nrdims )
    {
	for ( int idx=nrdims; idx<newnrdims; idx++ )
	    vars_ += VarDef( BufferString("Dim ",idx) );
	while ( newnrdims < nrDims() )
	    vars_.removeSingle( vars_.size() - 1 );
	nrdims = newnrdims;
    }

    for ( int idx=0; idx<nrdims; idx++ )
    {
	par.get( IOPar::compKey(sKeyDimStats,idx), vars_[idx].exp_,
						   vars_[idx].std_ );
	par.get( IOPar::compKey(sKey::Name(),idx), vars_[idx].name_ );
    }

    readUOMFromPar( par );

    corrs_.erase();
    for ( int idx=0; ; idx++ )
    {
	const char* res = par.find( IOPar::compKey(sKeyCorr,idx) );
	if ( !res ) break;
	FileMultiString fms( res );
	Corr corr;
	corr.idx0_ = fms.getIValue( 0 );
	corr.idx1_ = fms.getIValue( 1 );
	corr.cc_ = fms.getFValue( 2 );
	corrs_ += corr;
    }

    deepErase( rgens_ );
    return true;
}
