/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
________________________________________________________________________

-*/

#include "seisdatapack.h"

#include "atomic.h"
#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "binnedvalueset.h"
#include "convmemvalseries.h"
#include "flatposdata.h"
#include "paralleltask.h"
#include "randomlinegeom.h"
#include "seistrc.h"
#include "staticstring.h"
#include "survinfo.h"
#include "survgeom.h"
#include "keystrs.h"

#include <limits.h>


class Regular2RandomDataCopier : public ParallelTask
{
public:
		Regular2RandomDataCopier( RandomSeisDataPack& ransdp,
					  const RegularSeisDataPack& regsdp,
					  int rancompidx )
		    : ransdp_( ransdp )
		    , regsdp_( regsdp )
		    , regidx_( -1 )
		    , ranidx_( rancompidx )
		    , domemcopy_( false )
		    , samplebytes_( sizeof(float) )
		    , srcptr_( 0 )
		    , dstptr_( 0 )
		    , srctrcbytes_( 0 )
		    , dsttrcbytes_( 0 )
		    , srclnbytes_( 0 )
		    , bytestocopy_( 0 )
		    , idzoffset_( 0 )
		{
		    ransdp.getPath( path_ );
		}

    od_int64	nrIterations() const			{ return path_.size(); }

    bool	doPrepare(int nrthreads);
    bool	doWork(od_int64 start,od_int64 stop,int thread);


protected:

    RandomSeisDataPack&		ransdp_;
    const RegularSeisDataPack&	regsdp_;
    TrcKeyPath			path_;
    int				regidx_;
    int				ranidx_;
    bool			domemcopy_;
    int				samplebytes_;
    const unsigned char*	srcptr_;
    unsigned char*		dstptr_;
    od_int64			srctrcbytes_;
    od_int64			dsttrcbytes_;
    od_int64			srclnbytes_;
    od_int64			bytestocopy_;
    int				idzoffset_;
};


bool Regular2RandomDataCopier::doPrepare( int nrthreads )
{
    regidx_ = regsdp_.getComponentIdx( ransdp_.getComponentName(ranidx_),
				       ranidx_ );

    if ( !regsdp_.validComp(regidx_) || !ransdp_.validComp(ranidx_) )
	return false;

    if ( !regsdp_.getZRange().overlaps(ransdp_.getZRange()) )
	return false;

    idzoffset_ = regsdp_.getZRange().nearestIndex( ransdp_.getZRange().start );

    if ( !regsdp_.getZRange().isCompatible(ransdp_.getZRange(),1e-3) )
    {
	pErrMsg( "Unexpected incompatibility of datapack Z-ranges" );
	return false;
    }

    if ( regsdp_.getDataDesc() != ransdp_.getDataDesc() )
	return true;

    srcptr_ = rCast( const unsigned char*, regsdp_.data(regidx_).getData() );
    mDynamicCastGet( const ConvMemValueSeries<float>*, regstorage,
		     regsdp_.data(regidx_).getStorage() );
    if ( regstorage )
    {
	srcptr_ = rCast( const unsigned char*, regstorage->storArr() );
	samplebytes_ = regsdp_.getDataDesc().nrBytes();
    }

    dstptr_ = rCast( unsigned char*, ransdp_.data(ranidx_).getData() );
    mDynamicCastGet( ConvMemValueSeries<float>*, ranstorage,
		     ransdp_.data(ranidx_).getStorage() );
    if ( ranstorage )
	dstptr_ = rCast( unsigned char*, ranstorage->storArr() );

    if ( !srcptr_ || !dstptr_ )
	return true;

    srctrcbytes_ = samplebytes_ * regsdp_.sampling().size(OD::ZSlice);
    srclnbytes_ = srctrcbytes_ * regsdp_.sampling().size(OD::CrosslineSlice);
    dsttrcbytes_ = samplebytes_ * (ransdp_.getZRange().nrSteps()+1);

    bytestocopy_ = dsttrcbytes_;

    if ( idzoffset_ < 0 )
    {
	dstptr_ -= samplebytes_ * idzoffset_;
	bytestocopy_ += samplebytes_ * idzoffset_;
    }
    else
	srcptr_ += samplebytes_ * idzoffset_;

    const int stopoffset = regsdp_.getZRange().nrSteps() -
		regsdp_.getZRange().nearestIndex( ransdp_.getZRange().stop );

    if ( stopoffset < 0 )
	bytestocopy_ += samplebytes_ * stopoffset;

    domemcopy_ = true;
    return true;
}


bool Regular2RandomDataCopier::doWork( od_int64 start, od_int64 stop,
				       int thread )
{
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const TrcKeySampling& hsamp = regsdp_.sampling().hsamp_;
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

	for ( int newidz=0; newidz<=ransdp_.getZRange().nrfSteps(); newidz++ )
	{
	    const int oldidz = newidz + idzoffset_;
	    const float val =
		regsdp_.data(regidx_).info().validPos(inlidx,crlidx,oldidz) ?
		regsdp_.data(regidx_).get(inlidx,crlidx,oldidz) : mUdf(float);

	    ransdp_.data(ranidx_).set( 0, idx, newidz, val );
	}
    }

    return true;
}


//=============================================================================
// SeisVolumeDataPack

SeisVolumeDataPack::SeisVolumeDataPack( const char* cat, const BinDataDesc* bdd)
    : VolumeDataPack(cat,bdd)
{
}


SeisVolumeDataPack::SeisVolumeDataPack( const SeisVolumeDataPack& oth )
    : VolumeDataPack(oth)
{
    copyClassData( oth );
}


SeisVolumeDataPack::~SeisVolumeDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers( SeisVolumeDataPack, VolumeDataPack )


bool SeisVolumeDataPack::isFullyCompat( const z_steprg_type& zrg,
					const DataCharacteristics& dc ) const
{
    const auto myzrg = getZRange();
    return dc == getDataDesc() && myzrg.isEqual(zrg,1e-6f);
}


void SeisVolumeDataPack::fillTrace( const TrcKey& trcky, SeisTrc& trc ) const
{
    fillTraceInfo( trcky, trc.info() );
    fillTraceData( trcky, trc.data() );
}


void SeisVolumeDataPack::fillTraceInfo( const TrcKey& tk,
					SeisTrcInfo& ti ) const
{
    const auto zrg = getZRange();
    ti.sampling_.start = zrg.start;
    ti.sampling_.step = zrg.step;
    ti.trcKey() = tk;
    ti.coord_ = tk.getCoord();
    ti.offset_ = 0.f;
}


void SeisVolumeDataPack::fillTraceData( const TrcKey& trcky,
					TraceData& td ) const
{
    DataCharacteristics dc;
    if ( !scaler_ )
	dc = DataCharacteristics( getDataDesc() );
    td.convertTo( dc, false );

    const int trcsz = getZRange().nrSteps() + 1;
    td.reSize( trcsz );

    const int globidx = globalIdx( trcky );
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



//=============================================================================

// RegularSeisDataPack

RegularSeisDataPack::RegularSeisDataPack( const char* cat,
					  const BinDataDesc* bdd )
    : SeisVolumeDataPack(cat,bdd)
    , sampling_(false) // MUST be false, otherwise program startup issues
    , lcd_(0)
{
}


RegularSeisDataPack::RegularSeisDataPack( const RegularSeisDataPack& oth )
    : SeisVolumeDataPack(oth)
    , sampling_(oth.sampling_)
    , lcd_(0)
{
    copyClassData( oth );
}


RegularSeisDataPack::~RegularSeisDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignment( RegularSeisDataPack, SeisVolumeDataPack )

void RegularSeisDataPack::copyClassData( const RegularSeisDataPack& oth )
{
    sampling_ = oth.sampling_;
    lcd_ = oth.lcd_ ? new oth.lcd_->clone() : nullptr;
}


Monitorable::ChangeType RegularSeisDataPack::compareClassData(
					const RegularSeisDataPack& oth ) const
{
    if ( sampling_ != oth.sampling_ )
	return cEntireObjectChange();

    if ( !lcd_ )
	return oth.lcd_ ? cEntireObjectChange() : cNoChange();
    if ( !oth.lcd_ )
	return cEntireObjectChange();

    return *lcd_ == *oth.lcd_ ? cNoChange() : cEntireObjectChange();
}


RegularSeisDataPack* RegularSeisDataPack::getSimilar() const
{
    RegularSeisDataPack* ret = new RegularSeisDataPack( category(), &desc_ );
    ret->setSampling( sampling() );
    return ret;
}


void RegularSeisDataPack::setTrcsSampling( LineCollData* newlcd )
{
    lcd_ = newlcd;
}


const CubeData* RegularSeisDataPack::trcsSampling() const
{
    return lcd_.ptr();
}


PosInfo::LineCollData* RegularSeisDataPack::getTrcPositions() const
{
    cd.setEmpty();
    if ( lcd_ )
	return lcd_->clone();
    else if ( !sampling_.isDefined() )
	return nullptr;
    else
	return LineCollData::create( FullSubSel(sampling_) );
}


TrcKey RegularSeisDataPack::getTrcKey( int globaltrcidx ) const
{
    return sampling_.hsamp_.trcKeyAt( globaltrcidx );
}


bool RegularSeisDataPack::is2D() const
{
    return sampling_.hsamp_.is2D();
}


int RegularSeisDataPack::globalIdx( const TrcKey& tk ) const
{
    if ( !sampling_.hsamp_.includes(tk) )
	return -1;

    return mCast(int,sampling_.hsamp_.globalIdx(tk));
}


bool RegularSeisDataPack::addComponent( const char* nm, bool initvals )
{
    if ( !sampling_.isDefined() || sampling_.hsamp_.totalNr()>INT_MAX )
	return false;

    if ( !addArray(sampling_.nrLines(),sampling_.nrTrcs(),sampling_.nrZ(),
		   initvals) )
	return false;

    componentnames_.add( nm );
    return true;
}


void RegularSeisDataPack::doDumpInfo( IOPar& par ) const
{
    VolumeDataPack::doDumpInfo( par );

    const TrcKeySampling& tks = sampling_.hsamp_;
    if ( is2D() )
	par.set( sKey::TrcRange(), tks.start_.trcNr(), tks.stop_.trcNr(),
				   tks.step_.trcNr() );
    else
    {
	par.set( sKey::InlRange(), tks.start_.lineNr(), tks.stop_.lineNr(),
				   tks.step_.lineNr() );
	par.set( sKey::CrlRange(), tks.start_.trcNr(), tks.stop_.trcNr(),
				   tks.step_.trcNr() );
    }

    par.set( sKey::ZRange(), sampling_.zsamp_.start, sampling_.zsamp_.stop,
			     sampling_.zsamp_.step );
}


DataPack::ID RegularSeisDataPack::createDataPackForZSlice(
						const BinnedValueSet* bivset,
						const TrcKeyZSampling& tkzs,
						const ZDomain::Info& zinfo,
						const BufferStringSet* names )
{
    if ( !bivset || !tkzs.isDefined() || tkzs.nrZ()!=1 )
	return DataPack::cNoID();

    RegularSeisDataPack* regsdp = new RegularSeisDataPack(
				    VolumeDataPack::categoryStr(false,true) );
    regsdp->setSampling( tkzs );
    for ( int idx=1; idx<bivset->nrVals(); idx++ )
    {
	const char* name = names && names->validIdx(idx-1)
			    ? names->get(idx-1).str() : OD::EmptyString();
	regsdp->addComponent( name, true );
	BinnedValueSet::SPos pos;
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
    DPM(DataPackMgr::SeisID()).add( regsdp );
    return regsdp->id();
}


// RandomSeisDataPack
RandomSeisDataPack::RandomSeisDataPack( const char* cat,
					const BinDataDesc* bdd )
    : SeisVolumeDataPack(cat,bdd)
    , rdlid_(-1)
    , path_(*new TrcKeyPath)
{
}


RandomSeisDataPack::RandomSeisDataPack( const RandomSeisDataPack& oth )
    : SeisVolumeDataPack(oth)
    , path_(*new TrcKeyPath)
{
    copyClassData( oth );
}


RandomSeisDataPack::~RandomSeisDataPack()
{
    sendDelNotif();
    delete &path_;
}


mImplMonitorableAssignment( RandomSeisDataPack, SeisVolumeDataPack )


void RandomSeisDataPack::copyClassData( const RandomSeisDataPack& oth )
{
    rdlid_ = oth.rdlid_;
    path_ = oth.path_;
    zsamp_ = oth.zsamp_;
}


Monitorable::ChangeType RandomSeisDataPack::compareClassData(
					const RandomSeisDataPack& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	rdlid_ == oth.rdlid_
     && path_ == oth.path_
     && zsamp_ == oth.zsamp_ );
}


RandomSeisDataPack* RandomSeisDataPack::getSimilar() const
{
    RandomSeisDataPack* ret = new RandomSeisDataPack( category(), &desc_ );
    ret->setRandomLineID( rdlid_ );
    ret->setPath( path_ );
    ret->setZRange( zsamp_ );
    return ret;
}


void RandomSeisDataPack::getTrcKey( int trcidx, TrcKey& tk ) const
{
    tk = path_.validIdx(trcidx) ? path_[trcidx] : TrcKey::udf();
}


bool RandomSeisDataPack::addComponent( const char* nm, bool initvals )
{
    if ( path_.isEmpty() || zsamp_.isUdf() )
	return false;

    if ( !addArray(1,nrPositions(),zsamp_.nrSteps()+1,initvals) )
	return false;

    componentnames_.add( nm );
    return true;
}


void RandomSeisDataPack::setPath( const TrcKeyPath& pth )
{
    path_ = pth;
}


void RandomSeisDataPack::getPath( TrcKeyPath& pth ) const
{
    pth = path_;
}


int RandomSeisDataPack::globalIdx( const TrcKey& tk ) const
{
    return path_.indexOf(tk);
}


void RandomSeisDataPack::setRandomLineID( int rdlid,
					  const TypeSet<BinID>* rdlsubpath )
{
    ConstRefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get( rdlid );
    if ( !rdmline )
	return;

    if ( rdlsubpath )
    {
	path_.setSize( rdlsubpath->size(), TrcKey::udf() );
	for ( int idx=0; idx<rdlsubpath->size(); idx++ )
	    path_[idx] = TrcKey( (*rdlsubpath)[idx] );
	rdlid_ = rdlid;
	return;
    }

    TypeSet<BinID> knots, rdlpath;
    rdmline->getNodePositions( knots );
    rdmline->getPathBids( knots, rdlpath );
    path_.setSize( rdlpath.size(), TrcKey::udf() );
    for ( int idx=0; idx<rdlpath.size(); idx++ )
	path_[idx] = TrcKey( rdlpath[idx] );
    rdlid_ = rdlid;
}


DataPack::ID RandomSeisDataPack::createDataPackFrom(
					const RegularSeisDataPack& regsdp,
					int rdmlineid,
					const z_rg_type& zrange,
					const BufferStringSet* compnames,
					const TypeSet<BinID>* subpath )
{
    ConstRefMan<Geometry::RandomLine> rdmline = Geometry::RLM().get(rdmlineid);
    if ( !rdmline || regsdp.isEmpty() )
	return DataPack::cNoID();

    RandomSeisDataPack* randsdp = new RandomSeisDataPack(
	    VolumeDataPack::categoryStr(true,false), &regsdp.getDataDesc() );
    randsdp->setRandomLineID( rdmlineid );
    if ( regsdp.getScaler() )
	randsdp->setScaler( *regsdp.getScaler() );

    TrcKeyPath path;
    randsdp->getPath( path );

    TrcKeySampling unitsteptks = regsdp.sampling().hsamp_;
    unitsteptks.step_ = BinID( 1, 1 );

    // Remove outer undefined traces at both sides
    int pathidx = path.size()-1;
    while ( pathidx>0 && !unitsteptks.includes(path[pathidx]) )
	path.removeSingle( pathidx-- );

    while ( path.size()>1 && !unitsteptks.includes(path[0]) )
	path.removeSingle( 0 );

    if ( subpath )
    {
	const int subpathstopidx = path.indexOf( TrcKey(subpath->last()) );
	if ( subpathstopidx>=0 )
	    path.removeRange( subpathstopidx, path.size()-1 );
	const int subpathstartidx = path.indexOf( TrcKey(subpath->first()) );
	if ( subpathstartidx>=0 )
	    path.removeRange( 0, subpathstartidx );
    }
    // Auxiliary TrcKeyZSampling to limit z-range and if no overlap at all,
    // preserve one dummy voxel for displaying the proper undefined color.
    TrcKeyZSampling auxtkzs;
    auxtkzs.hsamp_.start_ = path.first().binID();
    auxtkzs.hsamp_.stop_ = path.last().binID();
    auxtkzs.zsamp_ = zrange;
    if ( !auxtkzs.adjustTo(regsdp.sampling(),true) && path.size()>1 )
	 path.removeRange( 1, path.size()-1 );

    randsdp->setZRange( auxtkzs.zsamp_ );

    const int nrcomps = compnames ? compnames->size() : regsdp.nrComponents();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	const char* compnm = compnames ? compnames->get(idx).buf()
				       : regsdp.getComponentName(idx);

	if ( regsdp.getComponentIdx(compnm,idx) >= 0 )
	{
	    randsdp->addComponent( compnm, true );
	    Regular2RandomDataCopier copier( *randsdp, regsdp,
					     randsdp->nrComponents()-1 );
	    copier.execute();
	}
    }

    randsdp->setZDomain( regsdp.zDomain() );
    randsdp->setName( regsdp.name() );
    DPM(DataPackMgr::SeisID()).add( randsdp );
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
{
    DPM(DataPackMgr::SeisID()).add(const_cast<SeisVolumeDataPack*>(&source));
    setName( source_->getComponentName(comp_) );
}


SeisFlatDataPack::SeisFlatDataPack( const SeisFlatDataPack& oth )
    : FlatDataPack(oth.source_->category())
    , source_(oth.source_)
{
    copyClassData( oth );
    setName( source_->getComponentName(comp_) );
}


SeisFlatDataPack::~SeisFlatDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignment( SeisFlatDataPack, FlatDataPack )


void SeisFlatDataPack::copyClassData( const SeisFlatDataPack& oth )
{
    source_ = oth.source_;
    comp_ = oth.comp_;
}


Monitorable::ChangeType SeisFlatDataPack::compareClassData(
					const SeisFlatDataPack& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	source_.ptr() == oth.source_.ptr() && comp_ == oth.comp_ );
}


bool SeisFlatDataPack::dimValuesInInt( const char* keystr ) const
{
    const FixedString key( keystr );
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
						getCoord(i0,0).getXY()).inl();
	case SeisTrcInfo::BinIDCrl:	return SI().transform(
						getCoord(i0,0).getXY()).crl();
	case SeisTrcInfo::CoordX:	return getCoord(i0,0).x_;
	case SeisTrcInfo::CoordY:	return getCoord(i0,0).y_;
	case SeisTrcInfo::TrcNr:	return path()[i0].trcNr();
	case SeisTrcInfo::RefNr:	return source_->getRefNr(i0);
	default:			return posdata_.position(true,i0);
    }
}


void SeisFlatDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 crd = getCoord( i0, i1 );
    iop.set( mKeyCoordX, crd.x_ );
    iop.set( mKeyCoordY, crd.y_ );
    iop.set( sKey::ZCoord(), crd.z_ * zDomain().userFactor() );

    if ( is2D() )
    {
	const int trcidx = nrPositions()==1 ? 0 : i0;
	const TrcKey& tk = getTrcKey( trcidx );
	iop.set( mKeyTrcNr, tk.trcNr() );
	iop.set( mKeyRefNr, source_->getRefNr(trcidx) );
    }
    else
    {
	const BinID bid = SI().transform( crd.getXY() );
	iop.set( mKeyInl, bid.inl() );
	iop.set( mKeyCrl, bid.crl() );
    }
}


float SeisFlatDataPack::gtNrKBytes() const
{
    return source_->nrKBytes() / source_->nrComponents();
}


#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

void SeisFlatDataPack::setPosData()
{
    const TrcKeyPath& path = path();
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
	    const float dist = (float)prevtk.distTo( trckey );
	    if ( mIsUdf(dist) )
		pos[idx] = pos[idx-1];
	    else
	    {
		pos[idx] = pos[idx-1] + dist;
		prevtk = trckey;
	    }
	}
    }

    posData().setX1Pos( pos, nrtrcs, 0 );
    posData().setRange( false, mStepIntvD(zSamp()) );
}



#define mIsStraight ((getTrcKey(0).distTo(getTrcKey(nrPositions()-1))/ \
	posdata_.position(true,nrPositions()-1))>0.99)

RegularFlatDataPack::RegularFlatDataPack(
		const RegularSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , usemulticomps_(comp_==-1)
    , hassingletrace_(nrPositions()==1)
{
    if ( usemulticomps_ )
	setSourceDataFromMultiCubes();
    else
	setSourceData();
}


RegularFlatDataPack::RegularFlatDataPack( const RegularFlatDataPack& oth )
    : SeisFlatDataPack( oth )
{
    copyClassData( oth );
}


RegularFlatDataPack::~RegularFlatDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignment( RegularFlatDataPack, SeisFlatDataPack )


void RegularFlatDataPack::copyClassData( const RegularFlatDataPack& oth )
{
    path_ = oth.path_;
    usemulticomps_ = oth.usemulticomps_;
    hassingletrace_ = oth.hassingletrace_;
}


Monitorable::ChangeType RegularFlatDataPack::compareClassData(
					const RegularFlatDataPack& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	path_ == oth.path_
    &&	usemulticomps_ == oth.usemulticomps_
    &&	hassingletrace_ == oth.hassingletrace_ );
}


Coord3 RegularFlatDataPack::getCoord( int i0, int i1 ) const
{
    const bool isvertical = dir() != OD::ZSlice;
    const int trcidx = isvertical ? (hassingletrace_ ? 0 : i0)
				  : i0*sampling().nrTrcs()+i1;
    const Coord c = getTrcKey(trcidx).getCoord();
    return Coord3( c.x_, c.y_, sampling().zsamp_.atIndex(isvertical ? i1 : 0) );
}


void RegularFlatDataPack::setTrcInfoFlds()
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
	if ( dir() == OD::CrosslineSlice )
	    tiflds_ += SeisTrcInfo::BinIDInl;
	else if ( dir() == OD::InlineSlice )
	    tiflds_ += SeisTrcInfo::BinIDCrl;
    }

    if ( is2D() && !mIsStraight )
	return;

    tiflds_ += SeisTrcInfo::CoordX;
    tiflds_ += SeisTrcInfo::CoordY;
}


const char* RegularFlatDataPack::dimName( bool dim0 ) const
{
    if ( dim0 && hassingletrace_ )
	return sKey::Series();

    const bool is2d = is2D();
    if ( dim0 )
	return is2d ? "Distance"
		    : dir()==OD::InlineSlice ? mKeyCrl : mKeyInl;
    else if ( dir()==OD::ZSlice )
	return mKeyCrl;

    mDeclStaticString( ret );
    ret.set( toString(zDomain().userName()) );
    return ret.buf();
}


float RegularFlatDataPack::getPosDistance( bool dim0, float posfidx ) const
{
    const int posidx = mCast(int,floor(posfidx));
    const float dfposidx = posfidx - posidx;
    if ( dim0 )
    {
	TrcKey idxtrc = getTrcKey( posidx );
	if ( !idxtrc.is2D() )
	{
	    const bool isinl = dir() == OD::InlineSlice;
	    const float dposdistance =
		isinl ? SI().inlDistance() : SI().crlDistance();
	    return (dposdistance*(float)posidx) + (dposdistance*dfposidx);
	}

	double posdistatidx = posdata_.position( true, posidx );
	if ( nrPositions()>=posidx+1 )
	{
	    double posdistatatnextidx = posdata_.position( true, posidx+1 );
	    posdistatidx += (posdistatatnextidx - posdistatidx)*dfposidx;
	}

	return mCast(float,posdistatidx);
    }

    return mUdf(float);
}


void RegularFlatDataPack::setSourceDataFromMultiCubes()
{
    const int nrcomps = source_->nrComponents();
    const int nrz = sampling().zsamp_.nrSteps() + 1;
    posdata_.setRange( true, StepInterval<double>(0,nrcomps-1,1) );
    posdata_.setRange( false, mStepIntvD(sampling().zsamp_) );

    arr2d_ = new Array2DImpl<float>( nrcomps, nrz );
    for ( int idx=0; idx<nrcomps; idx++ )
	for ( int idy=0; idy<nrz; idy++ )
	    arr2d_->set( idx, idy, source_->data(idx).get(0,0,idy) );
}


void RegularFlatDataPack::setSourceData()
{
    const bool isz = dir()==OD::ZSlice;
    if ( !isz )
    {
	path_.setCapacity( source_->nrPositions(), false );
	for ( int idx=0; idx<source_->nrPositions(); idx++ )
	    path_ += source_->getTrcKey( idx );
    }

    if ( !is2D() )
    {
	const bool isinl = dir()==OD::InlineSlice;
	posdata_.setRange(
		true, isinl ? mStepIntvD(sampling().hsamp_.crlRange())
			    : mStepIntvD(sampling().hsamp_.inlRange()) );
	posdata_.setRange( false, isz ? mStepIntvD(sampling().hsamp_.crlRange())
				      : mStepIntvD(sampling().zsamp_) );
    }
    else
	setPosData();

    const dim_idx_type dim0 = dir()==OD::InlineSlice ? 1 : 0;
    const dim_idx_type dim1 = dir()==OD::ZSlice ? 1 : 2;
    Array2DSlice<float>* slice2d
		= new Array2DSlice<float>(source_->data(comp_));
    slice2d->setDimMap( 0, dim0 );
    slice2d->setDimMap( 1, dim1 );
    dim_idx_type dirval = (dim_idx_type)dir();
    slice2d->setPos( dirval, 0 );
    slice2d->init();
    arr2d_ = slice2d;
    setTrcInfoFlds();
}



RandomFlatDataPack::RandomFlatDataPack(
		const RandomSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
{
    setSourceData();
}


RandomFlatDataPack::RandomFlatDataPack( const RandomFlatDataPack& oth )
    : SeisFlatDataPack( oth )
{
    copyClassData( oth );
}


RandomFlatDataPack::~RandomFlatDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers( RandomFlatDataPack, SeisFlatDataPack )


Coord3 RandomFlatDataPack::getCoord( int i0, int i1 ) const
{
    const Coord coord = path().validIdx(i0) ? path().get(i0).getCoord()
					    : Coord::udf();
    return Coord3( coord, zSamp().atIndex(i1) );
}


void RandomFlatDataPack::setTrcInfoFlds()
{
    if ( !mIsStraight )
	return;

    tiflds_ +=	SeisTrcInfo::BinIDInl;
    tiflds_ +=	SeisTrcInfo::BinIDCrl;
    tiflds_ +=	SeisTrcInfo::CoordX;
    tiflds_ +=	SeisTrcInfo::CoordY;
}


void RandomFlatDataPack::setSourceData()
{
    setPosData();
    Array2DSlice<float>* slice2d
		= new Array2DSlice<float>(source_->data(comp_));
    slice2d->setDimMap( 0, 1 );
    slice2d->setDimMap( 1, 2 );
    slice2d->setPos( 0, 0 );
    slice2d->init();
    arr2d_ = slice2d;
    setTrcInfoFlds();
}


void RandomFlatDataPack::setPosData()
{
    const TrcKeyPath& path = path();
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

	    const double dist = prevtk.isUdf() ? mUdf(double)
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
	    const double unitdist = pos[lastvalidposidx] / nrpostoregularize;
	    for ( int idx=firstvalidposidx+1; idx<=lastvalidposidx; idx++ )
		pos[idx] = mCast(float,(pos[idx-1] + unitdist));
	}
    }

    posData().setX1Pos( pos, nrtrcs, 0 );
    posData().setRange( false, mStepIntvD(zSamp()) );
}


float RandomFlatDataPack::getPosDistance( bool dim0, float posfidx ) const
{
    const int posidx = mCast(int,floor(posfidx));
    const float dfposidx = posfidx - posidx;
    if ( dim0 )
    {
	double posdistatidx = posdata_.position( true, posidx );
	if ( nrPositions()>=posidx+1 )
	{
	    double posdistatatnextidx = posdata_.position( true, posidx+1 );
	    posdistatidx += (posdistatatnextidx - posdistatidx)*dfposidx;
	}

	return mCast(float,posdistatidx);
    }

    return mUdf(float);
}
