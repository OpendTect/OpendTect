/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisrawtrcsseq.h"

#include "scaler.h"
#include "seiscommon.h"
#include "seisioobjinfo.h"
#include "seistrc.h"


Seis::RawTrcsSequence::RawTrcsSequence( const ObjectSummary& info, int nrpos )
    : info_(info)
    , tks_(nullptr)
    , nrpos_(nrpos)
    , intpol_(nullptr)
{
    trcscalers_.setNullAllowed( true );
    TraceData td;
    for ( int icomp=0; icomp<info.compnms_.size(); icomp++ )
	td.addComponent( info.nrsamppertrc_, info.getDataChar() );

    if ( !td.allOk() )
	return;

    for ( int idx=0; idx<nrpos; idx++ )
    {
	TraceData* newtd = new TraceData( td );
	if ( !newtd || !newtd->allOk() )
	{
	    delete newtd; deepErase( data_ );
	    return;
	}

	data_ += newtd;
	trcscalers_ += nullptr;
    }
}


Seis::RawTrcsSequence::RawTrcsSequence( const Seis::RawTrcsSequence& oth )
    : info_(oth.info_)
    , tks_(nullptr)
    , nrpos_(oth.nrpos_)
    , intpol_(nullptr)
{
    trcscalers_.setNullAllowed( true );
    for ( int idx=0; idx<oth.nrpos_; idx++ )
	trcscalers_ += nullptr;

    *this = oth;
}


Seis::RawTrcsSequence::~RawTrcsSequence()
{
    deepErase( data_ );
    deepErase( trcscalers_ );
    delete tks_;
}


Seis::RawTrcsSequence& Seis::RawTrcsSequence::operator=(
					const Seis::RawTrcsSequence& oth )
{
    if ( &oth == this ) return *this;

    Seis::RawTrcsSequence& thisseq = const_cast<Seis::RawTrcsSequence&>(*this);

    deepErase( data_ );
    deepErase( trcscalers_ );
    deleteAndZeroPtr( thisseq.tks_ );
    const int& thisnrpos = nrpos_;
    const_cast<int&>( thisnrpos ) = oth.nrpos_;
    TypeSet<TrcKey>* tks = new TypeSet<TrcKey>;
    for ( int ipos=0; ipos<nrpos_; ipos++ )
    {
	data_ += new TraceData( *oth.data_[ipos] );
	trcscalers_ += nullptr;
	setTrcScaler( ipos, oth.getTrcScaler(ipos) );
	*tks += TrcKey( (*oth.tks_)[ipos] );
    }

    delete tks_;
    tks_ = tks;

    intpol_ = nullptr;
    if ( oth.intpol_ )
	intpol_ = new ValueSeriesInterpolator<float>( *oth.intpol_ );

    return *this;
}


bool Seis::RawTrcsSequence::isOK() const
{
    return data_.size() == nrpos_ && info_.isOK() && tks_ &&
	   tks_->size() == nrpos_;
}


const ValueSeriesInterpolator<float>&
				Seis::RawTrcsSequence::interpolator() const
{
    if ( !intpol_ )
    {
	ValueSeriesInterpolator<float>* newintpol =
					new ValueSeriesInterpolator<float>();
	newintpol->snapdist_ = Seis::cDefSampleSnapDist();
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


const DataCharacteristics Seis::RawTrcsSequence::getDataChar() const
{ return info_.getDataChar(); }


const StepInterval<float>& Seis::RawTrcsSequence::getZRange() const
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


void Seis::RawTrcsSequence::setTrcScaler( int pos, const Scaler* newscaler )
{
    if ( !trcscalers_.validIdx(pos) )
	return;

    delete trcscalers_.replace( pos, newscaler ? newscaler->clone() : 0 );
}


const Scaler* Seis::RawTrcsSequence::getTrcScaler( int pos ) const
{
    if ( !trcscalers_.validIdx(pos) )
	return nullptr;

    return trcscalers_.get( pos );
}


const TrcKey& Seis::RawTrcsSequence::getPosition( int pos ) const
{ return (*tks_)[pos]; }


float Seis::RawTrcsSequence::get( int idx, int pos, int comp ) const
{
    const float val = data_.get( pos )->getValue( idx, comp );
    const Scaler* trcscaler = getTrcScaler( pos );
    return trcscaler ? float(trcscaler->scale(val)) : val;
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
    const Scaler* trcscaler = getTrcScaler( pos );
    if ( trcscaler )
	val = float( trcscaler->unScale(val) );
    data_.get( pos )->setValue( idx, val, comp );
}



const unsigned char* Seis::RawTrcsSequence::getData( int pos, int icomp,
						     int is ) const
{
    const int offset = is > 0 ? is * info_.nrbytespersamp_ : 0;
    return data_.get( pos )->getComponent(icomp)->data() + offset;
}


unsigned char* Seis::RawTrcsSequence::getData( int pos, int icomp, int is )
{
    return const_cast<unsigned char*>(
	const_cast<const Seis::RawTrcsSequence&>(*this).getData(pos,icomp,is) );
}


void Seis::RawTrcsSequence::copyFrom( const SeisTrc& trc, int* ipos )
{
    int pos = ipos ? *ipos : -1;
    const bool is2d = info_.is2D();
    if ( tks_ )
    {
	if ( !ipos )
	{
	    for ( int idx=0; idx<nrpos_; idx++ )
	    {
		if ( (is2d && trc.info().trcNr() != (*tks_)[idx].trcNr() ) ||
		    (!is2d && trc.info().binID() != (*tks_)[idx].position() ) )
		{
		    pErrMsg("wrong position");
		    continue;
		}

		pos = idx;
		break;
	    }
	}
#ifdef __debug__
	else
	{
	    if ( (is2d && trc.info().trcNr() != (*tks_)[*ipos].trcNr() )
	     || (!is2d && trc.info().binID() != (*tks_)[*ipos].position() ) )
		pErrMsg("wrong position");
	}
#endif
    }

    for ( int icomp=0; icomp<info_.nrcomp_; icomp++ )
    {
	if ( *trc.data().getInterpreter(icomp) ==
	     *data_.get(pos)->getInterpreter(icomp) ||
	     !getTrcScaler(pos) )
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
