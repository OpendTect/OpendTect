/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2016
________________________________________________________________________

-*/


#include "seisvolprovider.h"
#include "seislineprovider.h"
#include "seisps2dprovider.h"
#include "seisps3dprovider.h"

#include "dbman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "scaler.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisrawtrcsseq.h"
#include "seisselection.h"
#include "seistrc.h"
#include "seistrctr.h"
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
    return nameOf( dbky_ );
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


Seis::Provider* Seis::Provider::create( const IOObj& ioobj, uiRetVal* uirv )
{
    return create( ioobj.key() );
}


Seis::Provider* Seis::Provider::create( const DBKey& dbky, uiRetVal* uirv )
{
    SeisIOObjInfo objinf( dbky );
    Provider* ret = 0;
    if ( !objinf.isOK() )
    {
	if ( uirv )
	    uirv->set( uiStrings::phrCannotFindDBEntry(dbky) );
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
    const DBKey dbky = DBKey::getFromStr( iop.find(sKey::ID()) );
    if ( dbky.isInvalid() )
	return 0;

    Provider* ret = create( dbky, uirv );
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
	return DBKey::getFromStr( res );

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
	    if ( trl )
		rawseq.setTrcScaler( ipos, trl->curtrcscale_ );
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
