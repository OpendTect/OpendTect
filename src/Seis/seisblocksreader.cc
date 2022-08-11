/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

-*/

#include "seisblocksreader.h"
#include "seisselection.h"
#include "seistrc.h"
#include "uistrings.h"
#include "posidxpairdataset.h"
#include "scaler.h"
#include "datachar.h"
#include "file.h"
#include "filepath.h"
#include "keystrs.h"
#include "posinfo.h"
#include "survgeom3d.h"
#include "separstr.h"
#include "od_istream.h"
#include "ascstream.h"
#include "zdomain.h"
#include "genc.h"
#include <map>


namespace Seis
{

namespace Blocks
{


class OffsetTable : public std::map<HGlobIdx,od_stream_Pos> {};


class FileColumn : public Column
{ mODTextTranslationClass(Seis::Blocks::FileColumn)
public:

			FileColumn(const Reader&,const HGlobIdx&,uiRetVal&);
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


Seis::Blocks::FileColumn::FileColumn( const Reader& rdr, const HGlobIdx& gidx,
				      uiRetVal& uirv )
    : Column(gidx,Dimensions(0,0,0),rdr.nrComponents())
    , rdr_(rdr)
    , start_(0,0)
    , strm_(*rdr.strm_)
    , nrsamplesintrace_(0)
    , trcpartbuf_(0)
    , hgeom_(rdr_.hGeom())
    , startoffsinfile_(rdr.offstbl_[globidx_])
{
    uirv.setOK();

    strm_.setReadPosition( startoffsinfile_ );
    strm_.getBin( headernrbytes_ );
    SzType expectedhdrbts = rdr_.columnHeaderSize( rdr_.version_ );
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

    trc.setNrComponents( rdr_.nrcomponentsintrace_, rdr_.fprep_ );
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


#define mSeisBlocksReaderInitList() \
      offstbl_(*new OffsetTable) \
    , strm_(0) \
    , scaler_(0) \
    , interp_(0) \
    , cubedata_(*new PosInfo::CubeData) \
    , curcdpos_(*new PosInfo::CubeDataPos) \
    , seldata_(0) \
    , nrcomponentsintrace_(0) \
    , depthinfeet_(false) \
    , lastopwasgetinfo_(false)



Seis::Blocks::Reader::Reader( const char* inp )
    : mSeisBlocksReaderInitList()
    , strmmine_(true)
{
    initFromFileName( inp );
}


Seis::Blocks::Reader::Reader( od_istream& strm )
    : mSeisBlocksReaderInitList()
{
    basepath_.set( strm.fileName() );
    const BufferString ext = basepath_.extension();
    if ( ext == sInfoFileExtension() )
    {
	strmmine_ = true;
	readInfoFile( strm );
    }
    else
    {
	strmmine_ = false;
	strm_ = &strm;
	FilePath fp( basepath_ );
	fp.setExtension( sInfoFileExtension() );
	initFromFileName( fp.fullPath() );
    }
    basepath_.setExtension( 0 );
}



void Seis::Blocks::Reader::initFromFileName( const char* inp )
{
    if ( !File::exists(inp) )
    {
	if ( !inp || !*inp )
	    state_.set( tr("No input specified") );
	else
	    state_.set( uiStrings::phrDoesntExist(toUiString(inp)) );
	return;
    }

    basepath_.set( inp );
    basepath_.setExtension( 0 );

    const BufferString fnm( infoFileName() );
    od_istream strm( infoFileName() );
    if ( !strm.isOK() )
    {
	state_.set( uiStrings::phrCannotOpen(toUiString(strm.fileName())) );
	uiString statestr( state_ );
	strm.addErrMsgTo( statestr );
	state_ = statestr;
	return;
    }

    readInfoFile( strm );
}


Seis::Blocks::Reader::~Reader()
{
    closeStream();
    delete seldata_;
    delete &cubedata_;
    delete &curcdpos_;
    delete &offstbl_;
}


void Seis::Blocks::Reader::close()
{
    closeStream();
    offstbl_.clear();
    needreset_ = true;
}


void Seis::Blocks::Reader::closeStream() const
{
    if ( strmmine_ )
	delete strm_;
    strm_ = 0;
}


void Seis::Blocks::Reader::readInfoFile( od_istream& strm )
{
    ascistream astrm( strm );
    if ( !astrm.isOfFileType(sKeyFileType()) )
    {
	state_.set( tr("%1\nhas wrong file type").arg(strm.fileName()) );
	return;
    }

    bool havegensection = false, havepossection = false, haveoffsection = false;
    while ( !havepossection )
    {
	BufferString sectnm;
	if ( !strm.getLine(sectnm) )
	    break;

	if ( !sectnm.startsWith(sKeySectionPre()) )
	{
	    state_.set( tr("%1\n'%2' keyword not found")
			.arg(strm.fileName()).arg(sKeySectionPre()) );
	    return;
	}

	bool failed = false;
	if ( sectnm == sKeyPosSection() )
	{
	    failed = !cubedata_.read( strm, true );
	    havepossection = true;
	}
	else if ( sectnm == sKeyGenSection() )
	{
	    IOPar iop;
	    iop.getFrom( astrm );
	    failed = !getGeneralSectionData( iop );
	    havegensection = true;
	}
	else if ( sectnm == sKeyOffSection() )
	{
	    IOPar iop;
	    iop.getFrom( astrm );
	    failed = !getOffsetSectionData( iop );
	    haveoffsection = true;
	}
	else
	{
	    IOPar* iop = new IOPar;
	    iop->getFrom( astrm );
	    iop->setName( sectnm.str() + StringView(sKeySectionPre()).size() );
	    auxiops_ += iop;
	}
	if ( failed )
	{
	    state_.set( tr("%1\n'%2' section is invalid").arg(strm.fileName())
			.arg(sectnm) );
	    return;
	}
    }

    if ( !havegensection || !havepossection || !haveoffsection )
	state_.set( tr("%1\nlacks %2 section").arg(strm.fileName())
	       .arg( !havegensection ? tr("General")
		  : (!havepossection ? tr("Positioning")
				     : tr("File Offset")) ) );
}


bool Seis::Blocks::Reader::getGeneralSectionData( const IOPar& iop )
{
    int ver = version_;
    iop.get( sKeyFmtVersion(), ver );
    version_ = (SzType)ver;
    iop.get( sKeyCubeName(), cubename_ );
    if ( cubename_.isEmpty() )
	cubename_ = basepath_.fileName();
    iop.get( sKeySurveyName(), survname_ );

    hgeom_.getMapInfo( iop );
    hgeom_.setName( cubename_ );
    hgeom_.setZDomain( ZDomain::Def::get(iop) );
    iop.get( sKey::ZRange(), zgeom_ );
    iop.getYN( sKeyDepthInFeet(), depthinfeet_ );

    const char* res = iop.find( sKey::DataStorage() );
    if ( res && isdigit(*res) )
	fprep_ = DataCharacteristics::UserType( *res - '0' );
    interp_ = DataInterp::create( DataCharacteristics(fprep_), true );
    Scaler* scl = Scaler::get( iop.find(sKey::Scale()) );
    mDynamicCast( LinScaler*, scaler_, scl );

    int i1 = dims_.inl(), i2 = dims_.crl(), i3 = dims_.z();
    if ( !iop.get(sKeyDimensions(),i1,i2,i3) )
    {
	state_.set( tr("%1\nlacks block dimension info").arg(infoFileName()) );
	return false;
    }
    dims_.inl() = SzType(i1); dims_.crl() = SzType(i2); dims_.z() = SzType(i3);

    FileMultiString fms( iop.find(sKeyComponents()) );
    const int nrcomps = fms.size();
    if ( nrcomps < 1 )
    {
	compnms_.add( "Component 1" );
	compsel_ += true;
    }
    else
    {
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	{
	    compnms_.add( fms[icomp] );
	    compsel_ += true;
	}
    }

    datatype_ = dataTypeOf( iop.find( sKeyDataType() ) );
    return true;
}


bool Seis::Blocks::Reader::getOffsetSectionData( const IOPar& iop )
{
    for ( int idx=0; idx<iop.size(); idx++ )
    {
	BufferString kw( iop.getKey(idx) );
	char* ptr = kw.find( '.' );
	if ( !ptr )
	    continue;

	*ptr++ = '\0';
	const HGlobIdx globidx( (IdxType)toInt(kw), (IdxType)toInt(ptr) );
	const od_stream_Pos pos = toInt64( iop.getValue(idx) );
	offstbl_[globidx] = pos;
    }

    return !offstbl_.empty();
}


void Seis::Blocks::Reader::setSelData( const SelData* sd )
{
    if ( seldata_ != sd )
    {
	delete seldata_;
	if ( sd )
	    seldata_ = sd->clone();
	else
	    seldata_ = 0;
	needreset_ = true;
    }
}


bool Seis::Blocks::Reader::isSelected( const CubeDataPos& pos ) const
{
    return pos.isValid()
	&& (!seldata_ || seldata_->isOK(cubedata_.binID(pos)));
}


bool Seis::Blocks::Reader::advancePos( CubeDataPos& pos ) const
{
    while ( true )
    {
	if ( !cubedata_.toNext(pos) )
	{
	    pos.toPreStart();
	    return false;
	}
	if ( isSelected(pos) )
	    return true;
    }
}


uiRetVal Seis::Blocks::Reader::skip( int nrpos ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );
    for ( int idx=0; idx<nrpos; idx++ )
    {
	if ( !advancePos(curcdpos_) )
	{
	    uirv.set( tr("Failed skipping %1 positions").arg(idx+1) );
	    return uirv;
	}
    }
    return uirv;
}


bool Seis::Blocks::Reader::reset( uiRetVal& uirv ) const
{
    needreset_ = false;
    lastopwasgetinfo_ = false;

    curcdpos_.toPreStart();
    if ( !advancePos(curcdpos_) )
    {
	uirv.set( tr("No selected positions found") );
	return false;
    }

    const BufferString fnm( dataFileName() );
    if ( strm_ && fnm != strm_->fileName() )
	closeStream();
    if ( !strm_ )
	strm_ = new od_istream( fnm );
    else
	strm_->setReadPosition( 0 );
    if ( !strm_->isOK() )
    {
	closeStream();
	uirv.set( uiStrings::phrCannotOpen( toUiString(fnm) ) );
	return false;
    }

    int& nrcomps = const_cast<int&>( nrcomponentsintrace_ );
    nrcomps = 0;
    for ( int idx=0; idx<compsel_.size(); idx++ )
	if ( compsel_[idx] )
	    nrcomps++;

    Interval<float>& zrg = const_cast<Interval<float>&>( zrgintrace_ );
    zrg = zgeom_;
    if ( seldata_ )
    {
	zrg.limitTo( seldata_->zRange() );
	zrg.start = zgeom_.snap( zrg.start );
	zrg.stop = zgeom_.snap( zrg.stop );
    }

    return true;
}


bool Seis::Blocks::Reader::goTo( const BinID& bid ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );
    return doGoTo( bid, uirv );
}


uiRetVal Seis::Blocks::Reader::getTrcInfo( SeisTrcInfo& ti ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return uirv;
    }

    if ( lastopwasgetinfo_ )
	advancePos( curcdpos_ );
    lastopwasgetinfo_ = true;

    if ( !curcdpos_.isValid() )
	{ uirv.set( uiStrings::sFinished() ); return uirv; }

    const BinID bid = cubedata_.binID( curcdpos_ );
    fillInfo( bid, ti );

    return uirv;
}


uiRetVal Seis::Blocks::Reader::get( const BinID& bid, SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( !doGoTo(bid,uirv) )
	return uirv;

    doGet( trc, uirv );
    return uirv;
}


uiRetVal Seis::Blocks::Reader::getNext( SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    doGet( trc, uirv );
    return uirv;
}


bool Seis::Blocks::Reader::doGoTo( const BinID& bid, uiRetVal& uirv ) const
{
    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return false;
    }
    lastopwasgetinfo_ = false;

    PosInfo::CubeDataPos newcdpos = cubedata_.cubeDataPos( bid );
    if ( !newcdpos.isValid() )
    {
	uirv.set( tr("Position not present: %1/%2")
		.arg( bid.inl() ).arg( bid.crl() ) );
	return false;
    }

    curcdpos_ = newcdpos;
    return true;
}


void Seis::Blocks::Reader::fillInfo( const BinID& bid, SeisTrcInfo& ti ) const
{
    ti.setGeomID( hgeom_.getID() ).setPos( bid ).calcCoord();
    ti.sampling.start = zrgintrace_.start;
    ti.sampling.step = zgeom_.step;
}


void Seis::Blocks::Reader::doGet( SeisTrc& trc, uiRetVal& uirv ) const
{
    lastopwasgetinfo_ = false;
    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return;
    }

    if ( !curcdpos_.isValid() )
	{ uirv.set( uiStrings::sFinished() ); return; }

    readTrace( trc, uirv );
    if ( !uirv.isError() )
	advancePos( curcdpos_ );
}


Seis::Blocks::FileColumn* Seis::Blocks::Reader::getColumn(
		const HGlobIdx& globidx, uiRetVal& uirv ) const
{
    FileColumn* column = static_cast<FileColumn*>( findColumn(globidx) );
    if ( !column )
    {
	column = new FileColumn( *this, globidx, uirv );
	if ( uirv.isError() )
	    return 0;
	addColumn( column );
    }

    return column;
}


void Seis::Blocks::Reader::readTrace( SeisTrc& trc, uiRetVal& uirv ) const
{
    const BinID bid = cubedata_.binID( curcdpos_ );
    const HGlobIdx globidx( Block::globIdx4Inl(hgeom_,bid.inl(),dims_.inl()),
			    Block::globIdx4Crl(hgeom_,bid.crl(),dims_.crl()) );

    FileColumn* column = getColumn( globidx, uirv );
    if ( column )
    {
	column->fillTrace( bid, trc, uirv );
	fillInfo( bid, trc.info() );
    }
}
