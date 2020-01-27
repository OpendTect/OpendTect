/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/


#include "seisrawtrcsseq.h"
#include "scaler.h"
#include "seisioobjinfo.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "trckey.h"


Seis::RawTrcsSequence::RawTrcsSequence( const ObjectSummary& info, int nrpos )
    : info_(info)
    , nrpos_(nrpos)
    , tks_(0)
    , intpol_(0)
{
    trcscalers_.setNullAllowed( true );
    TraceData td;
    for ( int icomp=0; icomp<info.compnms_.size(); icomp++ )
	td.addComponent( info.nrsamppertrc_, info.dataChar() );

    if ( !td.allOk() )
	return;

    for ( int pos=0; pos<nrpos; pos++ )
    {
	TraceData* newtd = new TraceData( td );
	if ( !newtd || !newtd->allOk() )
	{
	    delete newtd; deepErase( data_ );
	    return;
	}

	data_ += newtd;
	trcscalers_ += 0;
    }
}


Seis::RawTrcsSequence::RawTrcsSequence( const Seis::RawTrcsSequence& oth )
    : info_(oth.info_)
    , nrpos_(oth.nrpos_)
    , tks_(0)
    , intpol_(0)
{
    trcscalers_.setNullAllowed( true );
    for ( int idx=0; idx<oth.nrpos_; idx++ )
	trcscalers_ += 0;

    *this = oth;
}


Seis::RawTrcsSequence::~RawTrcsSequence()
{
    deepErase( data_ );
    delete tks_;
    deepErase( trcscalers_ );
}


Seis::RawTrcsSequence& Seis::RawTrcsSequence::operator =(
					const Seis::RawTrcsSequence& oth )
{
    if ( &oth == this ) return *this;

    Seis::RawTrcsSequence& thisseq = const_cast<Seis::RawTrcsSequence&>(*this);

    deepErase( data_ );
    deepErase( trcscalers_ );
    deleteAndZeroPtr( thisseq.tks_ );
    const int& thisnrpos = nrpos_;
    const_cast<int&>( thisnrpos ) = oth.nrpos_;
    TypeSet<TrcKey>* tks =  new TypeSet<TrcKey>;
    for ( int pos=0; pos<nrpos_; pos++ )
    {
	data_ += new TraceData( *oth.data_.get( pos ) );
	trcscalers_ += 0;
	setTrcScaler( pos, oth.trcscalers_[pos] );
	*tks += TrcKey( (*oth.tks_)[pos] );
    }

    delete tks_;
    tks_ = tks;

    intpol_ = 0;
    if ( oth.intpol_ )
	intpol_ = new ValueSeriesInterpolator<float>( *oth.intpol_ );

    return *this;
}


bool Seis::RawTrcsSequence::isOK() const
{
    return data_.size() == nrpos_ && info_.isOK() && tks_ &&
	   tks_->size() == nrpos_;
}


const DataCharacteristics Seis::RawTrcsSequence::dataChar() const
{ return info_.dataChar(); }



const ValueSeriesInterpolator<float>&
				  Seis::RawTrcsSequence::interpolator() const
{
    if ( !intpol_ )
    {
	ValueSeriesInterpolator<float>* newintpol =
					new ValueSeriesInterpolator<float>();
	newintpol->snapdist_ = cDefSampleSnapDist();
	newintpol->smooth_ = true;
	newintpol->extrapol_ = false;
	newintpol->udfval_ = 0;

	intpol_.setIfNull(newintpol,true);
    }

    intpol_->maxidx_ = info_.nrsamppertrc_ - 1;

    return *intpol_;
}


bool Seis::RawTrcsSequence::isPS() const
{ return info_.isPS(); }


const ZSampling& Seis::RawTrcsSequence::getZRange() const
{ return info_.zsamp_; }


int Seis::RawTrcsSequence::nrPositions() const
{
    return tks_ ? nrpos_ : 0;
}


void Seis::RawTrcsSequence::setPositions( const TypeSet<TrcKey>& tks )
{
    delete tks_;
    tks_ = &tks;
}


void Seis::RawTrcsSequence::setTrcScaler( int pos, const Scaler* trcscaler )
{
    if ( !trcscalers_.validIdx(pos) )
	return;

    delete trcscalers_.replace( pos, trcscaler ? trcscaler->clone() : 0 );
}


float Seis::RawTrcsSequence::get( int idx, int pos, int comp ) const
{
    const float val = data_.get( pos )->getValue( idx, comp );
    const Scaler* trcscaler = trcscalers_[pos];
    return trcscaler ? (float)trcscaler->scale(val) : val;
}


float Seis::RawTrcsSequence::getValue( float z, int pos, int comp ) const
{
    const int sz = info_.nrsamppertrc_;
    const int sampidx = info_.zsamp_.getIndex( z );
    if ( sampidx < 0 || sampidx >= sz )
	return interpolator().udfval_;

    const float samppos = ( z - info_.zsamp_.start ) / info_.zsamp_.step;
    if ( sampidx-samppos > -cDefSampleSnapDist() &&
	 sampidx-samppos <  cDefSampleSnapDist() )
	return get( sampidx, pos, comp );

    return interpolator().value( RawTrcsSequenceValueSeries(*this,pos,comp),
				 samppos );
}


void Seis::RawTrcsSequence::set( int idx, float val, int pos, int comp )
{
    const Scaler* trcscaler = trcscalers_[pos];
    if ( trcscaler )
	val = (float)trcscaler->unScale( val );
    data_.get( pos )->setValue( idx, val, comp );
}



const DataBuffer::buf_type* Seis::RawTrcsSequence::getData( int pos, int icomp,
							    int is ) const
{
    const int offset = is > 0 ? is * info_.nrbytespersamp_ : 0;

    return data_.get( pos )->getComponent(icomp)->data() + offset;
}


DataBuffer::buf_type* Seis::RawTrcsSequence::getData( int pos, int icomp,
						      int is )
{
    return const_cast<DataBuffer::buf_type*>(
     const_cast<const Seis::RawTrcsSequence&>( *this ).getData(pos,icomp,is) );
}


void Seis::RawTrcsSequence::copyFrom( const SeisTrc& trc, int* ipos )
{
    int pos = ipos ? *ipos : -1;
    if ( tks_ )
    {
	if ( !ipos )
	{
	    for ( int idx=0; idx<nrpos_; idx++ )
	    {
		if ( trc.info().trcKey() != (*tks_)[idx] )
		    { pErrMsg("wrong position"); continue; }

		pos = idx;
		break;
	    }
	}
#ifdef __debug__
	else
	{
	    if ( trc.info().trcKey() != (*tks_)[*ipos] )
		pErrMsg("wrong position");
	}
#endif
    }

    for ( int icomp=0; icomp<info_.compnms_.size(); icomp++ )
    {
	if ( *trc.data().getInterpreter(icomp) ==
	     *data_.get(pos)->getInterpreter(icomp) ||
	     !trcscalers_[pos] )
	{
	    const od_int64 nrbytes = info_.nrdatabytespespercomptrc_;
	    OD::sysMemCopy( getData( pos, icomp ),
			    trc.data().getComponent( icomp )->data(), nrbytes);
	}
	else
	{
	    const int nrz = info_.zRange().nrSteps()+1;
	    for ( int idz=0; idz<nrz; idz++ )
	    {
		const float val = trc.get( idz, icomp );
		set( idz, val, pos, icomp );
	    }
	}
    }
}



Seis::RawTrcsSequenceValueSeries::RawTrcsSequenceValueSeries(
					const Seis::RawTrcsSequence& seq,
					int pos, int comp )
    : seq_(const_cast<Seis::RawTrcsSequence&>(seq))
    , ipos_(pos)
    , icomp_(comp)
{
}


Seis::RawTrcsSequenceValueSeries::~RawTrcsSequenceValueSeries()
{
}


ValueSeries<float>* Seis::RawTrcsSequenceValueSeries::clone() const
{ return new RawTrcsSequenceValueSeries( seq_, ipos_, icomp_ ); }


void Seis::RawTrcsSequenceValueSeries::setValue( od_int64 idx, float val )
{ seq_.set( (int)idx, val, ipos_, icomp_ ); }


float* Seis::RawTrcsSequenceValueSeries::arr()
{ return (float*)seq_.getData(ipos_,icomp_); }


float Seis::RawTrcsSequenceValueSeries::value( od_int64 idx ) const
{ return seq_.get( (int)idx, ipos_, icomp_ ); }

const float* Seis::RawTrcsSequenceValueSeries::arr() const
{ return (float*)seq_.getData(ipos_,icomp_); }

od_int64 Seis::RawTrcsSequenceValueSeries::size() const
{
    return seq_.data_.isEmpty() ? 0
				: seq_.data_.first()->size();
}
