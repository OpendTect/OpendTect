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
#include "cubedata.h"
#include "flatposdata.h"
#include "fullsubsel.h"
#include "keystrs.h"
#include "paralleltask.h"
#include "randomlinegeom.h"
#include "seistrc.h"
#include "staticstring.h"
#include "survinfo.h"
#include "survgeom.h"
#include "trckey.h"
#include "trckeyzsampling.h"

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

    if ( !regsdp_.zRange().overlaps(ransdp_.zRange()) )
	return false;

    idzoffset_ = regsdp_.zRange().nearestIndex( ransdp_.zRange().start );

    if ( !regsdp_.zRange().isCompatible(ransdp_.zRange(),1e-3) )
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

    const auto ransdpzrg = ransdp_.zRange();

    srctrcbytes_ = samplebytes_ * regsdp_.subSel().nrZ();
    srclnbytes_ = srctrcbytes_ * regsdp_.horSubSel().trcNrRange().nrSteps()+1;
    dsttrcbytes_ = samplebytes_ * (ransdpzrg.nrSteps() + 1);

    bytestocopy_ = dsttrcbytes_;

    if ( idzoffset_ >= 0 )
	srcptr_ += samplebytes_ * idzoffset_;
    else
    {
	dstptr_ -= samplebytes_ * idzoffset_;
	bytestocopy_ += samplebytes_ * idzoffset_;
    }

    const int stopoffset = regsdp_.zRange().nrSteps() -
		regsdp_.zRange().nearestIndex( ransdpzrg.stop );

    if ( stopoffset < 0 )
	bytestocopy_ += samplebytes_ * stopoffset;

    domemcopy_ = true;
    return true;
}


bool Regular2RandomDataCopier::doWork( od_int64 start, od_int64 stop,
				       int thread )
{
    const auto& hss = regsdp_.horSubSel();
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const auto bid( path_[idx].binID() );
	if ( !hss.includes(bid) )
	    continue;

	const auto lidx = hss.idx4LineNr( bid.inl() );
	const auto tidx = hss.idx4TrcNr( bid.crl() );

	if ( domemcopy_ )
	{
	    const unsigned char* srcptr = srcptr_ + lidx*srclnbytes_
						  + tidx*srctrcbytes_;
	    unsigned char* dstptr = dstptr_ + idx*dsttrcbytes_;
	    OD::sysMemCopy( dstptr, srcptr, bytestocopy_ );
	    continue;
	}

	const auto nrsteps = ransdp_.zRange().nrfSteps();
	for ( int newidz=0; newidz<=nrsteps; newidz++ )
	{
	    const int oldidz = newidz + idzoffset_;
	    const float val =
		regsdp_.data(regidx_).info().validPos(lidx,tidx,oldidz) ?
		regsdp_.data(regidx_).get(lidx,tidx,oldidz) : mUdf(float);

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
    return dc == getDataDesc() && zRange().isEqual(zrg,1e-6f);
}


void SeisVolumeDataPack::fillTrace( const BinID& bid, SeisTrc& trc ) const
{
    fillTraceInfo( bid, trc.info() );
    fillTraceData( bid, trc.data() );
}


void SeisVolumeDataPack::fillTrace( const Bin2D& b2d, SeisTrc& trc ) const
{
    fillTraceInfo( b2d, trc.info() );
    fillTraceData( b2d, trc.data() );
}


void SeisVolumeDataPack::fillTraceInfo( const BinID& bid,
					SeisTrcInfo& ti ) const
{
    fillTraceInfo( TrcKey(bid), ti );
}


void SeisVolumeDataPack::fillTraceInfo( const Bin2D& b2d,
					SeisTrcInfo& ti ) const
{
    fillTraceInfo( TrcKey(b2d), ti );
}


void SeisVolumeDataPack::fillTraceInfo( const TrcKey& tk,
					SeisTrcInfo& ti ) const
{
    const auto zrg = zRange();
    ti.sampling_.start = zrg.start;
    ti.sampling_.step = zrg.step;
    ti.setTrcKey( tk );
    ti.coord_ = tk.getCoord();
    ti.offset_ = 0.f;
}


void SeisVolumeDataPack::fillTraceData( const BinID& bid,
					TraceData& td ) const
{
    fillTraceData( globalIdx(bid), td );
}


void SeisVolumeDataPack::fillTraceData( const Bin2D& b2d,
					TraceData& td ) const
{
    fillTraceData( globalIdx(b2d), td );
}


void SeisVolumeDataPack::fillTraceData( glob_idx_type globidx,
					TraceData& td ) const
{
    DataCharacteristics dc;
    if ( !scaler_ )
	dc = DataCharacteristics( getDataDesc() );
    td.convertTo( dc, false );

    const int trcsz = zRange().nrSteps() + 1;
    td.reSize( trcsz );

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


RegularSeisDataPack::RegularSeisDataPack( const RegularSeisDataPack& oth )
    : SeisVolumeDataPack(oth)
{
    copyClassData( oth );
}


RegularSeisDataPack::~RegularSeisDataPack()
{
    sendDelNotif();
    delete subsel_;
    delete lcd_;
}


mImplMonitorableAssignment( RegularSeisDataPack, SeisVolumeDataPack )

void RegularSeisDataPack::copyClassData( const RegularSeisDataPack& oth )
{
    subsel_ = oth.subSelClone();
    lcd_ = oth.lcd_ ? oth.lcd_->clone() : nullptr;
}


RegularSeisDataPack::GeomSubSel* RegularSeisDataPack::subSelClone() const
{
    return subsel_ ? (GeomSubSel*)subsel_->duplicate() : nullptr;
}


Monitorable::ChangeType RegularSeisDataPack::compareClassData(
					const RegularSeisDataPack& oth ) const
{
    if ( !subsel_ && !oth.subsel_)
	return cNoChange();
    if ( !subsel_ || !oth.subsel_ || *subsel_ != *oth.subsel_ )
	return cEntireObjectChange();

    if ( !lcd_ )
	return oth.lcd_ ? cEntireObjectChange() : cNoChange();
    if ( !oth.lcd_ )
	return cEntireObjectChange();

    return *lcd_ == *oth.lcd_ ? cNoChange() : cEntireObjectChange();
}


bool RegularSeisDataPack::is2D() const
{
    return subSel().is2D();
}


RegularSeisDataPack* RegularSeisDataPack::getSimilar() const
{
    RegularSeisDataPack* ret = new RegularSeisDataPack( category(), &desc_ );
    ret->setSubSel( subSel() );
    return ret;
}


void RegularSeisDataPack::setTracePositions( LineCollData* newlcd )
{
    lcd_ = newlcd;
}


const PosInfo::LineCollData* RegularSeisDataPack::tracePositions() const
{
    return lcd_;
}


Survey::GeomSubSel& RegularSeisDataPack::subSel()
{
    if ( !subsel_ )
	{ pErrMsg("No subsel"); subsel_ = new CubeSubSel(); }
    return *subsel_;
}


const Survey::GeomSubSel& RegularSeisDataPack::subSel() const
{
    return mSelf().subSel();
}


void RegularSeisDataPack::setSubSel( const GeomSubSel& newss )
{
    if ( !subsel_ || *subsel_ != newss )
    {
	// lock for write
	deleteAndZeroPtr( subsel_ );
	subsel_ = (GeomSubSel*)newss.duplicate();
	// send notif
    }
}


void RegularSeisDataPack::getTKZS( TrcKeyZSampling& tkzs ) const
{
    if ( !subsel_ )
	tkzs.init();
    else
	tkzs = TrcKeyZSampling( *subsel_ );
}



void RegularSeisDataPack::getTKS( TrcKeySampling& tks ) const
{
    if ( !subsel_ )
	tks.init();
    else
	tks = TrcKeySampling( horSubSel() );
}


void RegularSeisDataPack::setSampling( const TrcKeyZSampling& tkzs )
{
    if ( tkzs.is2D() )
	setSubSel( LineSubSel(tkzs) );
    else
	setSubSel( CubeSubSel(tkzs) );
}


PosInfo::LineCollData* RegularSeisDataPack::getTrcPositions() const
{
    if ( lcd_ )
	return lcd_->clone();
    else if ( !subsel_ )
	return nullptr;
    else
	return LineCollData::create( Survey::FullSubSel(*subsel_) );
}


void RegularSeisDataPack::gtTrcKey( glob_idx_type gidx, TrcKey& tk ) const
{
    if ( is2D() )
	tk.setPos( horSubSel().asLineHorSubSel()->atGlobIdx(gidx) );
    else
	tk.setPos( horSubSel().asCubeHorSubSel()->atGlobIdx(gidx) );
}


RegularSeisDataPack::glob_idx_type
RegularSeisDataPack::gtGlobalIdx( const TrcKey& tk ) const
{
    return is2D() ? horSubSel().asLineHorSubSel()->globIdx( tk.trcNr() )
		  : horSubSel().asCubeHorSubSel()->globIdx( tk.binID() );
}


bool RegularSeisDataPack::addComponent( const char* nm, bool initvals )
{
    if ( !subsel_ )
	return false;

    const auto nrlines = is2D() ? 1 : horSubSel().asCubeHorSubSel()->nrInl();
    const auto nrtrcs = horSubSel().trcNrRange().nrSteps() + 1;
    if ( !addArray(nrlines,nrtrcs,subsel_->nrZ(),initvals) )
	return false;

    componentnames_.add( nm );
    return true;
}


void RegularSeisDataPack::doDumpInfo( IOPar& par ) const
{
    VolumeDataPack::doDumpInfo( par );
    if ( !subsel_ )
	return;

    const auto& hss = horSubSel();
    if ( is2D() )
	par.set( sKey::TrcRange(), hss.trcNrStart(), hss.trcNrStop(),
				   hss.trcNrStep() );
    else
    {
	const auto& chss = *hss.asCubeHorSubSel();
	par.set( sKey::InlRange(), chss.inlStart(), chss.inlStop(),
				   chss.inlStep() );
	par.set( sKey::CrlRange(), chss.crlStart(), chss.crlStop(),
				   chss.crlStep() );
    }

    const auto zrg = subsel_->zRange();
    par.set( sKey::ZRange(), zrg.start, zrg.stop, zrg.step );
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


bool RandomSeisDataPack::is2D() const
{
    return !path_.isEmpty() && path_.first().is2D();
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


RandomSeisDataPack::glob_size_type RandomSeisDataPack::nrPositions() const
{
    return path_.size();
}


void RandomSeisDataPack::gtTrcKey( glob_idx_type idx, TrcKey& tk ) const
{
    tk = trcKey( idx );
}


const TrcKey& RandomSeisDataPack::trcKey( glob_idx_type idx ) const
{
    return path_.validIdx(idx) ? path_[idx] : TrcKey::udf();
}


TrcKey& RandomSeisDataPack::trcKey( glob_idx_type idx )
{
    if ( path_.validIdx(idx) )
	return path_.get( idx );
    static TrcKey ret;
    ret = TrcKey::udf();
    return ret;
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


RandomSeisDataPack::glob_idx_type
RandomSeisDataPack::gtGlobalIdx( const TrcKey& tk ) const
{
    return path_.indexOf( tk );
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

    PtrMan<Survey::HorSubSel> allposss = regsdp.horSubSel().duplicate();
    allposss->clearSubSel();

    // Remove outer undefined traces at both sides
    int pathidx = path.size()-1;
    while ( pathidx>0 && !allposss->includes(path[pathidx]) )
	path.removeSingle( pathidx-- );

    while ( path.size()>1 && !allposss->includes(path[0]) )
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
    if ( !auxtkzs.adjustTo(TrcKeyZSampling(regsdp.subSel()),true)
	    && path.size()>1 )
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


Pos::GeomID SeisFlatDataPack::geomID() const
{
    if ( !is2D() )
	return GeomID::get3D();
    const auto& pth = path();
    return pth.isEmpty() ? GeomID() : pth.first().geomID();
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
	case SeisTrcInfo::TrcNr:	return path().get(i0).trcNr();
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
	TrcKey tk; getTrcKey( trcidx, tk );
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



template <class T>
static StepInterval<double> gtDIntv( const StepInterval<T>& rg )
{
    return StepInterval<double>( rg.start, rg.stop, rg.step );
}


void SeisFlatDataPack::setPosData()
{
    const auto& tkpath = path();
    const int nrtrcs = tkpath.size();
    if ( nrtrcs < 1 )
	return;

    float* pos = new float[nrtrcs];
    pos[0] = 0;

    TrcKey prevtk = tkpath[0];
    for ( int idx=1; idx<nrtrcs; idx++ )
    {
	const TrcKey& trckey = tkpath[idx];
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
    posData().setRange( false, gtDIntv(zRange()) );
}



template<class DP>
static TrcKey trcKy( DP* dp, int idx )
{
    TrcKey tk; dp->getTrcKey( idx, tk );
    return tk;
}


#define mIsStraight ( (trcKy(this,0).distTo(trcKy(this,nrPositions()-1)) \
	    / posdata_.position(true,nrPositions()-1))>0.99 )


RegularSeisFlatDataPack::RegularSeisFlatDataPack(
		const RegularSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , usemulticomps_(comp_==-1)
    , hassingletrace_(nrPositions()==1)
    , path_(*new TrcKeyPath)
{
    if ( usemulticomps_ )
	setSourceDataFromMultiCubes();
    else
	setSourceData();
}


RegularSeisFlatDataPack::RegularSeisFlatDataPack(
			    const RegularSeisFlatDataPack& oth )
    : SeisFlatDataPack( oth )
    , path_(*new TrcKeyPath)
{
    copyClassData( oth );
}


RegularSeisFlatDataPack::~RegularSeisFlatDataPack()
{
    sendDelNotif();
    delete &path_;
}


mImplMonitorableAssignment( RegularSeisFlatDataPack, SeisFlatDataPack )


void RegularSeisFlatDataPack::copyClassData(
				const RegularSeisFlatDataPack& oth )
{
    path_ = oth.path_;
    usemulticomps_ = oth.usemulticomps_;
    hassingletrace_ = oth.hassingletrace_;
}


Monitorable::ChangeType RegularSeisFlatDataPack::compareClassData(
				    const RegularSeisFlatDataPack& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	path_ == oth.path_
    &&	usemulticomps_ == oth.usemulticomps_
    &&	hassingletrace_ == oth.hassingletrace_ );
}


Coord3 RegularSeisFlatDataPack::getCoord( int i0, int i1 ) const
{
    const bool isvertical = dir() != OD::ZSlice;
    const int trcidx = isvertical ? (hassingletrace_ ? 0 : i0)
				  : i0*horSubSel().trcNrSize()+i1;
    TrcKey tk; getTrcKey( trcidx, tk );
    const Coord c = tk.getCoord();
    return Coord3( c.x_, c.y_, subSel().zRange().atIndex(isvertical ? i1 : 0) );
}


OD::SliceType RegularSeisFlatDataPack::dir() const
{
    const auto* css = subSel().asCubeSubSel();
    if ( !css )
	return OD::InlineSlice; // 2D line ...

    return css->defaultDir();
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


const char* RegularSeisFlatDataPack::dimName( bool dim0 ) const
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


float RegularSeisFlatDataPack::getPosDistance( bool dim0, float posfidx ) const
{
    const int posidx = mCast(int,floor(posfidx));
    const float dfposidx = posfidx - posidx;
    if ( dim0 )
    {
	if ( !is2D() )
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


void RegularSeisFlatDataPack::setSourceDataFromMultiCubes()
{
    const int nrcomps = source_->nrComponents();
    const int nrz = subSel().nrZ();
    posdata_.setRange( true, StepInterval<double>(0,nrcomps-1,1) );
    posdata_.setRange( false, gtDIntv(subSel().zRange()) );

    arr2d_ = new Array2DImpl<float>( nrcomps, nrz );
    for ( int idx=0; idx<nrcomps; idx++ )
	for ( int idy=0; idy<nrz; idy++ )
	    arr2d_->set( idx, idy, source_->data(idx).get(0,0,idy) );
}


void RegularSeisFlatDataPack::setSourceData()
{
    const bool isz = dir()==OD::ZSlice;
    if ( !isz )
    {
	path_.setCapacity( source_->nrPositions(), false );
	for ( int idx=0; idx<source_->nrPositions(); idx++ )
	    path_ += trcKy( source_.ptr(), idx );
    }

    if ( is2D() )
	setPosData();
    else
    {
	const bool isinl = dir()==OD::InlineSlice;
	const auto& css = *subSel().asCubeSubSel();
	posdata_.setRange(
		true, isinl ? gtDIntv(css.crlRange())
			    : gtDIntv(css.inlRange()) );
	posdata_.setRange( false, isz ? gtDIntv(css.crlRange())
				      : gtDIntv(css.zRange()) );
    }

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



RandomSeisFlatDataPack::RandomSeisFlatDataPack(
		const RandomSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
{
    setSourceData();
}


RandomSeisFlatDataPack::RandomSeisFlatDataPack(
				const RandomSeisFlatDataPack& oth )
    : SeisFlatDataPack( oth )
{
    copyClassData( oth );
}


RandomSeisFlatDataPack::~RandomSeisFlatDataPack()
{
    sendDelNotif();
}


mImplMonitorableAssignmentWithNoMembers( RandomSeisFlatDataPack,
					 SeisFlatDataPack )


Coord3 RandomSeisFlatDataPack::getCoord( int i0, int i1 ) const
{
    const Coord coord = path().validIdx(i0) ? path().get(i0).getCoord()
					    : Coord::udf();
    return Coord3( coord, zRange().atIndex(i1) );
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


void RandomSeisFlatDataPack::setPosData()
{
    const TrcKeyPath& tkpath = path();
    const int nrtrcs = tkpath.size();
    if ( nrtrcs < 1 )
	return;

    float* pos = new float[nrtrcs];
    pos[0] = 0;
    int firstvalidposidx = -1, lastvalidposidx = -1;
    if ( !tkpath[0].isUdf() )
	firstvalidposidx = 0;

    TrcKey prevtk = tkpath[0];
    for ( int idx=1; idx<nrtrcs; idx++ )
    {
	const TrcKey& trckey = tkpath[idx];
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
    posData().setRange( false, gtDIntv(zRange()) );
}


float RandomSeisFlatDataPack::getPosDistance( bool dim0, float posfidx ) const
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
