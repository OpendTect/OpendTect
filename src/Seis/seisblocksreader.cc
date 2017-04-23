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


namespace Seis
{

namespace Blocks
{

class FileColumn : public Column
{ mODTextTranslationClass(Seis::Blocks::FileColumn)
public:

			FileColumn(const Reader&,const HGlobIdx&);
			~FileColumn();

    void		activate(uiRetVal&);
    void		retire();

    void		fillTrace(const BinID&,SeisTrc&,uiRetVal&) const;

    const Reader&	rdr_;
    od_istream*		strm_;

    IOClass::HdrSzVersionType headernrbytes_;
    HLocIdx		start_;
    const Dimensions	dims_;
    int			nrcomponents_;
    int			nrsamplesintrace_;
    int			nrcomponentsintrace_;
    DataInterp*		interp_;
    LinScaler*		scaler_;
    const HGeom&	hgeom_;
    float		zstart_;

    struct Chunk
    {
	int		comp_;
	int		startsamp_;
	int		nrsamps_;
	od_stream_Pos	offs_;
	od_stream_Pos	trcpartnrbytes_;
    };
    ObjectSet<Chunk>    chunks_;

protected:

    bool		wasretired_;
    char*		trcpartbuf_;

    void		createOffsetTable();
    void		closeStream();

};

} // namespace Blocks

} // namespace Seis


Seis::Blocks::FileColumn::FileColumn( const Reader& rdr, const HGlobIdx& gidx )
    : Column(gidx,Dimensions(0,0,0),rdr.nrComponents())
    , rdr_(rdr)
    , start_(0,0)
    , interp_(0)
    , strm_(0)
    , scaler_(0)
    , nrsamplesintrace_(0)
    , nrcomponentsintrace_(0)
    , trcpartbuf_(0)
    , wasretired_(false)
    , hgeom_(*rdr_.hgeom_)
{
    for ( int idx=0; idx<rdr_.compsel_.size(); idx++ )
	if ( rdr_.compsel_[idx] )
	    nrcomponentsintrace_++;
}


Seis::Blocks::FileColumn::~FileColumn()
{
    closeStream();
    delete scaler_;
    deepErase( chunks_ );
    delete [] trcpartbuf_;
}


void Seis::Blocks::FileColumn::activate( uiRetVal& uirv )
{
    uirv.setEmpty();
    if ( strm_ )
	return;

    const File::Path fp( rdr_.dataDirName(), rdr_.fileNameFor(globidx_) );
    const BufferString fnm( fp.fullPath() );
    strm_ = new od_istream( fnm );
    if ( !strm_->isOK() )
    {
	closeStream();
	uirv.set( uiStrings::phrCannotOpen( toUiString(fnm) ) );
	return;
    }

    if ( wasretired_ )
	return;

    IOClass::HdrSzVersionType version, dfmt;
    strm_->getBin( headernrbytes_ ).getBin( version ).getBin( dfmt );
    IOClass::HdrSzVersionType expectedhdrbts = rdr_.columnHeaderSize( version );
    if ( headernrbytes_ != expectedhdrbts )
    {
	closeStream();
	uirv.set( tr("%1: unexpected header size.\nFound %2, should be %3.")
	          .arg( fnm ).arg( headernrbytes_ ).arg( expectedhdrbts ) );
	return;
    }
    const DataCharacteristics dc( (OD::FPDataRepType)dfmt );
    interp_ = DataInterp::create( dc, true );

    HGlobIdx gidx; // will be ignored
    Dimensions& dms( const_cast<Dimensions&>(dims_) );
    strm_->getBin( dms.first ).getBin( dms.second ).getBin( dms.third );
    strm_->getBin( gidx.first ).getBin( gidx.second );
    strm_->getBin( start_.first ).getBin( start_.second );

    const int nrscalebytes = 2 * sizeof(float);
    char* buf = new char [nrscalebytes];
    strm_->getBin( buf, nrscalebytes );
    bool havescaler = false;
    for ( int idx=0; idx<nrscalebytes; idx++ )
    {
	if ( buf[idx] != 0 )
	    { havescaler = true; break; }
    }
    if ( havescaler )
    {
	const float* vals = (const float*)buf;
	scaler_ = new LinScaler( vals[0], vals[1] );
    }
    delete [] buf;

    if ( !strm_->isOK() )
    {
	closeStream();
	uirv.set( tr("%1: unexpected en of file.").arg( fnm ) );
	return;
    }

    createOffsetTable();
}


void Seis::Blocks::FileColumn::retire()
{
    closeStream();
    wasretired_ = true;
}


void Seis::Blocks::FileColumn::closeStream()
{
    delete strm_; strm_ = 0;
}


void Seis::Blocks::FileColumn::createOffsetTable()
{
    const ZGeom& zgeom = rdr_.zGeom();
    Interval<float> zrg( zgeom.start, zgeom.stop );
    if ( rdr_.seldata_ )
    {
	zrg.limitTo( rdr_.seldata_->zRange() );
	zrg.start = zgeom.snap( zrg.start );
	zrg.stop = zgeom.snap( zrg.stop );
    }
    zstart_ = zrg.start;

    const int nrsamplesinfile = zgeom.nrSteps() + 1;
    const int nrbytespersample = interp_->nrBytes();
    const od_stream_Pos nrbytespercompslice = ((od_stream_Pos)dims_.inl())
				  * dims_.crl() * nrbytespersample;

    Interval<IdxType> globzidxrg(
	    Block::globIdx4Z( zgeom, zrg.start, dims_.z() ),
	    Block::globIdx4Z( zgeom, zrg.stop, dims_.z() ) );
    nrsamplesintrace_ = 0;
    int nrfilesamplessofar = 0;
    od_stream_Pos blockstartoffs = headernrbytes_;
    for ( IdxType gzidx=globzidxrg.start; gzidx<=globzidxrg.stop; gzidx++ )
    {
	Dimensions rddims( dims_ );
	if ( gzidx == globzidxrg.stop )
	    rddims.z() = SzType(nrsamplesinfile - nrfilesamplessofar);
	nrfilesamplessofar += rddims.z();

	IdxType startzidx = 0;
	if ( gzidx == globzidxrg.start )
	{
	    startzidx = Block::locIdx4Z( zgeom, zrg.start, dims_.z() );
	    rddims.z() = dims_.z() - startzidx;
	}
	if ( gzidx == globzidxrg.stop )
	    rddims.z() = Block::locIdx4Z( zgeom, zrg.stop, dims_.z() )
		     - startzidx + 1;

	const od_stream_Pos blocknrbytes = nrbytespercompslice * rddims.z();

	for ( int icomp=0; icomp<nrcomps_; icomp++ )
	{
	    if ( rdr_.compsel_[icomp] )
	    {
		Chunk* chunk = new Chunk;
		chunk->comp_ = icomp;
		chunk->offs_ = blockstartoffs + startzidx * nrbytespersample;
		chunk->startsamp_ = nrsamplesintrace_;
		chunk->nrsamps_ = rddims.z();
		chunk->trcpartnrbytes_ = rddims.z() * nrbytespersample;
		chunks_ += chunk;
	    }
	    blockstartoffs += blocknrbytes;
	}

	nrsamplesintrace_ += rddims.z();
    }

    delete [] trcpartbuf_;
    trcpartbuf_ = new char [dims_.z() * nrbytespersample];
}


void Seis::Blocks::FileColumn::fillTrace( const BinID& bid, SeisTrc& trc,
					  uiRetVal& uirv ) const
{
    const HGeom& hgeom = *rdr_.hgeom_;
    const HLocIdx locidx(
	Block::locIdx4Inl(hgeom,bid.inl(),rdr_.dims_.inl()) - start_.inl(),
	Block::locIdx4Crl(hgeom,bid.crl(),rdr_.dims_.crl()) - start_.crl() );
    if ( locidx.inl() < 0 || locidx.crl() < 0
      || locidx.inl() >= dims_.inl() || locidx.crl() >= dims_.crl() )
    {
	// something's diff between bin block and main file; a getNext() found
	// this position in the CubeData, but the position is not available
	uirv.set( tr("Location from .cube file (%1/%2) not in file")
		.arg(bid.inl()).arg(bid.crl()) );
	return;
    }

    trc.setNrComponents( nrcomponentsintrace_ );
    trc.reSize( nrsamplesintrace_, false );

    const int nrtrcs = ((int)locidx.inl()) * dims_.crl() + locidx.crl();
    for ( int idx=0; idx<chunks_.size(); idx++ )
    {
	const Chunk& chunk = *chunks_[idx];
	strm_->setReadPosition( chunk.offs_ + nrtrcs * chunk.trcpartnrbytes_ );
	strm_->getBin( trcpartbuf_, chunk.trcpartnrbytes_ );
	for ( int isamp=0; isamp<chunk.nrsamps_; isamp++ )
	{
	    float val = interp_->get( trcpartbuf_, isamp );
	    if ( scaler_ )
		val = (float)scaler_->scale( val );
	    trc.set( chunk.startsamp_ + isamp, val, chunk.comp_ );
	}
    }
}


Seis::Blocks::Reader::Reader( const char* inp )
    : hgeom_(0)
    , cubedata_(*new PosInfo::CubeData)
    , curcdpos_(*new PosInfo::CubeDataPos)
    , seldata_(0)
    , globinlidxrg_(0,0)
    , globcrlidxrg_(0,0)
{
    File::Path fp( inp );
    if ( !File::exists(inp) )
    {
	if ( !inp || !*inp )
	    state_.set( tr("No input specified") );
	else
	    state_.set( uiStrings::phrDoesntExist(toUiString(inp)) );
	return;
    }

    if ( !File::isDirectory(inp) )
	fp.setExtension( 0 );
    filenamebase_ = fp.fileName();
    fp.setFileName( 0 );
    basepath_ = fp;

    readMainFile();
}


Seis::Blocks::Reader::~Reader()
{
    delete seldata_;
    delete hgeom_;
    delete &cubedata_;
    delete &curcdpos_;
}


void Seis::Blocks::Reader::readMainFile()
{
    const BufferString fnm( mainFileName() );
    od_istream strm( mainFileName() );
    if ( !strm.isOK() )
    {
	state_.set( uiStrings::phrCannotOpen(toUiString(strm.fileName())) );
	strm.addErrMsgTo( state_ );
	return;
    }

    ascistream astrm( strm );
    if ( !astrm.isOfFileType(sKeyFileType()) )
    {
	state_.set( tr("%1\nhas wrong file type").arg(strm.fileName()) );
	return;
    }

    bool havegensection = false, havepossection = false;
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
	else
	{
	    IOPar* iop = new IOPar;
	    iop->getFrom( astrm );
	    iop->setName( sectnm.str() + FixedString(sKeySectionPre()).size() );
	    auxiops_ += iop;
	}
	if ( failed )
	{
	    state_.set( tr("%1\n'%2' section is invalid")
		    .arg(strm.fileName()).arg(sectnm) );
	    return;
	}
    }

    if ( !havegensection || !havepossection )
	state_.set( tr("%1\nlacks %1 section").arg(strm.fileName())
	       .arg( havegensection ? tr("Position") : tr("General") ) );
}


bool Seis::Blocks::Reader::getGeneralSectionData( const IOPar& iop )
{
    int ver = version_;
    iop.get( sKeyFmtVersion(), ver );
    version_ = (HdrSzVersionType)ver;
    iop.get( sKeyCubeName(), cubename_ );
    if ( cubename_.isEmpty() )
	cubename_ = filenamebase_;
    iop.get( sKeySurveyName(), survname_ );

    hgeom_ = new HGeom( cubename_, ZDomain::SI() );
    hgeom_->getStructure( iop );
    iop.get( sKey::ZRange(), zgeom_ );
    DataCharacteristics::getUserTypeFromPar( iop, fprep_ );
    Scaler* scl = Scaler::get( iop );
    mDynamicCast( LinScaler*, scaler_, scl );

    int i1 = dims_.inl(), i2 = dims_.crl(), i3 = dims_.z();
    if ( !iop.get(sKeyDimensions(),i1,i2,i3) )
    {
	state_.set( tr("%1\nlacks block dimension info").arg(mainFileName()) );
	return false;
    }
    dims_.inl() = SzType(i1); dims_.crl() = SzType(i2); dims_.z() = SzType(i3);

    i1 = globinlidxrg_.start; i2 = globinlidxrg_.stop;
    iop.get( sKeyGlobInlRg(), i1, i2 );
    globinlidxrg_.start = IdxType(i1); globinlidxrg_.stop = IdxType(i2);
    i1 = globcrlidxrg_.start; i2 = globcrlidxrg_.stop;
    iop.get( sKeyGlobCrlRg(), i1, i2 );
    globcrlidxrg_.start = IdxType(i1); globcrlidxrg_.stop = IdxType(i2);

    iop.get( sKey::InlRange(), inlrg_ );
    iop.get( sKey::CrlRange(), crlrg_ );

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

    int maxinlblocks = globinlidxrg_.width() + 1;
    int maxcrlblocks = globcrlidxrg_.width() + 1;
    maxnrfiles_ = mMAX( maxinlblocks, maxcrlblocks ) * 2;
    if ( maxnrfiles_ > 1024 )
	maxnrfiles_ = 1024; // this is a *lot*, just a bit of sanity
			    // no prob anyway because we retire not destroy

    return true;
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


bool Seis::Blocks::Reader::reset( uiRetVal& uirv ) const
{
    curcdpos_.toPreStart();
    if ( !advancePos(curcdpos_) )
    {
	uirv.set( tr("No selected positions found") );
	return false;
    }

    needreset_ = false;
    return true;
}


uiRetVal Seis::Blocks::Reader::get( const BinID& bid, SeisTrc& trc ) const
{
    uiRetVal uirv;
    Threads::Locker locker( accesslock_ );

    if ( needreset_ )
    {
	if ( !reset(uirv) )
	    return uirv;
    }

    PosInfo::CubeDataPos newcdpos = cubedata_.cubeDataPos( bid );
    if ( !newcdpos.isValid() )
    {
	uirv.set( tr("Position not present: %1/%2")
		.arg( bid.inl() ).arg( bid.crl() ) );
	return uirv;
    }

    curcdpos_ = newcdpos;
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


void Seis::Blocks::Reader::doGet( SeisTrc& trc, uiRetVal& uirv ) const
{
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
    FileColumn* column = (FileColumn*)findColumn( globidx );
    if ( !column )
    {
	column = new FileColumn( *this, globidx );
	addColumn( column );
    }

    return activateColumn(column,uirv) ? column : 0;
}


bool Seis::Blocks::Reader::activateColumn( FileColumn* column,
					    uiRetVal& uirv ) const
{
    column->activate( uirv );
    if ( uirv.isError() )
	return false;

    if ( activitylist_.first() != column )
    {
	int curidx = activitylist_.indexOf( column );
	if ( curidx < 0 )
	{
	    const int cursz = activitylist_.size();
	    if ( cursz < maxnrfiles_ )
		activitylist_ += column;
	    else
	    {
		FileColumn* oldest = activitylist_.last();
		oldest->retire();
		activitylist_.replace( cursz-1, column );
	    }
	    curidx = activitylist_.size() - 1;
	}
	activitylist_.swap( curidx, 0 );
    }

    return true;
}


void Seis::Blocks::Reader::readTrace( SeisTrc& trc, uiRetVal& uirv ) const
{
    const BinID bid = cubedata_.binID( curcdpos_ );
    const HGlobIdx globidx( Block::globIdx4Inl(*hgeom_,bid.inl(),dims_.inl()),
			    Block::globIdx4Crl(*hgeom_,bid.crl(),dims_.crl()) );

    FileColumn* column = getColumn( globidx, uirv );
    if ( column )
    {
	column->fillTrace( bid, trc, uirv );
	trc.info().sampling_.start = column->zstart_;
	trc.info().sampling_.step = zgeom_.step;
	trc.info().setBinID( bid );
	trc.info().coord_ = hgeom_->transform( bid );
    }
}
