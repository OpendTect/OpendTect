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
#include "seisselection.h"
#include "scaler.h"
#include "od_iostream.h"
#include "uistrings.h"

typedef Seis::Blocks::SzType SzType;
static SzType columnHeaderSize( SzType ver ) { return 32; }


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
    delete strm_;
}


void Seis::Blocks::StreamWriteBackEnd::setColumnInfo(
	    const MemBlockColumn& column, const HLocIdx& start,
	    const HDimensions& dims, uiRetVal& uirv )
{
    mRetIfStrmFail();

    const SzType hdrsz = columnHeaderSize( wrr_.version_ );
    const od_stream_Pos orgstrmpos = strm_->position();
    column.fileid_ = orgstrmpos;

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


namespace Seis
{

namespace Blocks
{

class FileColumn : public Column
{ mODTextTranslationClass(Seis::Blocks::FileColumn)
public:

			FileColumn(const StreamReadBackEnd&,const HGlobIdx&,
				   uiRetVal&);
			~FileColumn();

    void		fillTrace(const BinID&,SeisTrc&,uiRetVal&) const;

    const Reader&	rdr_;
    const HGeom&	hgeom_;
    od_istream&		strm_;

    od_stream_Pos	startoffsinfile_;
    SzType		headernrbytes_;
    HLocIdx		start_;
    const Dimensions	dims_;
    int			nrsamplesintrace_;

    struct Chunk
    {
	int		comp_;
	int		startsamp_;
	int		nrsamps_;
	od_stream_Pos	offs_;
	int		trcpartnrbytes_;
	int		blockznrbytes_;
    };
    ObjectSet<Chunk>    chunks_;

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
    , start_(0,0)
    , strm_(*rdrbe.strm_)
    , nrsamplesintrace_(0)
    , trcpartbuf_(0)
    , hgeom_(rdrbe.rdr_.hGeom())
    , startoffsinfile_(rdrbe.rdr_.fileidtbl_[globidx_])
{
    uirv.setEmpty();

    strm_.setReadPosition( startoffsinfile_ );
    strm_.getBin( headernrbytes_ );
    const SzType expectedhdrbts = columnHeaderSize( rdr_.version_ );
    if ( headernrbytes_ != expectedhdrbts )
    {
	uirv.set( tr("%1: unexpected size in file.\nFound %2, should be %3.")
	          .arg( strm_.fileName() )
		  .arg( headernrbytes_ ).arg( expectedhdrbts ) );
	return;
    }

    Dimensions& dms( const_cast<Dimensions&>(dims_) );
    strm_.getBin( dms.first ).getBin( dms.second ).getBin( dms.third );
    strm_.getBin( start_.first ).getBin( start_.second );
    HGlobIdx gidx4dbg; // will be ignored, but helps debugging
    strm_.getBin( gidx4dbg.first ).getBin( gidx4dbg.second );

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

    const IdxType lastglobzidxinfile = Block::globIdx4Z( rdr_.zgeom_,
					rdr_.zgeom_.stop, dims_.z() );
    Interval<IdxType> trcgzidxrg(
	    Block::globIdx4Z( rdr_.zgeom_, rdr_.zrgintrace_.start, dims_.z() ),
	    Block::globIdx4Z( rdr_.zgeom_, rdr_.zrgintrace_.stop, dims_.z() ) );
    nrsamplesintrace_ = 0;
    int blocknrbytes = dims_.z() * nrbytespercompslice;
    od_stream_Pos blockstartoffs = startoffsinfile_ + headernrbytes_
				 + trcgzidxrg.start * blocknrbytes * nrcomps_;
    for ( IdxType gzidx=trcgzidxrg.start; gzidx<=trcgzidxrg.stop; gzidx++ )
    {
	SzType blockzdim = dims_.z();
	if ( gzidx == lastglobzidxinfile )
	{
	    SzType lastdim = SzType( nrsamplesinfile%dims_.z() );
	    if ( lastdim > 0 )
		blockzdim = lastdim;
	}

	IdxType startzidx = 0;
	IdxType stopzidx = IdxType( blockzdim ) - 1;
	if ( gzidx == trcgzidxrg.start )
	    startzidx = Block::locIdx4Z( rdr_.zgeom_, rdr_.zrgintrace_.start,
					 dims_.z() );
	if ( gzidx == trcgzidxrg.stop )
	    stopzidx = Block::locIdx4Z( rdr_.zgeom_, rdr_.zrgintrace_.stop,
					 dims_.z() );

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
		chunk->startsamp_ = nrsamplesintrace_;
		chunk->nrsamps_ = nrsampsthisblock;
		chunk->trcpartnrbytes_ = nrsampsthisblock * nrbytespersample;
		chunk->blockznrbytes_ = blockzdim * nrbytespersample;
		chunks_ += chunk;
		compintrc++;
	    }
	    blockstartoffs += blocknrbytes;
	}

	nrsamplesintrace_ += nrsampsthisblock;
    }

    delete [] trcpartbuf_;
    trcpartbuf_ = new char [dims_.z() * nrbytespersample];
}


void Seis::Blocks::FileColumn::fillTrace( const BinID& bid, SeisTrc& trc,
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

    trc.setNrComponents( rdr_.nrcomponentsintrace_, rdr_.datarep_ );
    trc.reSize( nrsamplesintrace_, false );

    const int nrtrcs = ((int)locidx.inl()) * dims_.crl() + locidx.crl();
    for ( int idx=0; idx<chunks_.size(); idx++ )
    {
	const Chunk& chunk = *chunks_[idx];
	strm_.setReadPosition( chunk.offs_ + nrtrcs * chunk.blockznrbytes_ );
	strm_.getBin( trcpartbuf_, chunk.trcpartnrbytes_ );
	for ( int isamp=0; isamp<chunk.nrsamps_; isamp++ )
	{
	    float val = rdr_.interp_->get( trcpartbuf_, isamp );
	    if ( rdr_.scaler_ )
		val = (float)rdr_.scaler_->scale( val );
	    trc.set( chunk.startsamp_ + isamp, val, chunk.comp_ );
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
    closeStream();
}


void Seis::Blocks::StreamReadBackEnd::openStream( const char* fnm,
						  uiRetVal& uirv )
{
    closeStream();
    strm_ = new od_istream( fnm );
    if ( !strm_->isOK() )
    {
	uirv.set( uiStrings::phrCannotOpen( toUiString(fnm) ) );
	strm_->addErrMsgTo( uirv );
	closeStream();
    }
}


void Seis::Blocks::StreamReadBackEnd::closeStream()
{
    if ( strmmine_ )
	delete strm_;
    strm_ = 0;
}


void Seis::Blocks::StreamReadBackEnd::reset( const char* fnm, uiRetVal& uirv )
{
    if ( strm_ && fnm != strm_->fileName() )
	closeStream();

    if ( !strm_ )
	strm_ = new od_istream( fnm );
    else
	strm_->setReadPosition( 0 );

    if ( !strm_->isOK() )
    {
	uirv.set( uiStrings::phrCannotOpen( toUiString(fnm) ) );
	strm_->addErrMsgTo( uirv );
	closeStream();
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


void Seis::Blocks::StreamReadBackEnd::fillTrace( Column& column,
	const BinID& bid, SeisTrc& trc, uiRetVal& uirv ) const
{
    FileColumn& filecolumn = static_cast<FileColumn&>( column );
    filecolumn.fillTrace( bid, trc, uirv );
}
