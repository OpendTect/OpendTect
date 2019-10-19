/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2018
________________________________________________________________________

-*/

#include "seisblocksbackend.h"
#include "seisblocksreader.h"
#include "seistrc.h"
#include "seismemblocks.h"
#include "seisseldata.h"
#include "od_iostream.h"
#include "uistrings.h"

mUseType( Seis::Blocks, size_type );
mUseType( Seis::Blocks, version_type );
static size_type columnHeaderSize( version_type ver ) { return 32; }


#define mRetIfStrmFail() \
    if ( !strm_->isOK() ) \
    { \
	uiString msg; strm_->addErrMsgTo( msg ); \
	if ( msg.isEmpty() ) \
	    msg = uiStrings::phrErrDuringWrite(); \
	uirv.add( msg ); \
	return; \
    }


Seis::Blocks::StreamWriteBackEnd::StreamWriteBackEnd( Writer& wrr,
						      uiRetVal& uirv )
    : WriteBackEnd(wrr)
    , strm_(new od_ostream(wrr.dataFileName()))
{
    mRetIfStrmFail()
}


Seis::Blocks::StreamWriteBackEnd::~StreamWriteBackEnd()
{
    if ( strm_ )
    {
	pErrMsg( "need an explicit close()" );
	uiRetVal uirv;
	close( uirv );
    }
}


void Seis::Blocks::StreamWriteBackEnd::close( uiRetVal& )
{
    delete strm_;
    strm_ = 0;
}


void Seis::Blocks::StreamWriteBackEnd::setColumnInfo(
	    const MemBlockColumn& column, const HLocIdx& start,
	    const HDimensions& dims, uiRetVal& uirv )
{
    mRetIfStrmFail();

    const size_type hdrsz = columnHeaderSize( wrr_.version_ );
    const od_stream_Pos orgstrmpos = strm_->position();
    column.fileid_ = orgstrmpos;

    strm_->addBin( hdrsz );
    strm_->addBin( dims.inl() ).addBin( dims.crl() ).addBin( wrr_.dims_.z() );
    strm_->addBin( start.inl() ).addBin( start.crl() );
    strm_->addBin( column.globIdx().inl() ).addBin( column.globIdx().crl() );
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
	const idx_type wrstopinl = wrstart.inl() + wrdims.inl();
	const idx_type wrstopcrl = wrstart.crl() + wrdims.crl();

	const DataBuffer::buf_type* dataptr;
	for ( idx_type iinl=wrstart.inl(); iinl<wrstopinl; iinl++ )
	{
	    dataptr = bufdata + iinl * bytesperentireinl
			      + wrstart.crl() * bytesperentirecrl;
	    for ( idx_type icrl=wrstart.crl(); icrl<wrstopcrl; icrl++ )
	    {
		strm_->addBin( dataptr, bytes2write );
		mRetIfStrmFail();
		dataptr += bytesperentirecrl;
	    }
	}
    }
}


namespace Seis
{

namespace Blocks
{

class FileColumn : public Column
{ mODTextTranslationClass(Seis::Blocks::FileColumn)
public:

    mUseType( Pos,	ZSubSel );

			FileColumn(const StreamReadBackEnd&,const HGlobIdx&,
				   uiRetVal&);
			~FileColumn();

    void		fillTraceData(const BinID&,TraceData&,uiRetVal&) const;

    const Reader&	rdr_;
    const HGeom&	hgeom_;
    od_istream&		strm_;

    od_stream_Pos	startoffsinfile_;
    size_type		headernrbytes_;
    HLocIdx		start_;
    const Dimensions	dims_;
    ZSubSel		zss_;

    struct Chunk
    {
	int		comp_;
	int		startsamp_;
	int		nrsamps_;
	od_stream_Pos	offs_;
	int		trcpartnrbytes_;
	int		blockznrbytes_;
    };
    ObjectSet<Chunk>	chunks_;

protected:

    char*		trcpartbuf_;

    void		createOffsetTable();

};

} // namespace Blocks

} // namespace Seis


Seis::Blocks::FileColumn::FileColumn( const StreamReadBackEnd& rdrbe,
				      const HGlobIdx& gidx, uiRetVal& uirv )
    : Column(gidx,Dimensions(0,0,0),rdrbe.rdr_.nrComponents())
    , rdr_(rdrbe.rdr_)
    , zss_(rdrbe.rdr_.zgeom_)
    , hgeom_(rdrbe.rdr_.hGeom())
    , start_(0,0)
    , strm_(*rdrbe.strm_)
    , trcpartbuf_(0)
    , startoffsinfile_(rdrbe.rdr_.fileidtbl_[globidx_])
{
    zss_.setOutputZRange( rdr_.zrgintrace_ );
    uirv.setEmpty();

    strm_.setReadPosition( startoffsinfile_ );
    strm_.getBin( headernrbytes_ );
    const size_type expectedhdrbts = columnHeaderSize( rdr_.version_ );
    if ( headernrbytes_ != expectedhdrbts )
    {
	uirv.set( tr("%1: unexpected size in file.\nFound %2, should be %3.")
		  .arg( strm_.fileName() )
		  .arg( headernrbytes_ ).arg( expectedhdrbts ) );
	return;
    }

    Dimensions& dms( const_cast<Dimensions&>(dims_) );
    strm_.getBin( dms.first() ).getBin( dms.second() ).getBin( dms.third() );
    strm_.getBin( start_.first() ).getBin( start_.second() );
    HGlobIdx gidx4dbg; // will be ignored, but helps debugging
    strm_.getBin( gidx4dbg.first() ).getBin( gidx4dbg.second() );

    if ( !strm_.isOK() )
    {
	uirv.set( tr("%1: unexpected end of file.").arg( strm_.fileName() ) );
	return;
    }

    createOffsetTable();
}


Seis::Blocks::FileColumn::~FileColumn()
{
    deepErase( chunks_ );
    delete [] trcpartbuf_;
}


void Seis::Blocks::FileColumn::createOffsetTable()
{
    const int nrsamplesinfile = rdr_.zgeom_.nrSteps() + 1;
    const int nrbytespersample = rdr_.interp_->nrBytes();
    const int nrbytespercompslice = ((int)dims_.inl()) * dims_.crl()
				    * nrbytespersample;

    const idx_type lastglobzidxinfile = Block::globIdx4Z( rdr_.zgeom_,
					rdr_.zgeom_.stop, dims_.z() );
    int nrsamplesintrace = 0;
    int blocknrbytes = dims_.z() * nrbytespercompslice;
    od_stream_Pos blockstartoffs = startoffsinfile_ + headernrbytes_;
    for ( idx_type gzidx=0; gzidx<=lastglobzidxinfile; gzidx++ )
    {
	size_type blockzdim = dims_.z();
	if ( gzidx == lastglobzidxinfile )
	{
	    size_type lastdim = size_type( nrsamplesinfile%dims_.z() );
	    if ( lastdim > 0 )
		blockzdim = lastdim;
	}

	idx_type startzidx = 0;
	idx_type stopzidx = idx_type( blockzdim ) - 1;
	blocknrbytes = nrbytespercompslice * blockzdim;
	int nrsampsthisblock = stopzidx - startzidx + 1;

	int compintrc = 0;
	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	{
	    if ( rdr_.compsel_[icomp] )
	    {
		Chunk* chunk = new Chunk;
		chunk->comp_ = compintrc;
		chunk->offs_ = blockstartoffs + startzidx * nrbytespersample;
		chunk->startsamp_ = nrsamplesintrace;
		chunk->nrsamps_ = nrsampsthisblock;
		chunk->trcpartnrbytes_ = nrsampsthisblock * nrbytespersample;
		chunk->blockznrbytes_ = blockzdim * nrbytespersample;
		chunks_ += chunk;
		compintrc++;
	    }
	    blockstartoffs += blocknrbytes;
	}

	nrsamplesintrace += nrsampsthisblock;
    }

    delete [] trcpartbuf_;
    trcpartbuf_ = new char [dims_.z() * nrbytespersample];
}


void Seis::Blocks::FileColumn::fillTraceData( const BinID& bid, TraceData& td,
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

    td.setNrComponents( rdr_.nrcomponentsintrace_, rdr_.datarep_ );
    td.reSize( zss_.size() );

    const int nrtrcs = ((int)locidx.inl()) * dims_.crl() + locidx.crl();
    int sampidx = 0;
    for ( int ichunk=0; ichunk<chunks_.size(); ichunk++ )
    {
	const Chunk& chunk = *chunks_[ichunk];
	const auto nextchunksampidx = sampidx + chunk.nrsamps_;
	if ( zss_.subSelIdx(nextchunksampidx-1) < 0 )
	    { sampidx = nextchunksampidx; continue; }
	else if ( zss_.subSelIdx(sampidx) >= zss_.size() )
	    break;

	strm_.setReadPosition( chunk.offs_ + nrtrcs * chunk.blockznrbytes_ );
	strm_.getBin( trcpartbuf_, chunk.trcpartnrbytes_ );
	for ( int isamp=0; isamp<chunk.nrsamps_; isamp++ )
	{
	    if ( zss_.isSelectedArrIdx(sampidx) )
	    {
		const float val = rdr_.interp_->get( trcpartbuf_, isamp );
		td.setValue( zss_.subSelIdx(sampidx),
			     rdr_.scaledVal(val), chunk.comp_ );
	    }
	    sampidx++;
	}
    }
}


Seis::Blocks::StreamReadBackEnd::StreamReadBackEnd( Reader& rdr,
			const char* fnm, uiRetVal& uirv )
    : ReadBackEnd(rdr)
    , strmmine_(true)
    , strm_(0)
{
    openStream( fnm, uirv );
}


Seis::Blocks::StreamReadBackEnd::StreamReadBackEnd( Reader& rdr,
						    od_istream& strm )
    : ReadBackEnd(rdr)
    , strmmine_(false)
    , strm_(&strm)
{
    strm_->setReadPosition( 0 );
}


Seis::Blocks::StreamReadBackEnd::~StreamReadBackEnd()
{
    close();
}


void Seis::Blocks::StreamReadBackEnd::openStream( const char* fnm,
						  uiRetVal& uirv )
{
    close();
    strm_ = new od_istream( fnm );
    if ( !strm_->isOK() )
    {
	uirv.set( uiStrings::phrCannotOpenForRead( fnm ) );
	strm_->addErrMsgTo( uirv );
	close();
    }
}


void Seis::Blocks::StreamReadBackEnd::close()
{
    if ( strmmine_ )
	delete strm_;
    strm_ = 0;
}


void Seis::Blocks::StreamReadBackEnd::reset( const char* fnm, uiRetVal& uirv )
{
    if ( strm_ && fnm != strm_->fileName() )
	close();

    if ( !strm_ )
	strm_ = new od_istream( fnm );
    else
	strm_->setReadPosition( 0 );

    if ( !strm_->isOK() )
    {
	uirv.set( uiStrings::phrCannotOpenForRead( fnm ) );
	strm_->addErrMsgTo( uirv );
	close();
    }
}


Seis::Blocks::Column* Seis::Blocks::StreamReadBackEnd::createColumn(
		const HGlobIdx& globidx, uiRetVal& uirv )
{
    FileColumn* ret = new FileColumn( *this, globidx, uirv );
    if ( uirv.isError() )
	{ delete ret; ret = 0; }
    return ret;
}


void Seis::Blocks::StreamReadBackEnd::fillTraceData( Column& column,
	const BinID& bid, TraceData& td, uiRetVal& uirv ) const
{
    FileColumn& filecolumn = static_cast<FileColumn&>( column );
    filecolumn.fillTraceData( bid, td, uirv );
}
