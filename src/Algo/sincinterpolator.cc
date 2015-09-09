#include "sincinterpolator.h"

#include "threadlock.h"
#include "windowfunction.h"

#define EWIN_FRAC	0.9f
#define NTAB_MAX	16385.0f


SincTableManager::Table::Table( int lsinc ,int nsinc, float emax,
				float fmax, int lmax )
    : asinc_(lsinc,nsinc)
    , emax_(emax)
    , fmax_(fmax)
    , lmax_(lmax)
{}


bool SincTableManager::Table::hasSameDesign( float fmax, int lmax ) const
{
    return mIsEqual(fmax_,fmax,fmax_*1e-5f) && lmax_ == lmax;
}


long SincTableManager::Table::getTableBytes() const
{ return mCast(long, sizeof(float) * asinc_.get1DDim() ); }

int SincTableManager::Table::getLength() const
{ return asinc_.info().getSize(0); }

int SincTableManager::Table::getNumbers() const
{ return asinc_.info().getSize(1); }

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

    Array2D<float>& asinc = table->asinc_;
    for ( int j=0; j<lsinc; ++j )
    {
	asinc.set( j, 0, 0.0f );
	asinc.set( j, nsinc-1, 0.0f );
    }

    asinc.set( lsinc/2-1, 0, 1.0f );
    asinc.set( lsinc/2, nsinc-1, 1.0f );

    dsinc = 1.f / mCast(float,nsinc-1);
    const float lsinc2 = mCast(float,lsinc/2.0f);
    StepInterval<float> xvals( -lsinc2 + 1.f, -lsinc2, -dsinc );
    for ( int isinc=1; isinc<nsinc-1; ++isinc )
    {
	float x = xvals.atIndex( isinc );
	for ( int i=0; i<lsinc; ++i,x+=1.0f )
	    asinc.set( i, isinc, sinc(x) * kwin.getValue(x/lsinc2) );
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
    , lsinc_(0)
    , ishift_(0)
{}


const float SincInterpolator::snapdist = 1e-4f;


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

