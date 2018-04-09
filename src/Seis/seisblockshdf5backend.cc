/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2018
________________________________________________________________________

-*/

#include "seisblocksbackend.h"
#include "seismemblocks.h"
#include "hdf5arraynd.h"
#include "od_iostream.h"
#include "uistrings.h"
#include "posinfo.h"
#include "keystrs.h"
#include "seistrc.h"


static const char* sKeyStartLoc = "Loc00";


Seis::Blocks::HDF5WriteBackEnd::HDF5WriteBackEnd( Writer& wrr, uiRetVal& uirv )
    : WriteBackEnd(wrr)
    , hdfwrr_(HDF5::mkWriter())
    , slabspec_(3)
{
    const BufferString fnm( wrr_.dataFileName() );
    if ( !hdfwrr_ )
	uirv.set( HDF5::Access::sHDF5NotAvailable(fnm) );
    else
	uirv = hdfwrr_->open( fnm );
}


Seis::Blocks::HDF5WriteBackEnd::~HDF5WriteBackEnd()
{
    if ( hdfwrr_ )
    {
	pErrMsg( "need an explicit close()" );
	uiRetVal uirv;
	close( uirv );
    }
}


void Seis::Blocks::HDF5WriteBackEnd::close( uiRetVal& uirv )
{
    if ( !hdfwrr_ )
	return;

    writeGlobalInfo( uirv );

    delete hdfwrr_;
    hdfwrr_ = 0;
}


void Seis::Blocks::HDF5WriteBackEnd::writeGlobalInfo( uiRetVal& uirv )
{
    HDF5::DataSetKey dsky;
    uirv = hdfwrr_->putInfo( dsky, wrr_.gensectioniop_ );
    if ( !uirv.isOK() )
	return;

    const int nrsegs = wrr_.cubedata_.totalNrSegments();
    if ( nrsegs > 0 )
    {
	dsky.setDataSetName( sKey::SeisCubePositions() );
	Array2DImpl<int> arr( 4, nrsegs );
	int segnr = 0;
	for ( int iln=0; iln<wrr_.cubedata_.size(); iln++ )
	{
	    const PosInfo::LineData& ld = *wrr_.cubedata_[iln];
	    for ( int iseg=0; iseg<ld.segments_.size(); iseg++ )
	    {
		const PosInfo::LineData::Segment& seg = ld.segments_[iseg];
		arr.set( 0, segnr, ld.linenr_ );
		arr.set( 1, segnr, seg.start );
		arr.set( 2, segnr, seg.stop );
		arr.set( 3, segnr, seg.step );
		segnr++;
	    }
	}
	HDF5::ArrayNDTool<int> arrtool( arr );
	uirv = arrtool.put( *hdfwrr_, dsky );
    }
}


void Seis::Blocks::HDF5WriteBackEnd::setColumnInfo(
	    const MemBlockColumn& column, const HLocIdx& start,
	    const HDimensions& dims, uiRetVal& uirv )
{
    columndims_.set( dims );
    columndims_.z() = (SzType)wrr_.traceSize();
    blockname_.set( column.globIdx().inl() ).add( "." )
	      .add( column.globIdx().crl() );

    const OD::DataRepType datatype = wrr_.dataRep();
    IOPar blockiop;
    blockiop.set( sKeyStartLoc, start.inl(), start.crl() );
    const Array3DInfoImpl arrinf( columndims_.inl(), columndims_.crl(),
				  columndims_.z() );

    for ( int icomp=0; icomp<wrr_.componentNames().size(); icomp++ )
    {
	const BufferString groupnm = wrr_.componentNames().get( icomp );
	const HDF5::DataSetKey dsky( groupnm, blockname_ );
	uirv = hdfwrr_->createDataSet( dsky, arrinf, datatype );
	if ( !uirv.isOK() )
	    return;

	uirv = hdfwrr_->putInfo( dsky, blockiop );
	if ( !uirv.isOK() )
	    return;

	column.fileid_ = hdfwrr_->curGroupID();
    }
}


void Seis::Blocks::HDF5WriteBackEnd::putBlock( int icomp, MemBlock& block,
		    HLocIdx wrstart, HDimensions wrhdims, uiRetVal& uirv )
{
    slabspec_[0].count_ = columndims_.inl();
    slabspec_[1].count_ = columndims_.crl();
    slabspec_[2].start_ = block.globIdx().z() * wrr_.dimensions().z();
    slabspec_[2].count_ = block.dims().z();

    const BufferString groupnm = wrr_.componentNames().get( icomp );
    const HDF5::DataSetKey dsky( groupnm, blockname_ );
    if ( !hdfwrr_->setScope(dsky) )
	mPutInternalInUiRv( uirv, "DataSet not present", return )

    uirv = hdfwrr_->putSlab( slabspec_, block.dbuf_.data() );
}


namespace Seis
{

namespace Blocks
{

class HDF5Column : public Column
{ mODTextTranslationClass(Seis::Blocks::HDF5Column)
public:

			HDF5Column(const HDF5ReadBackEnd&,const HGlobIdx&,
				   uiRetVal&);
			~HDF5Column();

    void		fillTrace(const BinID&,SeisTrc&,uiRetVal&) const;

    const Reader&	rdr_;
    const HGeom&	hgeom_;
    HDF5::Reader&	hdfrdr_;

    BufferString	blockname_;
    const HGlobIdx	globidx_;
    const HLocIdx	start_;
    const Dimensions	dims_;
    int			nrsamples2read_;
    int			nrbytes2read_;
    mutable HDF5::SlabSpec slabspec_;
    OD::DataRepType	datatype_;
    char*		trcpartbuf_;

protected:

    uiString		probInBlockStr(const uiString&) const;

};

} // namespace Blocks

} // namespace Seis


#define mRetOnInitialBlockProb(msg) \
    { uirv.set( probInBlockStr(msg) ); return; }


Seis::Blocks::HDF5Column::HDF5Column( const HDF5ReadBackEnd& rdrbe,
				      const HGlobIdx& gidx, uiRetVal& uirv )
    : Column(gidx,Dimensions(0,0,0),rdrbe.rdr_.nrComponents())
    , rdr_(rdrbe.rdr_)
    , globidx_(gidx)
    , start_(0,0)
    , dims_(0,0,0)
    , hdfrdr_(*rdrbe.hdfrdr_)
    , nrsamples2read_(0)
    , hgeom_(rdrbe.rdr_.hGeom())
    , trcpartbuf_(0)
    , slabspec_(3)
{
    blockname_.set( globidx_.inl() ).add( "." ).add( globidx_.crl() );
    const HDF5::DataSetKey dsky( rdr_.componentNames().get(0), blockname_ );
    if ( !hdfrdr_.setScope(dsky) )
	mRetOnInitialBlockProb( tr("Block is not present") )

    IOPar blockiop;
    uirv = hdfrdr_.getInfo( blockiop );
    if ( !uirv.isOK() )
	mRetOnInitialBlockProb( uirv )

    HLocIdx& start( const_cast<HLocIdx&>(start_) );
    blockiop.get( sKeyStartLoc, start.inl(), start.crl() );
    PtrMan<ArrayNDInfo> ainf = hdfrdr_.getDataSizes();
    if ( !ainf )
	mRetOnInitialBlockProb( tr("Cannot extract block sizes") )
    else if ( ainf->nrDims() != 3 )
	mRetOnInitialBlockProb( tr("Improper block found") )

    Dimensions& dms( const_cast<Dimensions&>(dims_) );
    dms.inl() = (SzType)ainf->getSize( 0 );
    dms.crl() = (SzType)ainf->getSize( 1 );
    dms.z() = (SzType)ainf->getSize( 2 );

    slabspec_[0].count_ = slabspec_[1].count_ = 1;
    HDF5::SlabDimSpec& zdimspec = slabspec_[2];
    const Interval<IdxType> zidxrg(
	    (IdxType)rdr_.zgeom_.nearestIndex( rdr_.zrgintrace_.start ),
	    (IdxType)rdr_.zgeom_.nearestIndex( rdr_.zrgintrace_.stop ) );
    nrsamples2read_ = zidxrg.width() + 1;
    zdimspec.start_ = (HDF5::SlabDimSpec::IdxType)zidxrg.start;
    zdimspec.count_ = (HDF5::SlabDimSpec::IdxType)nrsamples2read_;

    datatype_ = hdfrdr_.getDataType();
    const int bytesperval = nrBytes( datatype_ );
    if ( !rdr_.interp_ || rdr_.interp_->nrBytes() != bytesperval )
    {
	if ( rdr_.interp_ )
	    { pErrMsg("Dataype in info file != stored data type"); }
	Reader& rdr = const_cast<Reader&>( rdr_ );
	delete rdr.interp_;
	rdr.interp_ = DataInterpreter<float>::create( datatype_, true );
    }

    nrbytes2read_ = nrsamples2read_ * bytesperval;
    trcpartbuf_ = new char [ nrbytes2read_ ];
}


uiString Seis::Blocks::HDF5Column::probInBlockStr( const uiString& msg ) const
{
    return tr("Problem in Block %1.%2 in '%3':\n%4" )
	  .arg( globidx_.inl() ).arg( globidx_.crl() )
	  .arg( hdfrdr_.fileName() )
	  .addMoreInfo( msg, true );
}


Seis::Blocks::HDF5Column::~HDF5Column()
{
    delete [] trcpartbuf_;
}


void Seis::Blocks::HDF5Column::fillTrace( const BinID& bid, SeisTrc& trc,
					  uiRetVal& uirv ) const
{
    const HLocIdx locidx(
	Block::locIdx4Inl(hgeom_,bid.inl(),rdr_.dims_.inl()) - start_.inl(),
	Block::locIdx4Crl(hgeom_,bid.crl(),rdr_.dims_.crl()) - start_.crl() );
    if ( locidx.inl() < 0 || locidx.crl() < 0
      || locidx.inl() >= dims_.inl() || locidx.crl() >= dims_.crl() )
    {
	// something's diff between bin block and info file; a getNext() found
	// this position in the CubeData, but the position is not available
	uirv.set( tr("Location from .info file (%1/%2) not in file")
		.arg(bid.inl()).arg(bid.crl()) );
	return;
    }
    slabspec_[0].start_ = locidx.inl();
    slabspec_[1].start_ = locidx.crl();

    trc.setNrComponents( rdr_.nrcomponentsintrace_, rdr_.datarep_ );
    trc.reSize( nrsamples2read_, false );
    trc.zero();

    int trccompnr = 0;
    for ( int icomp=0; icomp<rdr_.compsel_.size(); icomp++ )
    {
	if ( !rdr_.compsel_[icomp] )
	    continue;

	const HDF5::DataSetKey dsky( rdr_.componentNames().get(icomp),
				     blockname_ );
	if ( !hdfrdr_.setScope(dsky) )
	    { pErrMsg("scope not found"); continue; }
	uirv = hdfrdr_.getSlab( slabspec_, trcpartbuf_ );
	if ( !uirv.isOK() )
	    return;

	for ( int isamp=0; isamp<nrsamples2read_; isamp++ )
	{
	    const float val = rdr_.interp_->get( trcpartbuf_, isamp );
	    trc.set( isamp, rdr_.scaledVal(val), trccompnr );
	}

	trccompnr++;
    }
}



Seis::Blocks::HDF5ReadBackEnd::HDF5ReadBackEnd( Reader& rdr, const char* fnm,
						uiRetVal& uirv )
    : ReadBackEnd(rdr)
    , hdfrdr_(0)
{
    reset( fnm, uirv );
}


Seis::Blocks::HDF5ReadBackEnd::~HDF5ReadBackEnd()
{
    delete hdfrdr_;
}


void Seis::Blocks::HDF5ReadBackEnd::close()
{
    delete hdfrdr_;
    hdfrdr_ = 0;
}


void Seis::Blocks::HDF5ReadBackEnd::reset( const char* fnm, uiRetVal& uirv )
{
    delete hdfrdr_;
    hdfrdr_ = HDF5::mkReader();
    if ( !hdfrdr_ )
	uirv.set( HDF5::Access::sHDF5NotAvailable(fnm) );
    else
	uirv = hdfrdr_->open( fnm );
}


Seis::Blocks::Column* Seis::Blocks::HDF5ReadBackEnd::createColumn(
				const HGlobIdx& gidx, uiRetVal& uirv )
{
    HDF5Column* ret = new HDF5Column( *this, gidx, uirv );
    if ( uirv.isError() )
	{ delete ret; ret = 0; }
    return ret;
}


void Seis::Blocks::HDF5ReadBackEnd::fillTrace( Column& column, const BinID& bid,
				SeisTrc& trc, uiRetVal& uirv ) const
{
    HDF5Column& hdfcolumn = static_cast<HDF5Column&>( column );
    hdfcolumn.fillTrace( bid, trc, uirv );
}
