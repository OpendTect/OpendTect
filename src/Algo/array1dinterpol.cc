/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Satyaki Maitra
 * DATE     : December 2009
-*/


#include "array1dinterpol.h"

#include "arrayndimpl.h"
#include "interpol1d.h"


Array1DInterpol::Array1DInterpol()
    : Executor( "Interpolator" )
    , arr_( 0 )
    , nrdone_( 0 )
    , maxgapsize_( mUdf(size_type) )
    , arrstarted_(false)
    , doextrapol_(false)
{
}


Array1DInterpol::~Array1DInterpol()
{
}


od_int64 Array1DInterpol::nrIterations() const
{
    return arr_ ? arr_->totalSize() : 0;
}


void Array1DInterpol::reset()
{
    nrdone_ = 0;
    arrstarted_ = false;
}


void Array1DInterpol::setArray( ArrT& arr )
{
    arr_ = &arr;
    reset();
}


LinearArray1DInterpol::LinearArray1DInterpol()
    : Array1DInterpol()
{
}


void LinearArray1DInterpol::extrapolate( bool start )
{
    idx_type firstvalidix = start ? 0 : arr_->getSize(0)-1;
    if ( !mIsUdf(arr_->get(firstvalidix)) ) return;
    idx_type nextvalidix = firstvalidix;

    bool doneonce = false;
    while( true )
    {
	const idx_type posidx = start ? ++nextvalidix : --nextvalidix;
	if ( !arr_->info().validPos(posidx) )
	{
	    if ( !doneonce )
		return;
	    start ? nextvalidix-- : nextvalidix++;
	    break;
	}
	if ( !mIsUdf(arr_->get(posidx)) )
	{
	    if ( doneonce )
		break;
	    firstvalidix  = nextvalidix;
	    doneonce = true;
	}
    }

    idx_type iteratoridx = start ? 0 : arr_->getSize(0)-1;
    while( arr_->info().validPos(iteratoridx) && mIsUdf(arr_->get(iteratoridx)))
    {
	const ValT val =
	    fillwithextremes_ ? arr_->get(firstvalidix)
			      : Interpolate::linear1D( (ValT)firstvalidix,
				      arr_->get(firstvalidix),
				      (ValT)nextvalidix,
				      arr_->get(nextvalidix),
				      (ValT)iteratoridx );
	if ( !mIsUdf(val) )
	    arr_->set( iteratoridx, val );
	start ? iteratoridx++ : iteratoridx--;
    }
}


int LinearArray1DInterpol::nextStep()
{
    if ( arr_->totalSize() <= nrdone_ )
    {
	if ( doextrapol_ )
	    extrapolate( false );
	return Finished();
    }

    if ( (!arr_->info().validPos(nrdone_) || mIsUdf(arr_->get(nrdone_)))
	  && !arrstarted_ )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( !arr_->info().validPos(nrdone_) || !mIsUdf(arr_->get(nrdone_)) )
    {
	if ( doextrapol_ && !arrstarted_ && nrdone_ )
	    extrapolate( true );

	arrstarted_ = true;
	nrdone_++;
	return MoreToDo();
    }

    idx_type startidx = nrdone_-1;
    idx_type stopidx = nrdone_;
    while ( arr_->info().validPos(++stopidx) )
    {
	if ( mIsUdf(arr_->get(stopidx)) )
	    continue;
	break;
    }

    if ( stopidx >= arr_->totalSize() )
    {
	if ( doextrapol_ )
	    extrapolate( false );
	return Finished();
    }

    if ( (stopidx-startidx)>maxgapsize_ )
    {
	nrdone_++;
	return MoreToDo();
    }

    ValT val0,val1;
    ValT pos0,pos1;

    val0 = arr_->get( startidx ); pos0 = (ValT)( startidx );
    val1 = arr_->get( stopidx ); pos1 = (ValT)stopidx;

    ValT val = Interpolate::linear1D( pos0, val0, pos1, val1, (ValT)nrdone_ );
    if ( !mIsUdf(val) )
	arr_->set( nrdone_, val );
    nrdone_++;

    return MoreToDo();
}


PolyArray1DInterpol::PolyArray1DInterpol()
    : Array1DInterpol()
{
}


bool PolyArray1DInterpol::getPositions( idx_type curpos,
					TypeSet<idx_type>& posidxs )
{
    bool fisrtundef =
	arr_->info().validPos(curpos-2) && mIsUdf(arr_->get(curpos-2));
    posidxs[0] = fisrtundef ? curpos-1 : curpos-2;
    if ( !fisrtundef )
	posidxs[1] = curpos-1;
    idx_type iteratoridx = curpos;
    idx_type posidx = fisrtundef ? 1 : 2;
    while ( arr_->info().validPos(++iteratoridx) )
    {
	if ( mIsUdf(arr_->get(iteratoridx)) )
	    continue;

	if ( (iteratoridx-curpos-1)>maxgapsize_ )
	    return false;

	posidxs[posidx] = iteratoridx;
	posidx++;
	if ( posidx>=posidxs.size() )
	    break;
    }

    return true;
}


void PolyArray1DInterpol::extrapolate( bool start )
{
    TypeSet<idx_type> posidxs( 4, 0 );
    TypeSet<ValT> vals( posidxs.size(), 0.f );

    idx_type nextvalidix = start ? 0 : arr_->getSize(0)-1;
    if ( !mIsUdf(arr_->get(nextvalidix)) ) return;
    idx_type positridx = 0;
    while( positridx<4 )
    {
	const ValT arrval = arr_->get( nextvalidix );
	if ( !mIsUdf(arrval) )
	{
	    posidxs[positridx] = nextvalidix;
	    vals[positridx] = arrval;
	    positridx++;
	}
	start ? nextvalidix++ : nextvalidix--;
    }

    idx_type iteratoridx = start ? 0 : arr_->getSize(0)-1;
    while( arr_->info().validPos(iteratoridx) && mIsUdf(arr_->get(iteratoridx)))
    {
	ValT val = fillwithextremes_ ? vals[0]
				      : Interpolate::poly1D(posidxs[0],vals[0],
					posidxs[1],vals[1],posidxs[2],vals[2],
					posidxs[3],vals[3],iteratoridx);
	if ( !mIsUdf(val) )
	    arr_->set( iteratoridx, val );
	start ? iteratoridx++ : iteratoridx--;
    }
}


int PolyArray1DInterpol::nextStep()
{
    if ( arr_->totalSize() <= nrdone_ )
    {
	if ( doextrapol_ )
	    extrapolate( false );
	return Finished();
    }

    if ( (!arr_->info().validPos(nrdone_) || mIsUdf(arr_->get(nrdone_)))
	  && !arrstarted_ )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( !arr_->info().validPos(nrdone_) || !mIsUdf(arr_->get(nrdone_)) )
    {
	if ( doextrapol_ && !arrstarted_ && nrdone_ )
	    extrapolate( true );

	arrstarted_ = true;
	nrdone_++;
	return MoreToDo();
    }

    TypeSet<idx_type> posidxs( 4, 0 );
    if ( !getPositions(nrdone_,posidxs) )
    {
	nrdone_++;
	return MoreToDo();
    }

    if ( mIsUdf(arr_->get(posidxs[posidxs.size()-1])) ||
	 posidxs[posidxs.size()-1]==0 )
    {
	if ( doextrapol_ )
	    extrapolate( false );
	return Finished();
    }

    TypeSet<ValT> vals( posidxs.size(), (ValT)0 );

    for ( int idx=0; idx<vals.size(); idx++ )
	vals[idx] = arr_->get( posidxs[idx] );

    ValT val = Interpolate::poly1D( posidxs[0], vals[0], posidxs[1], vals[1],
				     posidxs[2], vals[2], posidxs[3], vals[3],
				     nrdone_ );
    if ( !mIsUdf(val) )
	arr_->set( nrdone_, val );
    nrdone_++;

    return MoreToDo();
}
