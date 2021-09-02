/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "prestackgather.h"

#include "genericnumer.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioobjtags.h"
#include "iopar.h"
#include "ioman.h"
#include "perthreadrepos.h"
#include "ptrman.h"
#include "samplfunc.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seisread.h"
#include "seistrctr.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "keystrs.h"

static PerThreadObjectRepository<SeisTrc> rettrc_;

namespace PreStack
{

const char* Gather::sDataPackCategory()		{ return "Pre-Stack Gather"; }
const char* Gather::sKeyIsAngleGather()		{ return "Angle Gather"; }
const char* Gather::sKeyIsCorr()		{ return "Is Corrected"; }
const char* Gather::sKeyZisTime()		{ return "Z Is Time"; }
const char* Gather::sKeyPostStackDataID()	{ return "Post Stack Data"; }
const char* Gather::sKeyStaticsID()		{ return "Statics"; }
const char* GatherSetDataPack::sDataPackCategory()
{ return "Pre-Stack Gather Set"; }

Gather::Gather()
    : FlatDataPack( sDataPackCategory(), new Array2DImpl<float>(1,1) )
    , offsetisangle_( false )
    , iscorr_( true )
    , binid_( -1, -1 )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{}



Gather::Gather( const Gather& gather )
    : FlatDataPack( gather )
    , velocitymid_( gather.velocitymid_ )
    , storagemid_( gather.storagemid_ )
    , staticsmid_( gather.staticsmid_ )
    , offsetisangle_( gather.offsetisangle_ )
    , iscorr_( gather.iscorr_ )
    , zit_( gather.zit_ )
    , binid_( gather.binid_ )
    , coord_( gather.coord_ )
    , azimuths_( gather.azimuths_ )
    , zrg_(gather.zrg_)
    , linename_( gather.linename_ )
{}


Gather::Gather( const FlatPosData& fposdata )
    : FlatDataPack( sDataPackCategory(),
        new Array2DImpl<float>(fposdata.nrPts(true),fposdata.nrPts(false)) )
    , offsetisangle_( false )
    , iscorr_( true )
    , binid_( -1, -1 )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{
    posdata_ = fposdata;
    const StepInterval<double> zsamp = fposdata.range( false );
    zrg_.set( mCast(float,zsamp.start), mCast(float,zsamp.stop),
	      mCast(float,zsamp.step) );
}


Gather::~Gather()
{}


bool Gather::readFrom( const MultiID& mid, const BinID& bid, int comp,
		       uiString* errmsg )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	if ( errmsg ) (*errmsg) = tr("No valid gather selected.");
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    return readFrom( *ioobj, bid, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const BinID& bid, int comp,
		       uiString* errmsg )
{
    PtrMan<SeisPSReader> rdr = SPSIOPF().get3DReader( ioobj, bid.inl() );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = tr("This Prestack data store cannot be handled.");
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    linename_.setEmpty();
    return readFrom( ioobj, *rdr, bid, comp, errmsg );
}


bool Gather::readFrom( const MultiID& mid, const int trcnr,
		       const char* linename, int comp, uiString* errmsg )
{
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj )
    {
	if ( errmsg ) (*errmsg) = tr("No valid gather selected.");
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    return readFrom( *ioobj, trcnr, linename, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const int tracenr,
		       const char* linename, int comp, uiString* errmsg )
{
    PtrMan<SeisPSReader> rdr = SPSIOPF().get2DReader( ioobj, linename );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = tr("This Prestack data store cannot be handled.");
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    linename_ = linename;

    return readFrom( ioobj, *rdr, BinID(0,tracenr), comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, SeisPSReader& rdr, const BinID& bid,
		       int comp, uiString* errmsg )
{
    PtrMan<SeisTrcBuf> tbuf = new SeisTrcBuf( true );
    if ( !rdr.getGather(bid,*tbuf) )
    {
	if ( errmsg ) (*errmsg) = rdr.errMsg();
	delete arr2d_; arr2d_ = 0;
	return false;
    }
    if ( !setFromTrcBuf( *tbuf, comp, true ) )
       return false;

    ioobj.pars().getYN(sKeyZisTime(),zit_);

    velocitymid_.setEmpty();
    GetVelocityVolumeTag( ioobj, velocitymid_ );
    staticsmid_.setEmpty();
    ioobj.pars().get( sKeyStaticsID(), staticsmid_ );

    offsetisangle_ = false;
    ioobj.pars().getYN(sKeyIsAngleGather(), offsetisangle_ );

    iscorr_ = true;
    if ( !ioobj.pars().getYN(sKeyIsCorr(),iscorr_) )
	ioobj.pars().getYN( "Is NMO Corrected", iscorr_ );

    binid_ = bid;
    setName( ioobj.name() );

    storagemid_ = ioobj.key();

    return true;
}


bool Gather::setFromTrcBuf( SeisTrcBuf& tbuf, int comp, bool snapzrgtosi )
{
    tbuf.sort( true, SeisTrcInfo::Offset );

    bool isset = false;
    StepInterval<float> zrg;
    Coord crd( coord_ );
    for ( int idx=tbuf.size()-1; idx>-1; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	if ( mIsUdf( trc->info().offset ) )
	    delete tbuf.remove( idx );

	const int trcsz = trc->size();
	if ( !isset )
	{
	    isset = true;
	    zrg.setFrom( trc->info().sampling.interval( trcsz ) );
	    crd = trc->info().coord;
	}
	else
	{
	    zrg.start = mMIN( trc->info().sampling.start, zrg.start );
	    zrg.stop = mMAX( trc->info().sampling.atIndex(trcsz-1), zrg.stop );
	    zrg.step = mMIN( trc->info().sampling.step, zrg.step );
	}
    }

    if ( !isset )
    {
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    if ( snapzrgtosi )
    {
	SI().snapZ(zrg.start);
	SI().snapZ(zrg.stop);
    }

    zrg_ = zrg;
    int nrsamples = zrg.nrSteps()+1;
    if ( zrg.step>0 && (zrg.stop-zrg.atIndex(nrsamples-1))>fabs(zrg.step*0.5) )
	nrsamples++;

    const Array2DInfoImpl newinfo( tbuf.size(), nrsamples );
    if ( !arr2d_ || !arr2d_->setInfo( newinfo ) )
    {
	delete arr2d_;
	arr2d_ = new Array2DImpl<float>( newinfo );
	if ( !arr2d_ || !arr2d_->isOK() )
	{
	    delete arr2d_; arr2d_ = 0;
	    return false;
	}
    }

    azimuths_.setSize( tbuf.size(), mUdf(float) );

    for ( int trcidx=tbuf.size()-1; trcidx>=0; trcidx-- )
    {
	const SeisTrc* trc = tbuf.get( trcidx );
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float val = trc->getValue( zrg.atIndex( idx ), comp );
	    arr2d_->set( trcidx, idx, val );
	}

	azimuths_[trcidx] = trc->info().azimuth;
    }

    double offset;
    float* offsets = tbuf.getHdrVals(SeisTrcInfo::Offset, offset);
    if ( !offsets )
	return false;
    posData().setX1Pos( offsets, tbuf.size(), offset );
    StepInterval<double> pzrg(zrg.start, zrg.stop, zrg.step);
    posData().setRange( false, pzrg );

    zit_ = SI().zIsTime();
    coord_ = crd;
    binid_ = SI().transform( coord_ );

    return true;
}


const char* Gather::dimName( bool dim0 ) const
{
    return dim0 ? sKey::Offset() :
			(SI().zIsTime() ? sKey::Time() : sKey::Depth());
}


void Gather::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    par.set( "X", coord_.x );
    par.set( "Y", coord_.y );
    float z = (float) posData().position( false, idim1 );
    if ( zit_ ) z *= 1000;
    par.set( "Z", z );
    par.set( sKey::Offset(), getOffset(idim0) );
    if ( azimuths_.validIdx(idim0) )
	par.set( sKey::Azimuth(), getAzimuth(idim0) );
    if ( !is3D() )
	par.set( sKey::TraceNr(), binid_.crl() );
    else
	par.set( sKey::Position(), binid_.toString() );
}



const char* Gather::getSeis2DName() const
{
    return linename_.isEmpty() ? 0 : linename_.buf();
}


void Gather::setOffsetIsAngle( bool yn )
{
    offsetisangle_ = yn;
}


float Gather::getOffset( int idx ) const
{ return (float) posData().position( true, idx ); }


float Gather::getAzimuth( int idx ) const
{
    return azimuths_[idx];
}


OffsetAzimuth Gather::getOffsetAzimuth( int idx ) const
{
    return OffsetAzimuth( (float) posData().position( true, idx ),
			  azimuths_[idx] );
}


bool Gather::getVelocityID(const MultiID& stor, MultiID& vid )
{
    PtrMan<IOObj> ioobj = IOM().get( stor );
    return ioobj && GetVelocityVolumeTag( *ioobj, vid );
}

#define mIfNonZero \
const float val = data().get( offset, idz ); \
if ( val && !mIsUdf(val) )

void Gather::detectInnerMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = 0;
    for ( int offset=0; offset<nroffsets; offset++ )
    {
	for ( int idz=nrz-1; idz>=muteend; idz-- )
	{
	    mIfNonZero
		muteend = idz+1;
	}

	res[offset] = muteend-taperlen;
    }
}


void Gather::detectOuterMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = nrz-1;
    for ( int offset=nroffsets-1; offset>=0; offset-- )
    {
	for ( int idz=0; idz<=muteend; idz++ )
	{
	    mIfNonZero
		muteend = idz-1;
	}

	res[offset] = muteend + taperlen;
    }
}



GatherSetDataPack::GatherSetDataPack( const char*,
				      const ObjectSet<Gather>& gathers )
    : DataPack( sDataPackCategory() )
    , gathers_( gathers )
{
}


GatherSetDataPack::~GatherSetDataPack()
{
}


const Gather* GatherSetDataPack::getGather( const BinID& bid ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	if ( gathers_[idx]->getBinID() == bid )
	    return gathers_[idx];
    }

    return 0;
}


void GatherSetDataPack::fill( Array2D<float>& inp, int offsetidx ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	const Array2D<float>& data = gathers_[idx]->data();
	for ( int idz=0; idz<data.info().getSize(0); idz++ )
	    inp.set( idx, idz, data.get( offsetidx, idz ) );
    }
}


void GatherSetDataPack::fill( SeisTrcBuf& inp, int offsetidx ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	const int gathersz = gathers_[idx]->size(false);
	SeisTrc* trc = new SeisTrc( gathersz );
	trc->info().binid = gathers_[idx]->getBinID();
	trc->info().coord = SI().transform( gathers_[idx]->getBinID() );
	trc->info().nr = idx+1;
	const SamplingData<double>& sd = gathers_[idx]->posData().range( false);
	trc->info().sampling.set((float) sd.start, (float) sd.step );
	const Array2D<float>& data = gathers_[idx]->data();
	for ( int idz=0; idz<gathersz; idz++ )
	    trc->set( idz, data.get( offsetidx, idz ), 0 );

	inp.add( trc );
    }
}


void GatherSetDataPack::fill( SeisTrcBuf& inp, Interval<float> stackrg ) const
{
    const int gathersz = gathers_.size();
    TypeSet<int> offidxs;
    for ( int idx=0; idx<gathersz; idx++ )
    {
	const Gather& gather = *gathers_[idx];
	const int offsz = gather.size( true );

	for ( int idoff=0; idoff<offsz; idoff++ )
	{
	    if ( stackrg.includes( gather.getOffset( idoff ), true ) )
		offidxs.addIfNew( idoff );
	}
    }
    if ( offidxs.isEmpty() )
	return;

    for ( int idx=0; idx<offidxs.size(); idx++ )
    {
	if ( inp.isEmpty() )
	    fill( inp, offidxs[idx] );
	else
	{
	    SeisTrcBuf buf(false); fill( buf, offidxs[idx] );
	    for ( int idgather=0; idgather<gathersz; idgather++ )
	    {
		SeisTrcPropChg stckr( *inp.get( idgather ) );
		stckr.stack( *buf.get( idgather ) );
	    }
	}
    }
}


const SeisTrc* GatherSetDataPack::getTrace(int gatheridx,int offsetidx) const
{
    return gtTrace( gatheridx, offsetidx );
}


SeisTrc* GatherSetDataPack::getTrace( int gatheridx, int offsetidx )
{
    return gtTrace( gatheridx, offsetidx );
}


SeisTrc* GatherSetDataPack::gtTrace( int gatheridx, int offsetidx ) const
{
    SeisTrc& rettrc = rettrc_.getObject();
    const auto& gather = *gathers_[gatheridx];
    const Array2D<float>& data = gather.data();
    const int nrsamples = data.info().getSize( 1 );

    rettrc.reSize( nrsamples, false );
    rettrc.info().setBinID( gather.getBinID() );
    rettrc.info().coord = SI().transform( gather.getBinID() );
    rettrc.info().nr = gatheridx + 1;
    const SamplingData<double>& sd = gather.posData().range( false );
    rettrc.info().sampling.set( (float)sd.start, (float)sd.step );
    for ( int isamp=0; isamp<nrsamples; isamp++ )
	rettrc.set( isamp, data.get(offsetidx,isamp), 0 );

    return &rettrc;
}


StepInterval<float> GatherSetDataPack::zRange() const
{
    if ( gathers_.isEmpty() )
	return StepInterval<float>::udf();

    StepInterval<float> zrg = gathers_[0]->zRange();
    for ( int idx=1; idx<gathers_.size(); idx++ )
	zrg.include( gathers_[idx]->zRange(), false );

    return zrg;
}

} // namespace PreStack
