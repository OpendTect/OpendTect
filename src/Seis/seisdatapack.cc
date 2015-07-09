/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey
 Date:		February 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: seisdatapack.cc 38551 2015-03-18 05:38:02Z mahant.mothey@dgbes.com $";

#include "seisdatapack.h"

#include "arrayndimpl.h"
#include "arrayndslice.h"
#include "binidvalset.h"
#include "flatposdata.h"
#include "seistrc.h"
#include "survinfo.h"

#include <limits.h>


// RegularSeisDataPack
RegularSeisDataPack::RegularSeisDataPack( const char* cat,
					  const BinDataDesc* bdd )
    : SeisDataPack(cat,bdd)
{ sampling_.init( false ); }

TrcKey RegularSeisDataPack::getTrcKey( int globaltrcidx ) const
{ return sampling_.hsamp_.trcKeyAt( globaltrcidx ); }

bool RegularSeisDataPack::is2D() const
{ return sampling_.hsamp_.survid_ == Survey::GM().get2DSurvID(); }

int RegularSeisDataPack::getGlobalIdx( const TrcKey& tk ) const
{
    const int ret = mCast(int,sampling_.hsamp_.globalIdx(tk));
    return ret < nrTrcs() ? ret : -1;
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


void RegularSeisDataPack::dumpInfo( IOPar& par ) const
{
    DataPack::dumpInfo( par );

    const TrcKeySampling& tks = sampling_.hsamp_;
    // TODO: Change for 2D
    par.set( sKey::InlRange(), tks.start_.lineNr(), tks.stop_.lineNr(),
			       tks.step_.lineNr() );
    par.set( sKey::CrlRange(), tks.start_.trcNr(), tks.stop_.trcNr(),
			       tks.step_.trcNr() );
    par.set( sKey::ZRange(), sampling_.zsamp_.start, sampling_.zsamp_.stop,
			     sampling_.zsamp_.step );
}


DataPack::ID RegularSeisDataPack::createDataPackForZSlice(
						const BinIDValueSet* bivset,
						const TrcKeyZSampling& tkzs,
						const ZDomain::Info& zinfo,
						const BufferStringSet& names )
{
    if ( !bivset || tkzs.nrZ()!=1 )
	return DataPack::cNoID();

    RegularSeisDataPack* regsdp = new RegularSeisDataPack(
					SeisDataPack::categoryStr(false,true) );
    regsdp->setSampling( tkzs );
    for ( int idx=1; idx<bivset->nrVals(); idx++ )
    {
	const char* name = names.validIdx(idx-1) ? names[idx-1]->buf()
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
    DPM(DataPackMgr::SeisID()).add( regsdp );
    return regsdp->id();
}


// RandomSeisDataPack
RandomSeisDataPack::RandomSeisDataPack( const char* cat,
					const BinDataDesc* bdd )
    : SeisDataPack(cat,bdd)
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


int RandomSeisDataPack::getGlobalIdx( const TrcKey& tk ) const
{ return path_.indexOf( tk ); }


#define mKeyInl		SeisTrcInfo::getFldString(SeisTrcInfo::BinIDInl)
#define mKeyCrl		SeisTrcInfo::getFldString(SeisTrcInfo::BinIDCrl)
#define mKeyCoordX	SeisTrcInfo::getFldString(SeisTrcInfo::CoordX)
#define mKeyCoordY	SeisTrcInfo::getFldString(SeisTrcInfo::CoordY)
#define mKeyTrcNr	SeisTrcInfo::getFldString(SeisTrcInfo::TrcNr)
#define mKeyRefNr	SeisTrcInfo::getFldString(SeisTrcInfo::RefNr)


bool RandomSeisDataPack::setDataFrom( const RegularSeisDataPack* rgldp,
				      const TrcKeyPath& path,
				      const Interval<float>& zrg )
{
    if ( !rgldp || path.isEmpty()  || 
	 rgldp->nrComponents() ==0 ||
	 rgldp->sampling().totalNr()==0 ) 
	return false;

    setPath( path );
    setZRange(StepInterval<float>(zrg.start,zrg.stop,rgldp->getZRange().step));

    const int idzoffset = rgldp->getZRange().nearestIndex( getZRange().start );

    for ( int idx=0; idx<rgldp->nrComponents(); idx++ )
    {
	addComponent( rgldp->getComponentName(idx) );
	for ( int idy=0; idy<path.size(); idy++ )
	{
	    const int inlidx = 
		rgldp->sampling().hsamp_.inlIdx( path[idy].lineNr() );
	    const int crlidx = 
		rgldp->sampling().hsamp_.crlIdx( path[idy].trcNr() );

	    for ( int newidz=0; newidz<=getZRange().nrSteps(); newidz++ )
	    {
		const int oldidz = newidz + idzoffset;

		const float val = 
		    rgldp->data(idx).info().validPos(inlidx,crlidx,oldidz ) ?
		    rgldp->data(idx).get(inlidx,crlidx,oldidz) : mUdf(float);
		data(idx).set( 0, idy, newidz, val );
	    }
	}
    }

    setZDomain( rgldp->zDomain() );
    setName( rgldp->name() );

    return true;
}


bool RandomSeisDataPack::setDataFrom( const SeisTrcBuf& sbuf, 
			const TrcKeyPath& path, const TypeSet<BinID>& pathbid, 
			const BufferStringSet& cmpnms,
			const char* zdmkey, const char* nm )
{
    if ( path.isEmpty() || sbuf.isEmpty() || 
	sbuf.get(0)->nrComponents() == 0  || 
	cmpnms.isEmpty() )
	return false;

    setPath( path );
    setZRange( sbuf.get(0)->zRange() );

    for ( int idx = 0; idx<sbuf.get(0)->nrComponents(); idx++ )
    {
	addComponent( cmpnms.get(idx) );
	for ( int idy = 0; idy<data(idx).info().getSize(1); idy++ )
	{
	    const int trcidx = pathbid.isEmpty() ? idy : 
		sbuf.find( (pathbid)[idy] );
	    const SeisTrc* trc = trcidx<0 ? 0 : sbuf.get( trcidx );
	    for ( int idz = 0; idz<data(idx).info().getSize(2);	idz++ )
		data(idx).set( 0,idy,idz, 
		!trc ? mUdf(float) : trc->get(idz,idx) );
	}
    }

    setZDomain( ZDomain::Info(ZDomain::Def::get(zdmkey)) );
    setName( nm );

    return true;
}


// SeisFlatDataPack
SeisFlatDataPack::SeisFlatDataPack( const SeisDataPack& source,int comp)
    : FlatDataPack(source.category())
    , source_(source)
    , comp_(comp)
    , zsamp_(source.getZRange())
{
    DPM(DataPackMgr::SeisID()).addAndObtain(const_cast<SeisDataPack*>(&source));
    setName( source_.getComponentName(comp_) );
}


SeisFlatDataPack::~SeisFlatDataPack()
{
    DPM(DataPackMgr::SeisID()).release( source_.id() );
}


bool SeisFlatDataPack::dimValuesInInt( const char* keystr ) const
{
    const FixedString key( keystr );
    return key==mKeyInl || key==mKeyCrl || key==mKeyTrcNr ||
	   key==sKey::Series();
}


void SeisFlatDataPack::getAltDim0Keys( BufferStringSet& keys ) const
{
    for ( int idx=0; idx<tiflds_.size(); idx++ )
	keys.add( SeisTrcInfo::getFldString(tiflds_[idx]) );
}


double SeisFlatDataPack::getAltDim0Value( int ikey, int i0 ) const
{
    switch ( tiflds_[ikey] )
    {
	case SeisTrcInfo::BinIDInl:	return SI().transform(
						getCoord(i0,0)).inl();
	case SeisTrcInfo::BinIDCrl:	return SI().transform(
						getCoord(i0,0)).crl();
	case SeisTrcInfo::CoordX:	return getCoord(i0,0).x;
	case SeisTrcInfo::CoordY:	return getCoord(i0,0).y;
	case SeisTrcInfo::TrcNr:	return getPath()[i0].trcNr();
	case SeisTrcInfo::RefNr:	return source_.getRefNr(i0);
	default:			return posdata_.position(true,i0);
    }
}


void SeisFlatDataPack::getAuxInfo( int i0, int i1, IOPar& iop ) const
{
    const Coord3 crd = getCoord( i0, i1 );
    iop.set( mKeyCoordX, crd.x );
    iop.set( mKeyCoordY, crd.y );
    iop.set( sKey::ZCoord(), crd.z * zDomain().userFactor() );

    if ( is2D() )
    {
	const int trcidx = nrTrcs()==1 ? 0 : i0;
	iop.set( mKeyTrcNr, getTrcKey(trcidx).trcNr() );
	iop.set( mKeyRefNr, source_.getRefNr(trcidx) );
    }
    else
    {
	const BinID bid = SI().transform( crd );
	iop.set( mKeyInl, bid.inl() );
	iop.set( mKeyCrl, bid.crl() );
    }
}


float SeisFlatDataPack::nrKBytes() const
{
    return source_.nrKBytes() / source_.nrComponents();
}


#define mStepIntvD( rg ) \
    StepInterval<double>( rg.start, rg.stop, rg.step )

void SeisFlatDataPack::setPosData()
{
    const TrcKeyPath& path = getPath();
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
	    pos[idx] = mCast(float,(pos[idx-1] + prevtk.distTo(trckey)));
	    prevtk = trckey;
	}
    }

    posData().setX1Pos( pos, nrtrcs, 0 );
    posData().setRange( false, mStepIntvD(zsamp_) );
}



#define mIsStraight ((getTrcKey(0).distTo(getTrcKey(nrTrcs()-1))/ \
	posdata_.position(true,nrTrcs()-1))>0.99)

RegularFlatDataPack::RegularFlatDataPack(
		const RegularSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , sampling_(source.sampling())
    , dir_(sampling_.defaultDir())
    , usemulticomps_(comp_==-1)
    , hassingletrace_(sampling_.nrTrcs()==1)
{
    if ( usemulticomps_ )
	setSourceDataFromMultiCubes();
    else
	setSourceData();
}


Coord3 RegularFlatDataPack::getCoord( int i0, int i1 ) const
{
    const bool isvertical = dir_ != TrcKeyZSampling::Z;
    const int trcidx = isvertical ? (hassingletrace_ ? 0 : i0)
				  : i0*sampling_.nrTrcs()+i1;
    const Coord c = Survey::GM().toCoord( getTrcKey(trcidx) );
    return Coord3( c.x, c.y, sampling_.zsamp_.atIndex(isvertical ? i1 : 0) );
}


void RegularFlatDataPack::setTrcInfoFlds()
{
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


const char* RegularFlatDataPack::dimName( bool dim0 ) const
{
    if ( dim0 && hassingletrace_ ) return sKey::Series();
    if ( is2D() ) return dim0 ? "Distance" : "Z";
    return dim0 ? (dir_==TrcKeyZSampling::Inl ? mKeyCrl : mKeyInl)
		: (dir_==TrcKeyZSampling::Z ? mKeyCrl : "Z");
}


void RegularFlatDataPack::setSourceDataFromMultiCubes()
{
    const int nrcomps = source_.nrComponents();
    const int nrz = sampling_.zsamp_.nrSteps() + 1;
    posdata_.setRange( true, StepInterval<double>(0,nrcomps-1,1) );
    posdata_.setRange( false, mStepIntvD(sampling_.zsamp_) );

    arr2d_ = new Array2DImpl<float>( nrcomps, nrz );
    for ( int idx=0; idx<nrcomps; idx++ )
	for ( int idy=0; idy<nrz; idy++ )
	    arr2d_->set( idx, idy, source_.data(idx).get(0,0,idy) );
}


void RegularFlatDataPack::setSourceData()
{
    const bool isz = dir_==TrcKeyZSampling::Z;
    if ( !isz )
    {
	for ( int idx=0; idx<source_.nrTrcs(); idx++ )
	    path_ += source_.getTrcKey( idx );
    }

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
    Array2DSlice<float>* slice2d = new Array2DSlice<float>(source_.data(comp_));
    slice2d->setDimMap( 0, dim0 );
    slice2d->setDimMap( 1, dim1 );
    slice2d->setPos( dir_, 0 );
    slice2d->init();
    arr2d_ = slice2d;
    setTrcInfoFlds();
}



RandomFlatDataPack::RandomFlatDataPack(
		const RandomSeisDataPack& source, int comp )
    : SeisFlatDataPack(source,comp)
    , path_(source.getPath())
{
    setSourceData();
}


Coord3 RandomFlatDataPack::getCoord( int i0, int i1 ) const
{
    const Coord coord = path_.validIdx(i0) ? Survey::GM().toCoord(path_[i0])
					   : Coord::udf();
    return Coord3( coord, zsamp_.atIndex(i1) );
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
    Array2DSlice<float>* slice2d = new Array2DSlice<float>(source_.data(comp_));
    slice2d->setDimMap( 0, 1 );
    slice2d->setDimMap( 1, 2 );
    slice2d->setPos( 0, 0 );
    slice2d->init();
    arr2d_ = slice2d;
    setTrcInfoFlds();
}

