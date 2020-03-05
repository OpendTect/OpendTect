/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2018
________________________________________________________________________

-*/

#include "seisblocksbackend.h"
#include "seismemblocks.h"
#include "cubedata.h"
#include "hdf5arraynd.h"
#include "keystrs.h"
#include "od_iostream.h"
#include "seistrc.h"
#include "uistrings.h"
#include "zsubsel.h"


static const char* sKeyStartLoc = "Loc00";


Seis::Blocks::HDF5WriteBackEnd::HDF5WriteBackEnd( Writer& wrr, uiRetVal& uirv )
    : WriteBackEnd(wrr)
    , hdfwrr_(HDF5::mkWriter())
    , slabspec_(3)
    , databuf_(0)
{
    const BufferString fnm( wrr_.dataFileName() );
    if ( !hdfwrr_ )
	uirv.set( HDF5::Access::sHDF5NotAvailable(fnm) );
    else
	uirv = hdfwrr_->open( fnm );

    const Dimensions& dims = wrr_.dimensions();
    od_int64 bufsz = nrBytes( wrr_.dataRep() );
    bufsz *= dims.inl(); bufsz *= dims.crl(); bufsz *= dims.z();
    mTryAlloc( databuf_, char [ bufsz ] )
}


Seis::Blocks::HDF5WriteBackEnd::~HDF5WriteBackEnd()
{
    if ( hdfwrr_ )
    {
	pErrMsg( "need an explicit close()" );
	uiRetVal uirv;
	close( uirv );
    }
    delete [] databuf_;
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
    if ( !databuf_ )
	{ uirv.set( uiStrings::phrCannotAllocateMemory() ); return; }

    uirv = hdfwrr_->set( wrr_.gensectioniop_ );
    if ( !uirv.isOK() )
	return;

    const int nrsegs = wrr_.cubedata_.totalNrSegments();
    if ( nrsegs > 0 )
    {
	const HDF5::DataSetKey dsky( nullptr, sKey::SeisCubePositions() );;
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
    columndims_.z() = (size_type)wrr_.traceSize();
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

	uirv = hdfwrr_->set( blockiop, &dsky );
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

    const DataBuffer::buf_type* bufdata = block.dbuf_.data();
    const int bytespersample = block.dbuf_.bytesPerElement();
    const int bytesperentirecrl = bytespersample * block.dims().z();
    const int bytesperentireinl = bytesperentirecrl * block.dims().crl();

    const Dimensions wrdims( wrhdims.inl(), wrhdims.crl(), block.dims().z() );
    const int bytes2write = wrdims.z() * bytespersample;
    const idx_type wrstopinl = wrstart.inl() + wrdims.inl();
    const idx_type wrstopcrl = wrstart.crl() + wrdims.crl();

    const DataBuffer::buf_type* dataptr;
    char* bufptr = databuf_;
    for ( idx_type iinl=wrstart.inl(); iinl<wrstopinl; iinl++ )
    {
	dataptr = bufdata + iinl * bytesperentireinl
			  + wrstart.crl() * bytesperentirecrl;
	for ( idx_type icrl=wrstart.crl(); icrl<wrstopcrl; icrl++ )
	{
	    OD::sysMemCopy( bufptr, dataptr, bytes2write );
	    bufptr += bytes2write;
	    dataptr += bytesperentirecrl;
	}
    }

    const BufferString groupnm = wrr_.componentNames().get( icomp );
    const HDF5::DataSetKey dsky( groupnm, blockname_ );
    uirv = hdfwrr_->putSlab( dsky, slabspec_, databuf_ );
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

    void		fillTraceData(const BinID&,TraceData&,uiRetVal&) const;

    const Reader&	rdr_;
    const HGeom&	hgeom_;
    HDF5::Reader&	hdfrdr_;

    BufferString	blockname_;
    const HGlobIdx	globidx_;
    const HLocIdx	start_;
    const Dimensions	dims_;
    int			nrsamples2read_;
    Pos::ZSubSel	zss_;
    mutable HDF5::SlabSpec slabspec_;
    OD::DataRepType	datatype_;
    char*		valbuf_;
    ObjectSet<char>	zslicebufs_;
    mutable bool	zslicebufsread_;

    bool		isZSlice() const	{ return nrsamples2read_==1; }

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
    , hgeom_(rdrbe.rdr_.hGeom())
    , zss_(rdrbe.rdr_.zgeom_)
    , hdfrdr_(*rdrbe.hdfrdr_)
    , globidx_(gidx)
    , start_(0,0)
    , dims_(0,0,0)
    , nrsamples2read_(0)
    , valbuf_(0)
    , slabspec_(3)
    , zslicebufsread_(false)
{
    zss_.setOutputZRange( rdr_.zrgintrace_ );
    blockname_.set( globidx_.inl() ).add( "." ).add( globidx_.crl() );
    const HDF5::DataSetKey dsky( rdr_.componentNames().get(0), blockname_ );
    if ( !hdfrdr_.hasDataSet(dsky) )
	mRetOnInitialBlockProb( tr("Block is not present") )

    IOPar blockiop;
    uirv = hdfrdr_.get( blockiop, &dsky );
    if ( !uirv.isOK() )
	mRetOnInitialBlockProb( uirv )

    HLocIdx& start( const_cast<HLocIdx&>(start_) );
    blockiop.get( sKeyStartLoc, start.inl(), start.crl() );
    PtrMan<ArrayNDInfo> ainf = hdfrdr_.getDataSizes( dsky, uirv );
    if ( !ainf )
	mRetOnInitialBlockProb( tr("Cannot extract block sizes") )
    else if ( ainf->nrDims() != 3 )
	mRetOnInitialBlockProb( tr("Improper block found") )

    Dimensions& dms( const_cast<Dimensions&>(dims_) );
    dms.inl() = (size_type)ainf->getSize( 0 );
    dms.crl() = (size_type)ainf->getSize( 1 );
    dms.z() = (size_type)ainf->getSize( 2 );

    HDF5::SlabDimSpec& zdimspec = slabspec_[2];
    zdimspec.start_ = (HDF5::SlabDimSpec::idx_type)zss_.offset();
    zdimspec.step_ = (HDF5::SlabDimSpec::idx_type)zss_.step();
    zdimspec.count_ = (HDF5::SlabDimSpec::idx_type)zss_.size();
    if ( !isZSlice() )
	{ slabspec_[0].count_ = slabspec_[1].count_ = 1; }
    else
    {
	slabspec_[0].start_ = slabspec_[1].start_ = 0;
	slabspec_[0].count_ = (idx_type)dims_.inl();
	slabspec_[1].count_ = (idx_type)dims_.crl();
    }

    datatype_ = hdfrdr_.getDataType( dsky, uirv );
    const int bytesperval = nrBytes( datatype_ );
    if ( !rdr_.interp_ || rdr_.interp_->nrBytes() != bytesperval )
    {
	if ( rdr_.interp_ )
	    { pErrMsg("Dataype in info file != stored data type"); }
	Reader& rdr = const_cast<Reader&>( rdr_ );
	delete rdr.interp_;
	rdr.interp_ = DataInterpreter<float>::create( datatype_, true );
    }

    try {
	if ( !isZSlice() )
	    valbuf_ = new char [ zdimspec.count_ * bytesperval ];
	else
	{
	    od_int64 nrelems = dims_.inl(); nrelems *= dims_.crl();
	    for ( int icomp=0; icomp<rdr_.compsel_.size(); icomp++ )
		if ( rdr_.compsel_[icomp] )
		    zslicebufs_ += new char [ nrelems * bytesperval ];
	}
    } catch ( ... )
	{ uirv.set( uiStrings::phrCannotAllocateMemory() ); }
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
    delete [] valbuf_;
    for ( int idx=0; idx<zslicebufs_.size(); idx++ )
	delete [] zslicebufs_[idx];
}


void Seis::Blocks::HDF5Column::fillTraceData( const BinID& bid, TraceData& td,
					  uiRetVal& uirv ) const
{
    const HLocIdx locidx(
	Block::locIdx4Inl(hgeom_,bid.inl(),rdr_.dims_.inl()) - start_.inl(),
	Block::locIdx4Crl(hgeom_,bid.crl(),rdr_.dims_.crl()) - start_.crl() );
    if ( locidx.inl() < 0 || locidx.crl() < 0
      || locidx.inl() >= dims_.inl() || locidx.crl() >= dims_.crl() )
    {
	uirv.set( tr("Location from .info file (%1/%2) not in file")
		.arg(bid.inl()).arg(bid.crl()) );
	return;
    }

    td.setNrComponents( rdr_.nrcomponentsintrace_, rdr_.datarep_ );
    td.reSize( zss_.size() );
    if ( !isZSlice() )
    {
	td.zero();
	slabspec_[0].start_ = locidx.inl();
	slabspec_[1].start_ = locidx.crl();
    }

    int trccompnr = 0;
    for ( int icomp=0; icomp<rdr_.compsel_.size(); icomp++ )
    {
	if ( !rdr_.compsel_[icomp] )
	    continue;

	const bool needread = !isZSlice() || !zslicebufsread_;
	char* buf = isZSlice() ? (char*)zslicebufs_[trccompnr] : valbuf_;
	if ( needread )
	{
	    const HDF5::DataSetKey dsky( rdr_.componentNames().get(icomp),
					 blockname_ );
	    if ( !hdfrdr_.hasDataSet(dsky) )
		{ pErrMsg("dataset not found"); continue; }
	    uirv = hdfrdr_.getSlab( dsky, slabspec_, buf );
	    if ( !uirv.isOK() )
		return;
	}

	if ( isZSlice() )
	{
	    const int posnr = ((idx_type)dims_.crl()) * locidx.inl()
			    + locidx.crl();
	    td.setValue( 0, rdr_.interp_->get(buf,posnr), trccompnr );
	}
	else
	{
	    for ( int isamp=0; isamp<zss_.size(); isamp++ )
	    {
		const float val = rdr_.interp_->get( buf, isamp );
		td.setValue( isamp, rdr_.scaledVal(val), trccompnr );
	    }
	}

	trccompnr++;
    }

    zslicebufsread_ = true;
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
    if ( !hdfrdr_ )
	{ uirv.set( HDF5::Access::sHDF5NotAvailable() ); return 0; }

    HDF5Column* ret = new HDF5Column( *this, gidx, uirv );
    if ( uirv.isError() )
	{ delete ret; ret = 0; }

    return ret;
}


void Seis::Blocks::HDF5ReadBackEnd::fillTraceData( Column& column,
		    const BinID& bid, TraceData& td, uiRetVal& uirv ) const
{
    if ( !hdfrdr_ )
	{ uirv.set( HDF5::Access::sHDF5NotAvailable() ); return; }
    HDF5Column& hdfcolumn = static_cast<HDF5Column&>( column );
    hdfcolumn.fillTraceData( bid, td, uirv );
}
