/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/


#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seislineprovider.h"
#include "seisps2dprovider.h"
#include "seisps3dprovider.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seisvolprovider.h"
#include "uistrings.h"


Seis::Provider::Provider()
    : forcefpdata_(false)
    , readmode_(Prod)
    , zstep_(mUdf(float))
    , nrcomps_(1)
    , seldata_(0)
    , setupchgd_(true)
{
}


Seis::Provider::~Provider()
{
    delete seldata_;
}


BufferString Seis::Provider::name() const
{
    return DBM().nameOf( dbky_ );
}


Seis::Provider* Seis::Provider::create( Seis::GeomType gt )
{
    switch ( gt )
    {
    case Vol:
	return new VolProvider;
    case VolPS:
	return new PS3DProvider;
    case Line:
	return new LineProvider;
    case LinePS:
	return new PS2DProvider;
    }

    // can't reach
    return 0;
}


Seis::Provider* Seis::Provider::create( const DBKey& dbky, uiRetVal* uirv )
{
    SeisIOObjInfo objinf( dbky );
    Provider* ret = 0;
    if ( !objinf.isOK() )
    {
	if ( uirv )
	    uirv->set( uiStrings::phrCannotFindDBEntry(dbky.toUiString()) );
    }
    else
    {
	ret = create( objinf.geomType() );
	uiRetVal dum; if ( !uirv ) uirv = &dum;
	*uirv = ret->setInput( dbky );
	if ( !uirv->isOK() )
	    { delete ret; ret = 0; }
    }

    return ret;
}


Seis::Provider* Seis::Provider::create( const IOPar& iop, uiRetVal* uirv )
{
    const DBKey ky = DBKey::getFromString( iop.find(sKey::ID()) );
    if ( ky.isInvalid() )
	return 0;

    Provider* ret = create( ky, uirv );
    if ( ret )
	ret->usePar( iop );

    return ret;
}


uiRetVal Seis::Provider::reset() const
{
    uiRetVal uirv;
    doReset( uirv );
    if ( uirv.isOK() )
    {
	if ( seldata_ && !seldata_->isAll() )
	    totalnr_ = seldata_->expectedNrTraces( is2D() );
	else
	    totalnr_ = getTotalNrInInput();

	int nrselectedcomps = 0;
	for ( int idx=0; idx<selcomps_.size(); idx++ )
	    if ( selcomps_[idx] != -1 )
		nrselectedcomps++;

	nrcomps_ = SeisIOObjInfo(dbky_).nrComponents();
	if ( nrselectedcomps != 0 )
	    nrcomps_ = mMIN( nrselectedcomps, nrcomps_ );
    }
    setupchgd_ = false;
    return uirv;
}


od_int64 Seis::Provider::totalNr() const
{
    Threads::Locker locker( lock_ );
    return totalnr_;
}


int Seis::Provider::nrOffsets() const
{
    return gtNrOffsets();
}


uiRetVal Seis::Provider::getComponentInfo( BufferStringSet& nms,
					   DataType* pdt ) const
{
    nms.setEmpty();
    DataType dtype;
    uiRetVal uirv = doGetComponentInfo( nms, dtype );
    if ( uirv.isOK() && pdt )
	*pdt = dtype;

    return uirv;
}


uiRetVal Seis::Provider::doGetComponentInfo( BufferStringSet& nms,
					     DataType& dt ) const
{
    nms.add( sKey::Data() );
    dt = UnknownData;
    return uiRetVal::OK();
}


bool Seis::Provider::haveSelComps() const
{
    for ( int idx=0; idx<selcomps_.size(); idx++ )
	if ( selcomps_[idx] >= 0 )
	    return true;
    return false;
}


uiRetVal Seis::Provider::setInput( const DBKey& dbky )
{
    Threads::Locker locker( lock_ );
    dbky_ = dbky;
    delete seldata_; seldata_ = 0;
    setupchgd_ = true;
    return reset();
}


DBKey Seis::Provider::dbKey( const IOPar& iop )
{
    const char* res = iop.find( sKey::ID() );
    BufferString tmp;
    if ( !res )
    {
	res = iop.find( sKey::Name() );
	if ( res && *res )
	{
	    const IOObj* tryioobj = DBM().getByName(IOObjContext::Seis,res);
	    if ( !tryioobj )
		res = 0;
	    else
	    {
		tmp = tryioobj->key();
		res = tmp.buf();
	    }
	}
    }

    if ( res && *res )
	return DBKey::getFromString( res );

    return DBKey::getInvalid();
}


void Seis::Provider::setSampleInterval( float zs )
{
    Threads::Locker locker( lock_ );
    zstep_ = zs;
    setupchgd_ = true;
}


void Seis::Provider::selectComponent( int icomp )
{
    Threads::Locker locker( lock_ );
    selcomps_.setEmpty();
    selcomps_ += icomp;
    setupchgd_ = true;
}


void Seis::Provider::selectComponents( const TypeSet<int>& comps )
{
    Threads::Locker locker( lock_ );
    selcomps_ = comps;
    setupchgd_ = true;
}


void Seis::Provider::forceFPData( bool yn )
{
    Threads::Locker locker( lock_ );
    forcefpdata_ = yn;
}


void Seis::Provider::setReadMode( ReadMode rm )
{
    Threads::Locker locker( lock_ );
    readmode_ = rm;
    setupchgd_ = true;
}


uiRetVal Seis::Provider::fillPar( IOPar& iop ) const
{
    iop.setYN( sKeyForceFPData(), forcefpdata_ );
    iop.set( sKey::TraceKey(), curPosition() );
    iop.set( sKeySelectedComponents(), selcomps_ );

    uiRetVal ret;
    doFillPar( iop, ret );
    return ret;
}


uiRetVal Seis::Provider::usePar( const IOPar& iop )
{
    forcefpdata_ = iop.isTrue( sKeyForceFPData() );
    iop.get( sKeySelectedComponents(), selcomps_ );

    uiRetVal ret;
    doUsePar( iop, ret );

    TrcKey trckey;
    if ( iop.get(sKey::TraceKey(),trckey) )
	doGoTo( trckey );

    return ret;
}


void Seis::Provider::setSelData( SelData* sd )
{
    Threads::Locker locker( lock_ );
    delete seldata_;
    seldata_ = sd;
    setupchgd_ = true;
    reset();
}


void Seis::Provider::putTraceInGather( const SeisTrc& trc, SeisTrcBuf& tbuf )
{
    const int nrcomps = trc.data().nrComponents();
    const int trcsz = trc.size();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	SeisTrc* newtrc = new SeisTrc( trcsz,
			    trc.data().getInterpreter(icomp)->dataChar() );
	newtrc->info() = trc.info();
	newtrc->info().offset_ = icomp * 100.f;
	newtrc->data().copyFrom( trc.data(), icomp, 0 );
	tbuf.add( newtrc );
    }
}


void Seis::Provider::putGatherInTrace( const SeisTrcBuf& tbuf, SeisTrc& trc )
{
    const int nrcomps = tbuf.size();
    if ( nrcomps < 1 )
	return;

    trc.info() = tbuf.get(0)->info();
    trc.info().offset_ = 0.f;
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const SeisTrc& buftrc = *tbuf.get( icomp );
	trc.data().addComponent( buftrc.size(),
			      buftrc.data().getInterpreter(0)->dataChar() );
	trc.data().copyFrom( buftrc.data(), 0, icomp );
    }
}


void Seis::Provider::handleTrace( SeisTrc& trc ) const
{
    TraceData& data = trc.data();
    ensureRightComponents( data );
    ensureRightZSampling( trc );
    ensureRightDataRep( data );
    nrdone_++;
}


void Seis::Provider::handleTraces( SeisTrcBuf& tbuf ) const
{
    for ( int idx=0; idx<tbuf.size(); idx++ )
	handleTrace( *tbuf.get(idx) );
}


bool Seis::Provider::handleSetupChanges( uiRetVal& uirv ) const
{
    if ( setupchgd_ )
	uirv = reset();
    return uirv.isOK();
}


uiRetVal Seis::Provider::goTo( const TrcKey& tk )
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGoTo( tk );
    return uirv;
}


uiRetVal Seis::Provider::getNext( SeisTrc& trc ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGetNext( trc, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTrace( trc );
    return uirv;
}


uiRetVal Seis::Provider::get( const TrcKey& trcky, SeisTrc& trc ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGet( trcky, trc, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTrace( trc );
    return uirv;
}


uiRetVal Seis::Provider::getData( const TrcKey& trcky, TraceData& data,
				  SeisTrcInfo* trcinfo ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;
    doGetData( trcky, data, trcinfo, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	nrdone_++;
    return uirv;
}


uiRetVal Seis::Provider::getNextGather( SeisTrcBuf& tbuf ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGetNextGather( tbuf, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTraces( tbuf );
    return uirv;
}


uiRetVal Seis::Provider::getGather( const TrcKey& trcky,
				    SeisTrcBuf& tbuf ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;
    doGetGather( trcky, tbuf, uirv );
    locker.unlockNow();

    if ( uirv.isOK() )
	handleTraces( tbuf );
    return uirv;
}


uiRetVal Seis::Provider::getSequence( Seis::RawTrcsSequence& rawseq ) const
{
    uiRetVal uirv;

    Threads::Locker locker( lock_ );
    if ( !handleSetupChanges(uirv) )
	return uirv;

    doGetSequence( rawseq, uirv );
    locker.unlockNow();

    return uirv;
}


void Seis::Provider::ensureRightComponents( TraceData& data ) const
{
    for ( int idx=data.nrComponents()-1; idx>=nrcomps_; idx-- )
	data.delComponent( idx );
}


void Seis::Provider::ensureRightDataRep( TraceData& data ) const
{
    if ( !forcefpdata_ )
	return;

    const int nrcomps = data.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
	data.convertToFPs();
}


void Seis::Provider::ensureRightZSampling( SeisTrc& trc ) const
{
    if ( mIsUdf(zstep_) )
	return;

    const ZSampling trczrg( trc.zRange() );
    ZSampling targetzs( trczrg );
    targetzs.step = zstep_;
    int nrsamps = (int)( (targetzs.stop-targetzs.start)/targetzs.step + 1.5 );
    targetzs.stop = targetzs.atIndex( nrsamps-1 );
    if ( targetzs.stop - targetzs.step*0.001f > trczrg.stop )
    {
	nrsamps--;
	if ( nrsamps < 1 )
	    nrsamps = 1;
    }
    targetzs.stop = targetzs.atIndex( nrsamps-1 );

    TraceData newtd;
    const TraceData& orgtd = trc.data();
    const int newsz = targetzs.nrSteps() + 1;
    const int nrcomps = trc.nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const DataInterpreter<float>& di = *orgtd.getInterpreter(icomp);
	const DataCharacteristics targetdc( forcefpdata_
		? (di.nrBytes()>4 ? OD::F64 : OD::F32) : di.dataChar() );
	newtd.addComponent( newsz, targetdc );
	for ( int isamp=0; isamp<newsz; isamp++ )
	    newtd.setValue( isamp, trc.getValue(targetzs.atIndex(isamp),icomp));
    }

    trc.data() = newtd;
}


bool Seis::Provider::doGetIsPresent( const TrcKey& tk ) const
{
    return Survey::GM().getGeometry(curGeomID())->includes( tk );
}


void Seis::Provider::doGetNext( SeisTrc& trc, uiRetVal& uirv ) const
{
    SeisTrcBuf tbuf( true );
    doGetNextGather( tbuf, uirv );
    putGatherInTrace( tbuf, trc );
}


void Seis::Provider::doGet( const TrcKey& tkey, SeisTrc& trc,
			    uiRetVal& uirv ) const
{
    SeisTrcBuf tbuf( true );
    doGetGather( tkey, tbuf, uirv );
    putGatherInTrace( tbuf, trc );
}


void Seis::Provider::doGetData( const TrcKey& tkey, TraceData& data,
				SeisTrcInfo* trcinfo, uiRetVal& uirv ) const
{
    SeisTrc trc;
    doGet( tkey, trc, uirv );
    data = trc.data();
    if ( trcinfo )
	*trcinfo = trc.info();
}


void Seis::Provider::doGetNextGather( SeisTrcBuf& tbuf, uiRetVal& uirv ) const
{
    SeisTrc trc;
    tbuf.erase();
    doGetNext( trc, uirv );
    if ( uirv.isOK() )
	putTraceInGather( trc, tbuf );
}


void Seis::Provider::doGetGather( const TrcKey& tkey, SeisTrcBuf& tbuf,
				  uiRetVal& uirv ) const
{
    SeisTrc trc;
    tbuf.erase();
    doGet( tkey, trc, uirv );
    if ( uirv.isOK() )
	putTraceInGather( trc, tbuf );
}


void Seis::Provider::doGetSequence( Seis::RawTrcsSequence& rawseq,
				    uiRetVal& uirv ) const
{
    SeisTrc trc;
    SeisTrcInfo& trcinfo = trc.info();
    SeisTrcBuf tbuf( true );
    const bool isps = rawseq.isPS();
    for ( int ipos=0; ipos<rawseq.nrpos_; ipos++ )
    {
	const TrcKey& tk = (*rawseq.tks_)[ipos];
	if ( isps )
	{
	    uirv = getGather( tk, tbuf );
	    rawseq.copyFrom( tbuf );
	}
	else
	{
	    uirv = getData( tk, *rawseq.data_.get(ipos), &trcinfo );
	    SeisTrcTranslator* trl = getCurrentTranslator();
	    if ( trl ) rawseq.setTrcScaler( ipos, trl->curtrcscale_ );
	    if ( !uirv.isOK() )
	    {
		uirv = get( tk, trc );
		rawseq.copyFrom( trc, &ipos );
	    }
#ifdef __debug__
	    else if ( trcinfo.trckey_ != tk )
	    {
		pErrMsg("Wrong position returned from translator");
		return;
	    }
#endif
	}
    }
}


void Seis::Provider::doFillPar( IOPar& iop, uiRetVal& uirv ) const
{
    iop.set( sKey::ID(), dbKey() );
    if ( seldata_ )
	seldata_->fillPar( iop );
    else
	Seis::SelData::removeFromPar( iop );
}


void Seis::Provider::doUsePar( const IOPar& iop, uiRetVal& uirv )
{
    const DBKey dbkey = dbKey( iop );
    if ( !dbkey.isInvalid() && dbkey!=dbKey() )
	setInput( dbkey );

    setSelData( Seis::SelData::get(iop) );
}


ZSampling Seis::Provider3D::doGetZRange() const
{
    TrcKeyZSampling tkzs;
    getRanges( tkzs );
    return tkzs.zsamp_;
}


ZSampling Seis::Provider2D::doGetZRange() const
{
    StepInterval<int> trcrg; ZSampling zsamp;
    getRanges( curLineIdx(), trcrg, zsamp );
    return zsamp;
}



Seis::RawTrcsSequence::RawTrcsSequence( const ObjectSummary& info, int nrpos )
    : info_(info)
    , nrpos_(nrpos)
    , tks_(0)
    , intpol_(0)
{
    trcscalers_.allowNull( true );
    TraceData td;
    for ( int icomp=0; icomp<info.nrcomp_; icomp++ )
	td.addComponent( info.nrsamppertrc_, info.getDataChar() );

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
    trcscalers_.allowNull( true );
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


const DataCharacteristics Seis::RawTrcsSequence::getDataChar() const
{ return info_.getDataChar(); }



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
		if ( trc.info().trckey_ != (*tks_)[idx] )
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
	    if ( trc.info().trckey_ != (*tks_)[*ipos] )
		pErrMsg("wrong position");
	}
#endif
    }

    for ( int icomp=0; icomp<info_.nrcomp_; icomp++ )
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
