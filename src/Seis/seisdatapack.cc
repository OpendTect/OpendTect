/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seisdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "binidvalset.h"
#include "convmemvalseries.h"
#include "flatposdata.h"
#include "flatview.h"
#include "keystrs.h"
#include "paralleltask.h"
#include "posinfo.h"
#include "randomlinegeom.h"
#include "seistrc.h"
#include "survinfo.h"

#include <limits.h>

class Regular2RandomDataCopier : public ParallelTask
{
public:
		Regular2RandomDataCopier( RandomSeisDataPack& ransdp,
					  const RegularSeisDataPack& regsdp,
					  int rancompidx )
		    : ransdp_(&ransdp)
		    , regsdp_(&regsdp)
		    , ranidx_(rancompidx)
		    , path_( const_cast<const RandomSeisDataPack&>( ransdp )
								.getPath() )
		    , samplebytes_( sizeof(float) )
		{}

    od_int64	nrIterations() const override		{ return path_.size(); }

    bool	doPrepare(int nrthreads) override;
    bool	doWork(od_int64 start,od_int64 stop,int thread) override;


private:

    RefMan<RandomSeisDataPack>	ransdp_;
    ConstRefMan<RegularSeisDataPack> regsdp_;
    const TrcKeyPath&		path_;
    int				regidx_		= -1;
    int				ranidx_;
    bool			domemcopy_	= false;
    int				samplebytes_;
    const unsigned char*	srcptr_		= nullptr;
    unsigned char*		dstptr_		= nullptr;
    od_int64			srctrcbytes_	= 0;
    od_int64			dsttrcbytes_	= 0;
    od_int64			srclnbytes_	= 0;
    od_int64			bytestocopy_	= 0;
    int				idzoffset_	= 0;
};


bool Regular2RandomDataCopier::doPrepare( int nrthreads )
{
    const RegularSeisDataPack& regsdp = *regsdp_.ptr();
    RandomSeisDataPack& ransdp = *ransdp_.ptr();
    regidx_ = regsdp.getComponentIdx( ransdp.getComponentName(ranidx_),
				       ranidx_ );

    if ( !regsdp.validComp(regidx_) || !ransdp.validComp(ranidx_) )
	return false;

    if ( !regsdp.zRange().overlaps(ransdp.zRange()) )
	return false;

    idzoffset_ = regsdp.zRange().nearestIndex( ransdp.zRange().start_ );

    if ( !regsdp.zRange().isCompatible(ransdp.zRange(),1e-3) )
    {
	pErrMsg( "Unexpected incompatibility of datapack Z-ranges" );
	return false;
    }

    if ( regsdp.getDataDesc() != ransdp.getDataDesc() )
	return true;

    srcptr_ = mCast( const unsigned char*, regsdp.data(regidx_).getData() );
    mDynamicCastGet( const ConvMemValueSeries<float>*, regstorage,
		     regsdp.data(regidx_).getStorage() );
    if ( regstorage )
    {
	srcptr_ = mCast( const unsigned char*, regstorage->storArr() );
	samplebytes_ = regsdp.getDataDesc().nrBytes();
    }

    dstptr_ = mCast( unsigned char*, ransdp.data(ranidx_).getData() );
    mDynamicCastGet( const ConvMemValueSeries<float>*, ranstorage,
		     ransdp.data(ranidx_).getStorage() );
    if ( ranstorage )
	dstptr_ = mCast( unsigned char*, ranstorage->storArr() );

    if ( !srcptr_ || !dstptr_ )
	return true;

    srctrcbytes_ = samplebytes_ * regsdp.sampling().size(TrcKeyZSampling::Z);
    srclnbytes_ = srctrcbytes_ * regsdp.sampling().size(TrcKeyZSampling::Crl);
    dsttrcbytes_ = samplebytes_ * (ransdp.zRange().nrSteps()+1);

    bytestocopy_ = dsttrcbytes_;

    if ( idzoffset_ < 0 )
    {
	dstptr_ -= samplebytes_ * idzoffset_;
	bytestocopy_ += samplebytes_ * idzoffset_;
    }
    else
	srcptr_ += samplebytes_ * idzoffset_;

    const int stopoffset = regsdp.zRange().nrSteps() -
			regsdp.zRange().nearestIndex( ransdp.zRange().stop_ );

    if ( stopoffset < 0 )
	bytestocopy_ += samplebytes_ * stopoffset;

    domemcopy_ = true;
    return true;
}


bool Regular2RandomDataCopier::doWork( od_int64 start, od_int64 stop,
				       int thread )
{
    const RegularSeisDataPack& regsdp = *regsdp_.ptr();
    RandomSeisDataPack& ransdp = *ransdp_.ptr();
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const TrcKeySampling& hsamp = regsdp.sampling().hsamp_;
	if ( !hsamp.lineRange().includes(path_[idx].lineNr(),true) ||
	     !hsamp.trcRange().includes(path_[idx].trcNr(),true) )
	    continue;

	const int shiftedtogetnearestinl = path_[idx].lineNr() +
					   hsamp.step_.lineNr()/2;
	const int inlidx = hsamp.inlIdx( shiftedtogetnearestinl );
	const int shiftedtogetnearestcrl = path_[idx].trcNr() +
					   hsamp.step_.trcNr()/2;
	const int crlidx = hsamp.crlIdx( shiftedtogetnearestcrl );

	if ( domemcopy_ )
	{
	    const unsigned char* srcptr = srcptr_ + inlidx*srclnbytes_
						  + crlidx*srctrcbytes_;
	    unsigned char* dstptr = dstptr_ + idx*dsttrcbytes_;
	    OD::sysMemCopy( dstptr, srcptr, bytestocopy_ );
	    continue;
	}

	for ( int newidz=0; newidz<=ransdp.zRange().nrfSteps(); newidz++ )
	{
	    const int oldidz = newidz + idzoffset_;
	    const float val =
		regsdp.data(regidx_).info().validPos(inlidx,crlidx,oldidz) ?
		regsdp.data(regidx_).get(inlidx,crlidx,oldidz) : mUdf(float);

	    ransdp.data(ranidx_).set( 0, idx, newidz, val );
	}
    }

    return true;
}


//=============================================================================


// SeisVolumeDataPack

SeisVolumeDataPack::SeisVolumeDataPack( const char* cat,
					const BinDataDesc* bdd )
    : VolumeDataPack(cat,bdd)
{
}


SeisVolumeDataPack::~SeisVolumeDataPack()
{
}


bool SeisVolumeDataPack::isFullyCompat( const ZSampling& zrg,
					const DataCharacteristics& dc ) const
{
    return dc == getDataDesc() && zRange().isEqual(zrg,1e-6f);
}


void SeisVolumeDataPack::fillTrace( const TrcKey& tk, SeisTrc& trc ) const
{
    fillTraceInfo( tk, trc.info() );
    fillTraceData( tk, trc.data() );
}


void SeisVolumeDataPack::fillTraceInfo( const TrcKey& tk,
					SeisTrcInfo& sti ) const
{
    const ZSampling zrg = zRange();
    sti.sampling_.start_ = zrg.start_;
    sti.sampling_.step_ = zrg.step_;
    sti.setTrcKey( tk );
    sti.coord_ = tk.getCoord();
    sti.offset_ = 0.f;
}


void SeisVolumeDataPack::fillTraceData( const TrcKey& tk, TraceData& td ) const
{
    DataCharacteristics dc;
    if ( !scaler_ )
	dc = DataCharacteristics( getDataDesc() );

    td.convertTo( dc, false );

    const int trcsz = zRange().nrSteps() + 1;
    td.reSize( trcsz );

    const int globidx = getGlobalIdx( tk );
    if ( globidx < 0 )
	{ td.zero(); return; }

    Array1DImpl<float> copiedtrc( trcsz );
    const int nrcomps = nrComponents();
    for ( int icomp=0; icomp<nrcomps; icomp++ )
    {
	const float* vals = getTrcData( icomp, globidx );
	if ( !vals && !getCopiedTrcData(icomp,globidx,copiedtrc) )
	    { td.zero(); return; }

	float* copiedtrcptr = copiedtrc.getData();
	for ( int isamp=0; isamp<trcsz; isamp++ )
	{
	    const float val = vals ? vals[isamp] : copiedtrcptr
						  ? copiedtrcptr[isamp]
						  : copiedtrc.get( isamp );
	    td.setValue( isamp, val, icomp );
	}
    }
}


// RegularSeisDataPack

RegularSeisDataPack::RegularSeisDataPack( const char* cat,
					  const BinDataDesc* bdd )
    : SeisVolumeDataPack(cat,bdd)
{
    sampling_.init( false );
}


RegularSeisDataPack::~RegularSeisDataPack()
{
    delete rgldpckposinfo_;
}


RegularSeisDataPack* RegularSeisDataPack::clone() const
{
    RegularSeisDataPack* ret = getSimilar();
    if ( getTrcsSampling() )
	ret->setTrcsSampling( new PosInfo::SortedCubeData(*getTrcsSampling()) );

    ret->setZDomain( zDomain() );
    ret->setValUnit( valUnit() );
    ret->setRefNrs( refnrs_ );
    ret->setDataDesc( getDataDesc() );
    if ( getScaler() )
	ret->setScaler( *getScaler() );

    ret->copyFrom( *this );
    return ret;
}


RegularSeisDataPack* RegularSeisDataPack::getSimilar() const
{
    auto* ret = new RegularSeisDataPack( category(), &desc_ );
    ret->setSampling( sampling() );
    return ret;
}


bool RegularSeisDataPack::copyFrom( const RegularSeisDataPack& oth )
{
    componentnames_.setEmpty();
    deepErase( arrays_ );
    for ( int icomp=0; icomp<oth.nrComponents(); icomp++ )
    {
	if ( !addComponent(oth.getComponentName(icomp)) )
	    return false;

	*arrays_[icomp] = oth.data( icomp );
    }

    return true;
}


void RegularSeisDataPack::setTrcsSampling( PosInfo::CubeData* newposdata )
{
    delete rgldpckposinfo_;
    rgldpckposinfo_ = newposdata;
}


const PosInfo::CubeData* RegularSeisDataPack::getTrcsSampling() const
{
    return rgldpckposinfo_;
}


TrcKey RegularSeisDataPack::getTrcKey( int globaltrcidx ) const
{ return sampling_.hsamp_.trcKeyAt( globaltrcidx ); }

bool RegularSeisDataPack::is2D() const
{ return sampling_.is2D(); }


int RegularSeisDataPack::getGlobalIdx( const TrcKey& tk ) const
{
    if ( !sampling_.hsamp_.includes(tk) )
	return -1;

    return mCast(int,sampling_.hsamp_.globalIdx(tk));
}


bool RegularSeisDataPack::addComponent( const char* nm )
{
    if ( !sampling_.isDefined() || sampling_.hsamp_.totalNr()>INT_MAX )
	return false;

    if ( !addArray(sampling_.nrLines(),sampling_.nrTrcs(),sampling_.nrZ()) )
	return false;

    componentnames_.add( nm );
    return true;
}


bool RegularSeisDataPack::addComponentNoInit( const char* nm )
{
    if ( !sampling_.isDefined() || sampling_.hsamp_.totalNr()>INT_MAX )
	return false;

    if ( !addArrayNoInit(sampling_.nrLines(),sampling_.nrTrcs(),
			 sampling_.nrZ()) )
	return false;

    componentnames_.add( nm );
    return true;
}


void RegularSeisDataPack::dumpInfo( StringPairSet& infoset ) const
{
    SeisVolumeDataPack::dumpInfo( infoset );

    const TrcKeySampling& tks = sampling_.hsamp_;
    if ( is2D() )
    {
	infoset.add( sKey::TrcRange(), toUserString(tks.trcRange()) );
    }
    else
    {
	infoset.add( sKey::InlRange(), toUserString(tks.lineRange()) );
	infoset.add( sKey::CrlRange(), toUserString(tks.trcRange()) );
    }

    ZSampling zrg = sampling_.zsamp_;
    if ( !zrg.isUdf() )
    {
	const ZDomain::Info& zinfo = zDomain();
        const int nrdec = zinfo.def_.nrZDecimals( zrg.step_ );
	zrg.scale( zinfo.userFactor() );
	const BufferString keystr = toString( zinfo.getRange() );
	BufferString valstr;
        valstr.addDec( zrg.start_, nrdec )
                .add( " - " ).addDec( zrg.stop_, nrdec )
                .add( " [" ).addDec( zrg.step_, nrdec ).add( "]" );
	infoset.add( keystr, valstr );
    }
}


RefMan<RegularSeisDataPack> RegularSeisDataPack::createDataPackForZSliceRM(
						const BinIDValueSet* bivset,
						const TrcKeyZSampling& tkzs,
						const ZDomain::Info& zinfo,
						const BufferStringSet* names )
{
    if ( !bivset || !tkzs.isDefined() || tkzs.nrZ()!=1 )
	return nullptr;

    RefMan<RegularSeisDataPack> regsdp = new RegularSeisDataPack(
				VolumeDataPack::categoryStr(false,true) );
    regsdp->setSampling( tkzs );
    for ( int idx=1; idx<bivset->nrVals(); idx++ )
    {
	const char* name = names && names->validIdx(idx-1)
			 ? names->get(idx-1).buf()
			 : sKey::EmptyString().buf();
	regsdp->addComponent( name );
	BinIDValueSet::SPos pos;
	BinID bid;
	while ( bivset->next(pos,true) )
	{
	    bivset->get( pos, bid );
	    regsdp->data(idx-1).set( tkzs.hsamp_.inlIdx(bid.inl()),
				     tkzs.hsamp_.crlIdx(bid.crl()), 0,
				     bivset->getVals(pos)[idx] );
	}
    }

    regsdp->setZDomain( zinfo );
    return regsdp;
}


DataPackID RegularSeisDataPack::createDataPackForZSlice(
						const BinIDValueSet* bivset,
						const TrcKeyZSampling& tkzs,
						const ZDomain::Info& zinfo,
						const BufferStringSet* names )
{
    ConstRefMan<RegularSeisDataPack> regsdp =
			createDataPackForZSliceRM( bivset, tkzs, zinfo, names );
    if ( !regsdp )
	return DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).add( regsdp );
    regsdp->ref();
    return regsdp->id();
}


// RandomSeisDataPack

RandomSeisDataPack::RandomSeisDataPack( const char* cat,
					const BinDataDesc* bdd )
    : SeisVolumeDataPack(cat,bdd)
{
}


RandomSeisDataPack::~RandomSeisDataPack()
{
}


TrcKey RandomSeisDataPack::getTrcKey( int trcidx ) const
{
    return path_.validIdx(trcidx) ? path_[trcidx] : TrcKey::udf();
}


bool RandomSeisDataPack::addComponent( const char* nm )
{
    if ( path_.isEmpty() || zsamp_.isUdf() )
	return false;

    if ( !addArray(1,nrTrcs(),zsamp_.nrSteps()+1) )
	return false;

    componentnames_.add( nm );
    return true;
}


int RandomSeisDataPack::getGlobalIdx(const TrcKey& tk) const
{
    return path_.indexOf(tk);
}


RefMan<RandomSeisDataPack> RandomSeisDataPack::createDataPackFromRM(
					const RegularSeisDataPack& regsdp,
					const RandomLineID& rdmlineid,
					const Interval<float>& zrange,
					const BufferStringSet* compnames )
{
    ConstRefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get(rdmlineid);
    if ( !rdmline || regsdp.isEmpty() )
	return nullptr;

    RefMan<RandomSeisDataPack> randsdp = new RandomSeisDataPack(
	    VolumeDataPack::categoryStr(true,false), &regsdp.getDataDesc() );
    randsdp->setRandomLineID( rdmlineid );
    if ( regsdp.getScaler() )
	randsdp->setScaler( *regsdp.getScaler() );

    TrcKeyPath knots, path;
    rdmline->allNodePositions( knots );
    Geometry::RandomLine::getPathBids( knots, path );
    if ( path.isEmpty() )
	return nullptr;

    TrcKeySampling unitsteptks = regsdp.sampling().hsamp_;
    unitsteptks.step_ = BinID( 1, 1 );

    // Remove outer undefined traces at both sides
    int pathidx = path.size()-1;
    while ( pathidx>0 && !unitsteptks.includes(path[pathidx]) )
	path.removeSingle( pathidx-- );

    while ( path.size()>1 && !unitsteptks.includes(path[0]) )
	path.removeSingle( 0 );

    // Auxiliary TrcKeyZSampling to limit z-range and if no overlap at all,
    // preserve one dummy voxel for displaying the proper undefined color.
    TrcKeyZSampling auxtkzs;
    auxtkzs.hsamp_.start_ = path.first().binID();
    auxtkzs.hsamp_.stop_ = path.last().binID();
    auxtkzs.zsamp_.setInterval( zrange );
    if ( !auxtkzs.adjustTo(regsdp.sampling(),true) && path.size()>1 )
	path.removeRange( 1, path.size()-1 );

    randsdp->setPath( path );
    randsdp->setZRange( auxtkzs.zsamp_ );

    const int nrcomps = compnames ? compnames->size() : regsdp.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	const char* compnm = compnames ? compnames->get(idx).buf()
				       : regsdp.getComponentName(idx);

	if ( regsdp.getComponentIdx(compnm,idx) >= 0 )
	{
	    randsdp->addComponent( compnm );
	    Regular2RandomDataCopier copier( *randsdp, regsdp,
					     randsdp->nrComponents()-1 );
	    copier.execute();
	}
    }

    randsdp->setZDomain( regsdp.zDomain() );
    randsdp->setValUnit( regsdp.valUnit() );
    randsdp->setName( regsdp.name() );
    return randsdp;
}


RefMan<RandomSeisDataPack> RandomSeisDataPack::createDataPackFromRM(
					const RegularSeisDataPack& regsdp,
					const TrcKeyPath& path,
					const Interval<float>& zrange,
					const BufferStringSet* compnames )
{
    if ( path.isEmpty()  || regsdp.isEmpty() )
	return nullptr;

    RefMan<RandomSeisDataPack> randsdp = new RandomSeisDataPack(
		VolumeDataPack::categoryStr(true,false),&regsdp.getDataDesc() );
    randsdp->setPath( path );
    if ( regsdp.getScaler() )
	randsdp->setScaler( *regsdp.getScaler() );

    const StepInterval<float>& regzrg = regsdp.zRange();
    StepInterval<float> overlapzrg = regzrg;
    overlapzrg.limitTo( zrange ); // DataPack should be created only for
				  // overlap z-range.
    overlapzrg.start_ = regzrg.atIndex( regzrg.getIndex(overlapzrg.start_) );
    overlapzrg.stop_ = regzrg.atIndex(
				regzrg.indexOnOrAfter(overlapzrg.stop_,0.0) );
    randsdp->setZRange( overlapzrg );

    const int nrcomps = compnames ? compnames->size() : regsdp.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	const char* compnm = compnames ? compnames->get(idx).buf()
				       : regsdp.getComponentName(idx);

	if ( regsdp.getComponentIdx(compnm,idx) >= 0 )
	{
	    randsdp->addComponent( compnm );
	    Regular2RandomDataCopier copier( *randsdp, regsdp,
					     randsdp->nrComponents()-1 );
	    copier.execute();
	}
    }

    randsdp->setZDomain( regsdp.zDomain() );
    randsdp->setValUnit( regsdp.valUnit() );
    randsdp->setName( regsdp.name() );
    return randsdp;
}


DataPackID RandomSeisDataPack::createDataPackFrom(
					const RegularSeisDataPack& regsdp,
					const RandomLineID& rdmlineid,
					const Interval<float>& zrange )
{
    return createDataPackFrom( regsdp, rdmlineid, zrange, nullptr );
}


DataPackID RandomSeisDataPack::createDataPackFrom(
					const RegularSeisDataPack& regsdp,
					const RandomLineID& rdmlineid,
					const Interval<float>& zrange,
					const BufferStringSet* compnames )
{
    ConstRefMan<RandomSeisDataPack> randsdp = createDataPackFromRM( regsdp,
						rdmlineid, zrange, compnames );
    if ( !randsdp )
	return DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).add( randsdp );
    randsdp->ref();
    return randsdp->id();
}


DataPackID RandomSeisDataPack::createDataPackFrom(
					const RegularSeisDataPack& regsdp,
					const TrcKeyPath& path,
					const Interval<float>& zrange )
{
    return createDataPackFrom( regsdp, path, zrange, nullptr );
}


DataPackID RandomSeisDataPack::createDataPackFrom(
					const RegularSeisDataPack& regsdp,
					const TrcKeyPath& path,
					const Interval<float>& zrange,
					const BufferStringSet* compnames )
{
    ConstRefMan<RandomSeisDataPack> randsdp = createDataPackFromRM( regsdp,
						    path, zrange, compnames );
    if ( !randsdp )
	return DataPack::cNoID();

    DPM(DataPackMgr::SeisID()).add(randsdp);
    randsdp->ref();
    return randsdp->id();
}


#define mKeyInl		SeisTrcInfo::toString(SeisTrcInfo::BinIDInl)
#define mKeyCrl		SeisTrcInfo::toString(SeisTrcInfo::BinIDCrl)
#define mKeyCoordX	SeisTrcInfo::toString(SeisTrcInfo::CoordX)
#define mKeyCoordY	SeisTrcInfo::toString(SeisTrcInfo::CoordY)
#define mKeyTrcNr	SeisTrcInfo::toString(SeisTrcInfo::TrcNr)
#define mKeyRefNr	SeisTrcInfo::toString(SeisTrcInfo::RefNr)

// SeisFlatDataPack

SeisFlatDataPack::SeisFlatDataPack( const SeisVolumeDataPack& source, int comp )
    : FlatDataPack(source.category())
    , source_(&source)
    , comp_(comp)
    , zsamp_(source.zRange())
    , rdlid_(source.getRandomLineID())
{
    setName( source_->getComponentName(comp_) );
}


SeisFlatDataPack::~SeisFlatDataPack()
{
}


int SeisFlatDataPack::nrTrcs() const
{
    return source_->nrTrcs();
}


TrcKey SeisFlatDataPack::getTrcKey( int trcidx ) const
{
    return source_->getTrcKey( trcidx );
}


DataPackID SeisFlatDataPack::getSourceID() const
{
    return source_->id();
}


ConstRefMan<SeisVolumeDataPack> SeisFlatDataPack::getSource() const
{
    return source_;
}


int SeisFlatDataPack::getSourceGlobalIdx( const TrcKey& tk ) const
{
    return source_->getGlobalIdx( tk );
}


bool SeisFlatDataPack::is2D() const
{
    return source_->is2D();
}


bool SeisFlatDataPack::dimValuesInInt( const char* keystr ) const
{
    const StringView key( keystr );
    return key==mKeyInl || key==mKeyCrl || key==mKeyTrcNr ||
	   key==sKey::Series();
}


void SeisFlatDataPack::getAltDim0Keys( BufferStringSet& keys ) const
{
    if ( !isVertical() )
	return;

    for ( int idx=0; idx<tiflds_.size(); idx++ )
	keys.add( SeisTrcInfo::toString(tiflds_[idx]) );
}


double SeisFlatDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    if ( !tiflds_.validIdx(ikey) )
	return posdata_.position( true, i0 );

    switch ( tiflds_[ikey] )
    {
	case SeisTrcInfo::BinIDInl:	return SI().transform(
						getCoord(i0,0)).inl();
	case SeisTrcInfo::BinIDCrl:	return SI().transform(
						getCoord(i0,0)).crl();
        case SeisTrcInfo::CoordX:	return getCoord(i0,0).x_;
        case SeisTrcInfo::CoordY:	return getCoord(i0,0).y_;
	case SeisTrcInfo::TrcNr:	return getPath()[i0].trcNr();
	case SeisTrcInfo::RefNr:	return source_->getRefNr(i0);
	default:			return posdata_.position(true,i0);
    }
}


void SeisFlatDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 crd = getCoord( i0, i1 );
    iop.set( mKeyCoordX, crd.x_ );
    iop.set( mKeyCoordY, crd.y_ );
    iop.set( sKey::ZCoord(), (int)(crd.z_*zDomain().userFactor()) );
    mDynamicCastGet(RegularSeisFlatDataPack*,rseisdp,
					   const_cast<SeisFlatDataPack*>(this));
    iop.setYN( FlatView::Viewer::sKeyIsZSlice(),
				    rseisdp ? rseisdp->isVertical() : false );

    if ( is2D() )
    {
	const int trcidx = nrTrcs()==1 ? 0 : i0;
	const TrcKey tk = getTrcKey( trcidx );
	iop.set( mKeyTrcNr, tk.trcNr() );
	const float refnr = source_->getRefNr( trcidx );
	if ( !mIsUdf(refnr) )
	    iop.set( mKeyRefNr, refnr );
    }
    else
    {
	const BinID bid = SI().transform( crd );
	iop.set( mKeyInl, bid.inl() );
	iop.set( mKeyCrl, bid.crl() );
    }
}


const Scaler* SeisFlatDataPack::getScaler() const
{
    return source_->getScaler();
}


const ZDomain::Info& SeisFlatDataPack::zDomain() const
{
    return source_->zDomain();
}


float SeisFlatDataPack::nrKBytes() const
{
    return source_->nrKBytes() / source_->nrComponents();
}


#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start_, rg.stop_, rg.step_ )

void SeisFlatDataPack::setPosData()
{
    const TrcKeyPath& path = getPath();
    if ( path.isEmpty() )
	return;

    const int nrtrcs = path.size();
    float* pos = new float[nrtrcs];
    pos[0] = 0;

    TrcKey prevtk = path[0];
    for ( int idx=1; idx<nrtrcs; idx++ )
    {
	const TrcKey& trckey = path[idx];
	if ( trckey.isUdf() )
	    pos[idx] = mCast(float,(pos[idx-1]));
	else
	{
	    const Coord::DistType dist = prevtk.distTo( trckey );
	    if ( mIsUdf(dist) )
		pos[idx] = mCast(float,(pos[idx-1]));
	    else
	    {
		pos[idx] = mCast(float,(pos[idx-1] + dist));
		prevtk = trckey;
	    }
	}
    }

    posData().setX1Pos( pos, nrtrcs, 0 );
    posData().setRange( false, mStepIntvD(zsamp_) );
}


RandomLineID SeisFlatDataPack::getRandomLineID() const
{
    return rdlid_;
}


#define mIsStraight ((getTrcKey(0).distTo(getTrcKey(nrTrcs()-1))/ \
	posdata_.position(true,nrTrcs()-1))>0.99)

// RegularSeisFlatDataPack

RegularSeisFlatDataPack::RegularSeisFlatDataPack(
		const RegularSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , sampling_(source.sampling())
    , dir_(sampling_.defaultDir())
    , usemulticomps_(comp_==-1)
    , hassingletrace_(nrTrcs()==1)
{
    if ( usemulticomps_ )
	setSourceDataFromMultiCubes();
    else
	setSourceData();
}


RegularSeisFlatDataPack::~RegularSeisFlatDataPack()
{
}


Coord3 RegularSeisFlatDataPack::getCoord( int i0, int i1 ) const
{
    const int trcidx = isVertical() ? (hassingletrace_ ? 0 : i0)
				    : i0*sampling_.nrTrcs()+i1;
    const Coord crd( Survey::GM().toCoord( getTrcKey(trcidx) ) );
    return Coord3( crd.x_, crd.y_,
		   sampling_.zsamp_.atIndex(isVertical() ? i1 : 0) );
}


void RegularSeisFlatDataPack::setTrcInfoFlds()
{
    if ( hassingletrace_ )
	{ pErrMsg( "Trace info fields set for single trace display." ); return;}

    if ( is2D() )
    {
	tiflds_ += SeisTrcInfo::TrcNr;
	tiflds_ += SeisTrcInfo::RefNr;
    }
    else
    {
	if ( dir_ == TrcKeyZSampling::Crl )
	    tiflds_ += SeisTrcInfo::BinIDInl;
	else if ( dir_ == TrcKeyZSampling::Inl )
	    tiflds_ += SeisTrcInfo::BinIDCrl;
    }

    if ( is2D() && !mIsStraight )
	return;

    tiflds_ += SeisTrcInfo::CoordX;
    tiflds_ += SeisTrcInfo::CoordY;
}


const char* RegularSeisFlatDataPack::dimName( bool dim0 ) const
{
    if ( dim0 && hassingletrace_ ) return sKey::Series();
    if ( is2D() )
	return dim0 ? "Distance" : zDomain().userName().getFullString();

    return dim0 ? (dir_==TrcKeyZSampling::Inl ? mKeyCrl : mKeyInl)
		: (dir_==TrcKeyZSampling::Z
			? mKeyCrl : zDomain().userName().getFullString());
}


void RegularSeisFlatDataPack::setSourceDataFromMultiCubes()
{
    const int nrcomps = source_->nrComponents();
    const int nrz = sampling_.zsamp_.nrSteps() + 1;
    posdata_.setRange( true, StepInterval<double>(0,nrcomps-1,1) );
    posdata_.setRange( false, mStepIntvD(sampling_.zsamp_) );

    arr2d_ = new Array2DImpl<float>( nrcomps, nrz );
    for ( int idx=0; idx<nrcomps; idx++ )
	for ( int idy=0; idy<nrz; idy++ )
	    arr2d_->set( idx, idy, source_->data(idx).get(0,0,idy) );
}


void RegularSeisFlatDataPack::setSourceData()
{
    const bool isz = dir_==TrcKeyZSampling::Z;
    if ( !isz )
    {
	path_.setCapacity( source_->nrTrcs(), false );
	for ( int idx=0; idx<source_->nrTrcs(); idx++ )
	    path_ += source_->getTrcKey( idx );
    }

    rdlid_ = source_->getRandomLineID();
    if ( !is2D() )
    {
	const bool isinl = dir_==TrcKeyZSampling::Inl;
	posdata_.setRange( true, isinl ? mStepIntvD(sampling_.hsamp_.crlRange())
				: mStepIntvD(sampling_.hsamp_.inlRange()) );
	posdata_.setRange( false, isz ? mStepIntvD(sampling_.hsamp_.crlRange())
				      : mStepIntvD(sampling_.zsamp_) );
    }
    else
	setPosData();

    const int dim0 = dir_==TrcKeyZSampling::Inl ? 1 : 0;
    const int dim1 = dir_==TrcKeyZSampling::Z ? 1 : 2;
    auto* slice2d = new Array2DSlice<float>( source_->data(comp_) );
    slice2d->setDimMap( 0, dim0 );
    slice2d->setDimMap( 1, dim1 );
    slice2d->setPos( dir_, 0 );
    slice2d->init();
    arr2d_ = slice2d;
    setTrcInfoFlds();
}


float RegularSeisFlatDataPack::getPosDistance( bool dim0, float posfidx ) const
{
    const int posidx = mCast(int,floor(posfidx));
    const float dfposidx = posfidx - posidx;
    if ( dim0 )
    {
	TrcKey idxtrc = getTrcKey( posidx );
	if ( !idxtrc.is2D() )
	{
	    const bool isinl = dir_ == TrcKeyZSampling::Inl;
	    const float dposdistance =
		isinl ? SI().inlDistance() : SI().crlDistance();
	    return (dposdistance*(float)posidx) + (dposdistance*dfposidx);
	}

	double posdistatidx = posdata_.position( true, posidx );
	if ( nrTrcs()>=posidx+1 )
	{
	    double posdistatatnextidx = posdata_.position( true, posidx+1 );
	    posdistatidx += (posdistatatnextidx - posdistatidx)*dfposidx;
	}

	return mCast(float,posdistatidx);
    }

    return mUdf(float);
}


// RandomSeisFlatDataPack

RandomSeisFlatDataPack::RandomSeisFlatDataPack( const RandomSeisDataPack& src,
						int comp )
    : SeisFlatDataPack(src,comp)
    , path_(src.getPath())
{
    rdlid_ = getRandomLineID();
    setSourceData();
}


RandomSeisFlatDataPack::~RandomSeisFlatDataPack()
{
}


int RandomSeisFlatDataPack::getNearestGlobalIdx( const TrcKey& tk ) const
{
    return source_->getNearestGlobalIdx( tk );
}


Coord3 RandomSeisFlatDataPack::getCoord( int i0, int i1 ) const
{
    const Coord coord = path_.validIdx(i0) ? Survey::GM().toCoord(path_[i0])
					   : Coord::udf();
    return Coord3( coord, zsamp_.atIndex(i1) );
}


void RandomSeisFlatDataPack::setTrcInfoFlds()
{
    if ( !mIsStraight )
	return;

    tiflds_ +=	SeisTrcInfo::BinIDInl;
    tiflds_ +=	SeisTrcInfo::BinIDCrl;
    tiflds_ +=	SeisTrcInfo::CoordX;
    tiflds_ +=	SeisTrcInfo::CoordY;
}


void RandomSeisFlatDataPack::setSourceData()
{
    setRegularizedPosData();
    auto* slice2d = new Array2DSlice<float>( source_->data(comp_) );
    slice2d->setDimMap( 0, 1 );
    slice2d->setDimMap( 1, 2 );
    slice2d->setPos( 0, 0 );
    slice2d->init();
    arr2d_ = slice2d;
    setTrcInfoFlds();
}


void RandomSeisFlatDataPack::setRegularizedPosData()
{
    const TrcKeyPath& path = getPath();
    const int nrtrcs = path.size();
    float* pos = new float[nrtrcs];
    pos[0] = 0;
    int firstvalidposidx = -1, lastvalidposidx = -1;
    if ( !path[0].isUdf() )
	firstvalidposidx = 0;

    TrcKey prevtk = path[0];
    for ( int idx=1; idx<nrtrcs; idx++ )
    {
	const TrcKey& trckey = path[idx];
	if ( trckey.isUdf() )
	{
	    if ( firstvalidposidx >= 0 && lastvalidposidx < 0 )
		lastvalidposidx = idx - 1;

	    pos[idx] = mCast(float,(pos[idx-1]));
	}
	else
	{
	    if ( firstvalidposidx < 0 )
		firstvalidposidx = idx;

	    const Coord::DistType dist = prevtk.isUdf() ? mUdf(Coord::DistType)
						    : prevtk.distTo( trckey );
	    if ( mIsUdf(dist) )
		pos[idx] = mCast(float,(pos[idx-1]));
	    else
		pos[idx] = mCast(float,(pos[idx-1] + dist));

	    prevtk = trckey;
	}
    }

    if ( firstvalidposidx >= 0 )
    {
	if ( lastvalidposidx < 0 )
	    lastvalidposidx = nrtrcs - 1;

	const int nrpostoregularize = lastvalidposidx - firstvalidposidx;
	if ( nrpostoregularize > 1 )
	{
	    const Coord::DistType unitdist =
			    pos[lastvalidposidx] / nrpostoregularize;
	    for ( int idx=firstvalidposidx+1; idx<=lastvalidposidx; idx++ )
		pos[idx] = mCast(float,(pos[idx-1] + unitdist));
	}
    }

    posData().setX1Pos( pos, nrtrcs, 0 );
    posData().setRange( false, mStepIntvD(zsamp_) );
}


float RandomSeisFlatDataPack::getPosDistance( bool dim0, float posfidx ) const
{
    const int posidx = mCast(int,floor(posfidx));
    const float dfposidx = posfidx - posidx;
    if ( dim0 )
    {
	double posdistatidx = posdata_.position( true, posidx );
	if ( nrTrcs()>=posidx+1 )
	{
	    double posdistatatnextidx = posdata_.position( true, posidx+1 );
	    posdistatidx += (posdistatatnextidx - posdistatidx)*dfposidx;
	}

	return mCast(float,posdistatidx);
    }

    return mUdf(float);
}
