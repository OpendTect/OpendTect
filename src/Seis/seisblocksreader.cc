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

			FileColumn(const Reader&,const GlobIdx&);
			~FileColumn()		{ retire(); delete scaler_; }

    void		activate(uiRetVal&);
    void		retire();

    void		getTrace(const BinID&,SeisTrc&,uiRetVal&);

    const Reader&	rdr_;
    od_istream*		strm_;

    SampIdx		start_;
    int			nrcomponents_;
    int			nrsamplesinfile_;
    int			nrsamplesintrace_;
    DataInterpreter<float>* interp_;
    LinScaler*		scaler_;


protected:

    void		createOffsetTable();

};

} // namespace Blocks

} // namespace Seis


Seis::Blocks::FileColumn::FileColumn( const Reader& rdr, const GlobIdx& gidx )
    : Column(gidx,Dimensions(0,0,0),rdr.nrComponents())
    , rdr_(rdr)
    , start_(0,0,0)
    , interp_(0)
    , strm_(0)
    , scaler_(0)
    , nrsamplesinfile_(0)
    , nrsamplesintrace_(0)
{
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
	retire();
	uirv.set( uiStrings::phrCannotOpen( toUiString(fnm) ) );
	return;
    }

    IOClass::HdrSzVersionType hdrsz, version;
    strm_->getBin( hdrsz ).getBin( version );
    IOClass::HdrSzVersionType expectedhdrsz = rdr_.columnHeaderSize( version );
    if ( hdrsz != expectedhdrsz )
    {
	retire();
	uirv.set( tr("%1: unexpected header size.\nFound %2, should be %3.")
	          .arg( fnm ).arg( hdrsz ).arg( expectedhdrsz ) );
	return;
    }

    GlobIdx gidx; // not using inl/crl
    Dimensions& dims( const_cast<Dimensions&>(dims_) );
    strm_->getBin( gidx.first ).getBin( gidx.second ).getBin( gidx.third );
    strm_->getBin( start_.first ).getBin( start_.second ).getBin( start_.third);
    strm_->getBin( dims.first ).getBin( dims.second ).getBin( dims.third );
    strm_->getBin( nrsamplesinfile_ );
    OD::FPDataRepType fprep;
    strm_->getBin( fprep );
    const DataCharacteristics dc( fprep );
    interp_ = DataInterpreter<float>::create( dc, true );

    const int nrscalebytes = 2 * sizeof(float);
    char* buf = new char [nrscalebytes];
    strm_->getBin( buf, nrscalebytes );
    bool havescaler = false;
    for ( int idx=0; idx<nrscalebytes; idx++ )
    {
	if ( buf[idx] != 0 )
	    havescaler = true;
    }
    if ( havescaler )
    {
	const float* vals = (const float*)buf;
	scaler_ = new LinScaler( vals[0], vals[1] );
    }
    delete [] buf;

    const int bytesleft = (int)hdrsz - (int)strm_->position();
    buf = new char [bytesleft];
    strm_->getBin( buf, bytesleft );
    delete [] buf;
    if ( !strm_->isOK() )
    {
	retire();
	uirv.set( tr("%1: unexpected en of file.").arg( fnm ) );
	return;
    }
}


void Seis::Blocks::FileColumn::retire()
{
    delete strm_; strm_ = 0;
}


void Seis::Blocks::FileColumn::createOffsetTable()
{
    Interval<float> zrg;
    const SurvGeom& survgeom = *rdr_.survgeom_;
    zrg.start = Block::z4Idxs( survgeom, dims_.z(), globidx_.z(), start_.z() );
    zrg.stop = zrg.start + (nrsamplesinfile_-1) * survgeom.zStep();
    if ( rdr_.seldata_ )
	zrg.limitTo( rdr_.seldata_->zRange() );

    const od_stream_Pos singcompslicesize = ((od_stream_Pos)dims_.inl())
				  * dims_.crl() * interp_->nrBytes();
    const od_stream_Pos slicesize = singcompslicesize * nrcomps_;
    const od_stream_Pos firstblocksize = slicesize * (dims_.z()-start_.z());
    const od_stream_Pos blocksize = slicesize * dims_.z();

    nrsamplesintrace_ = 0;
    const Interval<IdxType> globzidxrg(
	    Block::globIdx4Z( survgeom, zrg.start, dims_.z() ),
	    Block::globIdx4Z( survgeom, zrg.stop, dims_.z() ) );
    for ( IdxType gzidx=globzidxrg.start; gzidx<=globzidxrg.stop; gzidx++ )
    {
	const int blockidx = gzidx - globidx_.z();
	const od_stream_Pos blockoffs = blockidx == 0 ? 0
			: firstblocksize + (blockidx-1) * blocksize;

	IdxType startzidx = 0;
	SzType nrz = dims_.z();
	if ( gzidx == globzidxrg.start )
	{
	    startzidx = Block::sampIdx4Z( survgeom, zrg.start, dims_.z() );
	    nrz = dims_.z() - startzidx;
	}
	else if ( gzidx == globzidxrg.stop )
	    nrz = Block::sampIdx4Z( survgeom, zrg.stop, dims_.z() )
		- startzidx + 1;

	nrsamplesintrace_ += nrz;

	od_stream_Pos offset = blockoffs + startzidx * slicesize;
	od_stream_Pos dummy = nrz * offset;
	strm_->setPosition( dummy );
    }
}


void Seis::Blocks::FileColumn::getTrace( const BinID& bid, SeisTrc& trc,
					 uiRetVal& uirv )
{
    const SurvGeom& survgeom = *rdr_.survgeom_;
    const SampIdx sampidx(
	Block::sampIdx4Inl(survgeom,bid.inl(),dims_.inl()) - start_.inl(),
	Block::sampIdx4Crl(survgeom,bid.crl(),dims_.crl()) - start_.crl(),
	0 );

    trc.reSize( nrsamplesintrace_, false );

    uirv.set( tr("Trace read not implemented yet" ) );

    /*
    const int nrbytespersample = interp_->nrBytes();
    for ( int idx=0; idx<offstbl_.size(); idx++ )
    {
	const OffsEntry& offsentry = *offstbl_[idx];
	strm_->getBin( blockbuf_, offsentry.nrz_ * nrbytespersample );
	for ( int isamp=0; isamp<offsentry.nrz_; isamp++ )
	{
	    float val = interp_->get( isamp );
	    if ( scaler_ )
		val = scaler_->scale( val );
	    trc.set( offsentry.starttrcidx_ + isamp, val, icomp );
	}
    }
    */
}


Seis::Blocks::Reader::Reader( const char* inp )
    : survgeom_(0)
    , cubedata_(*new PosInfo::CubeData)
    , curcdpos_(*new PosInfo::CubeDataPos)
    , globinlidxrg_(0,0)
    , globcrlidxrg_(0,0)
    , globzidxrg_(0,0)
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
    delete survgeom_;
    setEmpty();
    delete &cubedata_;
    delete &curcdpos_;
}


void Seis::Blocks::Reader::setEmpty()
{
    clearColumns();
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
	if ( atEndOfSection(astrm) )
	    astrm.next();
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
    version_ = (unsigned short)ver;
    iop.get( sKeyCubeName(), cubename_ );
    if ( cubename_.isEmpty() )
	cubename_ = filenamebase_;

    survgeom_ = new SurvGeom( cubename_, ZDomain::SI() );
    survgeom_->getStructure( iop );
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
    i1 = globzidxrg_.start; i2 = globzidxrg_.stop;
    iop.get( sKeyGlobZRg(), i1, i2 );
    globzidxrg_.start = IdxType(i1); globzidxrg_.stop = IdxType(i2);

    iop.get( sKey::InlRange(), inlrg_ );
    iop.get( sKey::CrlRange(), crlrg_ );
    iop.get( sKey::ZRange(), zrg_ );

    FileMultiString fms( iop.find(sKeyComponents()) );
    const int nrcomps = fms.size();
    if ( nrcomps < 1 )
	compnms_.add( "Component 1" );
    else
    {
	for ( int icomp=0; icomp<nrcomps; icomp++ )
	    compnms_.add( fms[icomp] );
    }

    int maxinlblocks = globinlidxrg_.width() + 1;
    int maxcrlblocks = globcrlidxrg_.width() + 1;
    maxnrfiles_ = mMAX( maxinlblocks, maxcrlblocks ) + 1;

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
		const GlobIdx& globidx, uiRetVal& uirv ) const
{
    FileColumn* column = (FileColumn*)findColumn( globidx );
    if ( !column )
    {
	column = new FileColumn( *this, globidx );
	addColumn( column );
    }
    column->activate( uirv );
    //TODO retire one if too many open files
    return uirv.isError() ? 0 : column;
}


void Seis::Blocks::Reader::readTrace( SeisTrc& trc, uiRetVal& uirv ) const
{
    const BinID bid = cubedata_.binID( curcdpos_ );
    const GlobIdx globidx( Block::globIdx4Inl(*survgeom_,bid.inl(),dims_.inl()),
			   Block::globIdx4Crl(*survgeom_,bid.crl(),dims_.crl()),
			   0 );

    FileColumn* column = getColumn( globidx, uirv );
    if ( column )
	column->getTrace( bid, trc, uirv );
}
