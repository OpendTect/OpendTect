/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jan 2010
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "gaussianprobdenfunc.h"
#include "iopar.h"
#include "keystrs.h"
#include "math2.h"
#include "statrand.h"
#include "separstr.h"
#include "perthreadrepos.h"
#include "od_iostream.h"

static const char* sKeyDimStats = "DimStats";
static const char* sKeyCorr = "Corr";


// 1D


void Gaussian1DProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const Gaussian1DProbDenFunc*,gpdf1d,&pdf)
    if ( gpdf1d )
	*this = *gpdf1d;
    else
	ProbDenFunc1D::copyFrom( pdf );
}


float Gaussian1DProbDenFunc::gtVal( float pos ) const
{
    const float gfac = (float)(1 / (std_ * Math::Sqrt(2 * M_PI)));
    pos -= exp_; pos /= std_;
    const float epow = -0.5f * pos * pos;
    return gfac * Math::Exp( epow );
}


void Gaussian1DProbDenFunc::drwRandPos( float& pos ) const
{
    pos = (float)Stats::randGen().getNormal( exp_, std_ );
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
    return true;
}


void Gaussian1DProbDenFunc::dump( od_ostream& strm, bool binary ) const
{
    if ( !binary )
	strm << exp_ << od_tab << std_ << od_endl;
    else
    {
	strm.addBin( &exp_, sizeof(float) );
	strm.addBin( &std_, sizeof(float) );
    }
}

bool Gaussian1DProbDenFunc::obtain( od_istream& strm, bool binary )
{
    if ( !binary )
	strm >> exp_ >> std_;
    else
    {
	strm.getBin( &exp_, sizeof(float) );
	strm.getBin( &std_, sizeof(float) );
    }
    return !strm.isBad();
}


inline static float toDistribPos( float v, float exp, float sd )
{
    v -= exp; v /= sd; return v;
}


inline static float fromDistribPos( float v, float exp, float sd )
{
    v *= sd; v += exp; return v;
}


// 2D


void Gaussian2DProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const Gaussian2DProbDenFunc*,gpdf2d,&pdf)
    if ( gpdf2d )
	*this = *gpdf2d;
    else
	ProbDenFunc2D::copyFrom( pdf );
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
    return true;
}


void Gaussian2DProbDenFunc::dump( od_ostream& strm, bool binary ) const
{
    if ( !binary )
	strm << exp0_ << od_tab << exp1_ << od_tab
	     << std0_ << od_tab << std1_ << od_tab
	     << cc_ << od_endl;
    else
    {
	strm.addBin( &exp0_, sizeof(float) );
	strm.addBin( &exp1_, sizeof(float) );
	strm.addBin( &std0_, sizeof(float) );
	strm.addBin( &std1_, sizeof(float) );
	strm.addBin( &cc_, sizeof(float) );
    }
}

bool Gaussian2DProbDenFunc::obtain( od_istream& strm, bool binary )
{
    if ( binary )
	strm >> exp0_ >> exp1_ >> std0_ >> std1_ >> cc_;
    else
    {
	strm.getBin( &exp0_, sizeof(float) );
	strm.getBin( &exp1_, sizeof(float) );
	strm.getBin( &std0_, sizeof(float) );
	strm.getBin( &std1_, sizeof(float) );
	strm.getBin( &cc_, sizeof(float) );
    }
    return !strm.isBad();
}



float Gaussian2DProbDenFunc::gtVal( float p0, float p1 ) const
{
    if ( cc_ > 1 - 1e-6f )
	return 0;

    const float gfac = (float)(1 / (2*M_PI*std0_*std1_*Math::Sqrt(1-cc_*cc_)));
    const float efac = -1 / (2 * ( 1 - cc_*cc_ ));
    p0 -= exp0_; p0 /= std0_;
    p1 -= exp1_; p1 /= std1_;
    const float epow = efac * ( p0*p0 + p1*p1 + 2*cc_*p0*p1 );
    return gfac * Math::Exp( epow );
}


void Gaussian2DProbDenFunc::drwRandPos( float& p0, float& p1 ) const
{
    const float x0 = (float)Stats::randGen().getNormal( 0, 1 );
    p0 = exp0_ + std0_ * x0;

    const float x1 = (float)Stats::randGen().getNormal( 0, 1 );
    const float x1corr = cc_ * x0 + Math::Sqrt(1 - cc_*cc_) * x1;
    p1 = exp1_ + std1_ * x1corr;
}


// ND

GaussianNDProbDenFunc::GaussianNDProbDenFunc( int nrdims )
{
    for ( int idx=0; idx<nrdims; idx++ )
	vars_ += VarDef( BufferString("Dim ",idx) );
}


GaussianNDProbDenFunc::~GaussianNDProbDenFunc()
{
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


void GaussianNDProbDenFunc::copyFrom( const ProbDenFunc& pdf )
{
    mDynamicCastGet(const GaussianNDProbDenFunc*,gpdfnd,&pdf)
    if ( gpdfnd )
	*this = *gpdfnd;
    else
    {
	setName( pdf.name() );
	for ( int idx=0; idx<nrDims(); idx++ )
	    setDimName( idx, pdf.dimName(idx) );
    }
}


float GaussianNDProbDenFunc::value( const TypeSet<float>& vals ) const
{
    // This is a bogus 'boxcar'; a real implementation is not feasible
    const int nrdims = nrDims();
    float ret = 1;
    for ( int idim=0; idim<nrdims; idim++ )
    {
	const float stdev = vars_[idim].std_;
	const float val = vals[idim] - vars_[idim].exp_;
	if ( val < -stdev || val > stdev )
	    return 0;

	ret /= 2 * stdev;
    }

    return ret;
}


void GaussianNDProbDenFunc::prepareRandDrawing() const
{
    /*
    const int nrdims = nrDims();
    BoolTypeSet isconn( nrdims, false );
    TypeSet<float> randpos;
    for ( int icorr=0; icorr<corrs_.size(); icorr++ )
    {
    }
    */
}


void GaussianNDProbDenFunc::drawRandomPos( TypeSet<float>& poss ) const
{
    //TODO implement properly. Now just uncorrelated.
    for ( int idim=0; idim<vars_.size(); idim++ )
    {
	const float x0 = (float)Stats::randGen().getNormal( 0, 1 );
	poss += vars_[idim].exp_ + vars_[idim].std_ * x0;
    }
}


void GaussianNDProbDenFunc::fillPar( IOPar& par ) const
{
    ProbDenFunc::fillPar( par );
    if ( nrDims() == 1 )
    {
	Gaussian1DProbDenFunc pdf1d( vars_[0].exp_, vars_[0].std_ );
	pdf1d.setName( name() );
	pdf1d.varnm_ = vars_[0].name_;
	pdf1d.fillPar( par );
    }
    else if ( nrDims() == 2 )
    {
	Gaussian2DProbDenFunc pdf2d;
	pdf2d.setName( name() );
	pdf2d.dim0nm_ = vars_[0].name_; pdf2d.dim1nm_ = vars_[1].name_;
	pdf2d.exp0_ = vars_[0].exp_; pdf2d.exp1_ = vars_[1].exp_;
	pdf2d.std0_ = vars_[0].std_; pdf2d.std1_ = vars_[1].std_;
	pdf2d.cc_ = corrs_.isEmpty() ? 0 : corrs_[0].cc_;
	pdf2d.fillPar( par );
    }

    for ( int idx=0; idx<vars_.size(); idx++ )
	par.set( IOPar::compKey(sKeyDimStats,idx), vars_[idx].exp_,
						   vars_[idx].std_ );
    for ( int idx=0; idx<corrs_.size(); idx++ )
	par.set( IOPar::compKey(sKeyCorr,idx), corrs_[idx].idx0_,
					corrs_[idx].idx1_, corrs_[idx].cc_ );
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
	par.get( IOPar::compKey(sKeyDimStats,idx), vars_[idx].exp_,
						   vars_[idx].std_ );
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

    return true;
}


void GaussianNDProbDenFunc::dump( od_ostream& strm, bool binary ) const
{ pErrMsg("not impl"); }

bool GaussianNDProbDenFunc::obtain( od_istream& strm, bool binary )
{ pErrMsg("not impl"); return false; }

