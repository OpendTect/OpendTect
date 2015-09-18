#include "sincinterpolator.h"

#include "threadlock.h"
#include "windowfunction.h"

#define EWIN_FRAC	0.9f
#define NTAB_MAX	16385.0f


SincTableManager::Table::Table( int lsinc ,int nsinc, float emax,
				float fmax, int lmax )
    : asinc_(0)
    , emax_(emax)
    , fmax_(fmax)
    , lmax_(lmax)
    , nsinc_(nsinc)
    , lsinc_(lsinc)
{
    asinc_ = SincTableManager::makeArray( lsinc, nsinc );
}


SincTableManager::Table::~Table()
{
    deleteArray( asinc_, lsinc_ );
}


bool SincTableManager::Table::hasSameDesign( float fmax, int lmax ) const
{
    return mIsEqual(fmax_,fmax,fmax_*1e-5f) && lmax_ == lmax;
}


od_int64 SincTableManager::Table::getTableBytes() const
{ return mCast(od_int64, sizeof(float) * (nsinc_*lsinc_) ); }

int SincTableManager::Table::getLength() const
{ return lsinc_; }

int SincTableManager::Table::getNumbers() const
{ return nsinc_; }

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
	    return 0;

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
	return 0;

    for ( int j=0; j<lsinc; ++j )
    {
	table->setValue( j, 0, 0.0f );
	table->setValue( j, nsinc-1, 0.0f );
    }

    table->setValue( lsinc/2-1, 0, 1.0f );
    table->setValue( lsinc/2, nsinc-1, 1.0f );

    dsinc = 1.f / mCast(float,nsinc-1);
    const float lsinc2 = mCast(float,lsinc/2.0f);
    StepInterval<float> xvals( -lsinc2 + 1.f, -lsinc2, -dsinc );
    for ( int isinc=1; isinc<nsinc-1; ++isinc )
    {
	float x = xvals.atIndex( isinc );
	for ( int i=0; i<lsinc; ++i,x+=1.0f )
	    table->setValue( i, isinc, sinc(x) * kwin.getValue(x/lsinc2) );
    }

    return table;
}


float SincTableManager::sinc( float x )
{ return mIsZero(x,mDefEpsF) ? 1.f : sin(M_PIf*x)/(M_PIf*x); }


float** SincTableManager::makeArray( int n1, int n2 )
{
    mDeclareAndTryAlloc(float**,arr,float*[n1])
    if ( !arr ) return 0;

    for ( int idx=0; idx<n1; idx++ )
    {
	mDeclareAndTryAlloc(float*,arrrow,float[n2])
	if ( !arrrow )
	{
	    for ( int idy=idx-1; idy>=0; idy-- )
		delete [] arr[idy];

	    delete [] arr;
	    return 0;
	}

	arr[idx] = arrrow;
    }

    return arr;
}


void SincTableManager::deleteArray( float** arr, int n1 )
{
    if ( !arr )
	return;

    for ( int idx=0; idx<n1; idx++ )
	delete [] arr[idx];

    delete [] arr;
}



SincTableManager& SincTableManager::STM()
{
    mDefineStaticLocalObject( PtrMan<SincTableManager>, sinctables,
			      (new SincTableManager) );
    return *sinctables;
}



SincInterpolator::SincInterpolator()
    : table_(0)
    , isudfarr_(0)
    , lsinc_(0)
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
	lsinc_ = table_->getLength();
	ishift_ = table_->getShift();
    }

    return table_;
}

