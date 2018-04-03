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


Seis::Blocks::HDF5ReadBackEnd::HDF5ReadBackEnd( Reader& rdr, const char* fnm,
						uiRetVal& uirv )
    : ReadBackEnd(rdr)
    , hdfrdr_(HDF5::mkReader())
{
    if ( !hdfrdr_ )
	uirv.set( HDF5::Access::sHDF5NotAvailable(fnm) );
    else
	uirv = hdfrdr_->open( fnm );
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
    if ( !HDF5::isAvailable() )
	{ uirv.add( HDF5::Access::sHDF5NotAvailable(fnm) ); return; }
    uirv.add( mTODONotImplPhrase() );
}


Seis::Blocks::Column* Seis::Blocks::HDF5ReadBackEnd::createColumn(
				const HGlobIdx& gidx, uiRetVal& uirv )
{
    uirv.add( mTODONotImplPhrase() );
    return 0;
}


void Seis::Blocks::HDF5ReadBackEnd::fillTrace( Column& column, const BinID& bid,
				SeisTrc& trc, uiRetVal& uirv ) const
{
    uirv.add( mTODONotImplPhrase() );
}
