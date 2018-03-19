/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksbackend.h"
#include "seismemblocks.h"
#include "od_iostream.h"
#include "uistrings.h"

typedef Seis::Blocks::SzType SzType;
static SzType columnHeaderSize( SzType ver ) { return 32; }


Seis::Blocks::StreamWriteBackEnd::StreamWriteBackEnd( Writer& wrr )
    : WriteBackEnd(wrr)
    , strm_(new od_ostream(wrr.dataFileName()))
{
}


Seis::Blocks::StreamWriteBackEnd::~StreamWriteBackEnd()
{
    delete strm_;
}


#define mRetIfStrmFail() \
    if ( !strm_->isOK() ) \
    { \
	uiString msg; strm_->addErrMsgTo( msg ); \
	if ( msg.isEmpty() ) \
	    msg = uiStrings::phrErrDuringWrite(); \
	uirv.add( msg ); \
	return; \
    }


void Seis::Blocks::StreamWriteBackEnd::setColumnInfo(
	    const MemBlockColumn& column, const HLocIdx& start,
	    const HDimensions& dims, uiRetVal& uirv )
{
    mRetIfStrmFail();

    const SzType hdrsz = columnHeaderSize( wrr_.version_ );
    const od_stream_Pos orgstrmpos = strm_->position();

    strm_->addBin( hdrsz );
    strm_->addBin( dims.first ).addBin( dims.second ).addBin(wrr_.dims_.third);
    strm_->addBin( start.first ).addBin( start.second );
    strm_->addBin( column.globIdx().first ).addBin( column.globIdx().second );
    mRetIfStrmFail();

    const int bytes_left_in_hdr = hdrsz - (int)(strm_->position()-orgstrmpos);
    if ( bytes_left_in_hdr > 0 )
    {
	char* buf = new char [bytes_left_in_hdr];
	OD::memZero( buf, bytes_left_in_hdr );
	strm_->addBin( buf, bytes_left_in_hdr );
	delete [] buf;
	mRetIfStrmFail();
    }
}


void Seis::Blocks::StreamWriteBackEnd::putBlock( int icomp, MemBlock& block,
		    HLocIdx wrstart, HDimensions wrhdims, uiRetVal& uirv )
{
    mRetIfStrmFail();

    const DataBuffer& dbuf = block.dbuf_;
    const Dimensions& blockdims = block.dims();
    const Dimensions wrdims( wrhdims.inl(), wrhdims.crl(), blockdims.z() );

    if ( wrdims == blockdims )
    {
	strm_->addBin( dbuf.data(), dbuf.totalBytes() );
	mRetIfStrmFail();
    }
    else
    {
	const DataBuffer::buf_type* bufdata = dbuf.data();
	const int bytespersample = dbuf.bytesPerElement();
	const int bytesperentirecrl = bytespersample * blockdims.z();
	const int bytesperentireinl = bytesperentirecrl * blockdims.crl();

	const int bytes2write = wrdims.z() * bytespersample;
	const IdxType wrstopinl = wrstart.inl() + wrdims.inl();
	const IdxType wrstopcrl = wrstart.crl() + wrdims.crl();

	const DataBuffer::buf_type* dataptr;
	for ( IdxType iinl=wrstart.inl(); iinl<wrstopinl; iinl++ )
	{
	    dataptr = bufdata + iinl * bytesperentireinl
			      + wrstart.crl() * bytesperentirecrl;
	    for ( IdxType icrl=wrstart.crl(); icrl<wrstopcrl; icrl++ )
	    {
		strm_->addBin( dataptr, bytes2write );
		mRetIfStrmFail();
		dataptr += bytesperentirecrl;
	    }
	}
    }
}


Seis::Blocks::HDF5WriteBackEnd::HDF5WriteBackEnd( Writer& wrr )
    : WriteBackEnd(wrr)
{
}


Seis::Blocks::HDF5WriteBackEnd::~HDF5WriteBackEnd()
{
    //TODO add file-level info before close
}


void Seis::Blocks::HDF5WriteBackEnd::setColumnInfo(
	    const MemBlockColumn& column, const HLocIdx& start,
	    const HDimensions& dims, uiRetVal& uirv )
{
    uirv.add( mTODONotImplPhrase() );
}


void Seis::Blocks::HDF5WriteBackEnd::putBlock( int icomp, MemBlock& block,
		    HLocIdx wrstart, HDimensions wrhdims, uiRetVal& uirv )
{
    uirv.add( mTODONotImplPhrase() );
}
