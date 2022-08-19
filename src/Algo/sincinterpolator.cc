/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "sincinterpolator.h"

#include "threadlock.h"
#include "windowfunction.h"

#define EWIN_FRAC	0.9f
#define NTAB_MAX	16385.0f


SincTableManager::~SincTableManager()
{
    deepErase( tables_ );
}



SincTableManager::Table::Table( int lsinc ,int nsinc, float emax,
				float fmax, int lmax )
    : asinc_(0)
    , asinc2_(nsinc,lsinc)
    , emax_(emax)
    , fmax_(fmax)
    , lmax_(lmax)
    , nsinc_(0)
    , lsinc_(0)
{}


bool SincTableManager::Table::hasSameDesign( float fmax, int lmax ) const
{
    return mIsEqual(fmax_,fmax,fmax_*1e-5f) && lmax_ == lmax;
}


od_int64 SincTableManager::Table::getTableBytes() const
{ return mCast(od_int64, sizeof(float) * asinc2_.info().getTotalSz()  ); }

int SincTableManager::Table::getNumbers() const
{ return asinc2_.info().getSize(0); }

int SincTableManager::Table::getLength() const
{ return asinc2_.info().getSize(1); }

int SincTableManager::Table::getShift() const
{ return - getLength()/2+1; }


const SincTableManager::Table* SincTableManager::getTable( float fmax,
							   int lmax )
{
    Threads::MutexLocker lock( lock_ );
    int tabidx = getTableIdx( fmax, lmax );
    if ( !tables_.validIdx(tabidx) )
    {
	lock.unLock();
	const Table* newtab = makeTable( fmax, lmax );
	if ( !newtab )
	    return nullptr;

	lock.lock();
	tabidx = getTableIdx( fmax, lmax );
	if ( !tables_.validIdx(tabidx) )
	{
	    tables_ += newtab;
	    return newtab;
	}
    }

    return tables_[tabidx];
}


int SincTableManager::getTableIdx( float fmax, int lmax ) const
{
    for ( int idx=0; idx<tables_.size(); idx++ )
    {
	if ( !tables_.validIdx(idx) )
	    continue;

	const Table* table = tables_[idx];
	if ( !table || !table->hasSameDesign(fmax,lmax) )
	    continue;

	return idx;
    }

    return -1;
}


const SincTableManager::Table* SincTableManager::makeTable( float fmax,
							    int lmax )
{
    const float wwin = 1.0f - 2.0f * fmax;

    KaiserWindow kwin;
    kwin.set( wwin, lmax );
    float ewin = 3.f * mCast(float,kwin.getError());
    float emax = ewin / EWIN_FRAC;
    float etabMin = 1.1f*M_PIf*fmax/(NTAB_MAX-1.0f);
    float emaxMin = etabMin/(1.0f-EWIN_FRAC);
    if ( emax<emaxMin )
    {
	emax = emaxMin;
	ewin = emax*EWIN_FRAC;
    }

    float etab = emax-ewin;
    float dsinc = fmax>0.0 ? etab/(M_PIf*fmax) : 1.0f;
    int nsincMin = 1+(int)ceil(1.0f/dsinc);
    int nsinc = 2;
    while ( nsinc<nsincMin )
	nsinc *= 2;

    ++nsinc;
    const int lsinc = kwin.getLength();

    SincTableManager::Table* table =
		new SincTableManager::Table( lsinc, nsinc, emax, fmax, lmax);
    if ( !table || !table->isOK() )
	{ delete table; return nullptr ; }

    for ( int j=0; j<lsinc; ++j )
    {
	table->setValue( 0, j, 0.0f );
	table->setValue( nsinc-1, j, 0.0f );
    }

    table->setValue( 0, lsinc/2-1, 1.0f );
    table->setValue( nsinc-1, lsinc/2, 1.0f );

    dsinc = 1.f / mCast(float,nsinc-1);
    const float lsinc2 = mCast(float,lsinc/2.0f);
    StepInterval<float> xvals( -lsinc2 + 1.f, -lsinc2, -dsinc );
    float sumvals; Array1DImpl<float> vals( lsinc );
    float* tapervals = vals.getData();
    for ( int isinc=1; isinc<nsinc-1; isinc++ )
    {
	float x = xvals.atIndex( isinc );
	sumvals = 0.f;
	for ( int ksinc=0; ksinc<lsinc; ksinc++,x+=1.0f )
	{
	    const float val = sinc(x) * kwin.getValue( x/lsinc2 );
	    tapervals[ksinc] = val;
	    sumvals += val;
	}

	const float scaler = 1.f / sumvals;
	for ( int ksinc=0; ksinc<lsinc; ksinc++ )
	    table->setValue( isinc, ksinc, tapervals[ksinc] * scaler );
    }

    return table;
}


float SincTableManager::sinc( float x )
{ return mIsZero(x,mDefEpsF) ? 1.f : sin(M_PIf*x)/(M_PIf*x); }


SincTableManager& SincTableManager::STM()
{
    mDefineStaticLocalObject( PtrMan<SincTableManager>, sinctables,
			      (new SincTableManager) );
    return *sinctables;
}



SincInterpolator::SincInterpolator()
    : table_(0)
    , asinc_(0)
    , isudfarr_(0)
    , lsinc_(0)
    , nsincm1_(-1)
    , ishift_(0)
    , extrapcst_(false)
    , extrapzero_(false)
{}


SincInterpolator::~SincInterpolator()
{
    delete [] isudfarr_;
}


const float SincInterpolator::snapdist = 1e-4f;


SincInterpolator::Extrapolation SincInterpolator::getExtrapolation()
{
    if ( extrapcst_ )
	return CONSTANT;

    if ( extrapzero_ )
	return ZERO;

    return NONE;
}


void SincInterpolator::setExtrapolation( Extrapolation et )
{
    extrapcst_ = et == CONSTANT;
    extrapzero_ = et == ZERO;
}


bool SincInterpolator::initTable( float fmax, int lmax )
{
    if ( fmax>=0.5 || lmax<8 || lmax%2 || (1.0-2.0*fmax)*lmax<=1.0 )
	return false;

    table_ = SincTableManager::STM().getTable( fmax, lmax );
    if ( table_ )
    {
	asinc_ = const_cast<const float**>( table_->asinc2_.get2DData() );
	lsinc_ = table_->getLength();
	nsincm1_ = table_->getNumbers()-1;
	ishift_ = table_->getShift();
    }

    return table_;
}
