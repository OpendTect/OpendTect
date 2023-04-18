/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madstream.h"
#include "trckeyzsampling.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seisioobjinfo.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seispacketinfo.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "uistrings.h"
#include "od_iostream.h"


using namespace ODMad;

static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyWrite = "Write";
static const char* sKeyIn = "in";
static const char* sKeyStdIn = "\"stdin\"";
static const char* sKeyDataFormat = "data_format";
static const char* sKeyNativeFloat = "\"native_float\"";
static const char* sKeyAsciiFloat = "\"ascii_float\"";
static const char* sKeyPosFileName = "Pos File Name";
static const char* sKeyScons = "Scons";

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return; }
#define mErrBoolRet(s) { errmsg_ = s; return false; }


MadStream::MadStream( IOPar& par )
    : pars_(par)
    , is2d_(false),isps_(false),isbinary_(true)
    , istrm_(0),ostrm_(0)
    , seisrdr_(0),seiswrr_(0)
    , psrdr_(0),pswrr_(0)
    , trcbuf_(0),curtrcidx_(-1)
    , stortrcbuf_(0)
    , stortrcbufismine_(true)
    , iter_(0),cubedata_(0),l2ddata_(0)
    , headerpars_(0)
    , errmsg_(*new uiString(uiString::emptyString()))
    , iswrite_(false)
{
    par.getYN( sKeyWrite, iswrite_ );
    if ( iswrite_ )
    {
	PtrMan<IOPar> outpar = par.subselect( sKeyOutput );
	if (!outpar) mErrRet(tr("Output parameters missing"));

	initWrite( outpar );
    }
    else
    {
	PtrMan<IOPar> inpar = par.subselect( sKeyInput );
	if (!inpar) mErrRet(uiStrings::sInputParamsMissing());

	initRead( inpar );
    }
}


MadStream::~MadStream()
{
    deleteAndNullPtr( istrm_ );
    if ( ostrm_ )
    {
	ostrm_->flush();
        deleteAndNullPtr(ostrm_);
    }

    delete seisrdr_; delete seiswrr_;
    delete psrdr_; delete pswrr_;
    delete trcbuf_; delete iter_; delete cubedata_; delete l2ddata_;
    delete &errmsg_;
    if ( headerpars_ ) delete headerpars_;
    if ( stortrcbuf_ && stortrcbufismine_ ) delete  stortrcbuf_;
}


static bool getScriptForScons( BufferString& str )
{
    FilePath fp( str.buf() );
    if ( fp.fileName() != "SConstruct" )
	return false;

    const BufferString rsfroot = GetEnvVar("RSFROOT");
    const FilePath sconsfp( rsfroot, "bin", "scons" );
    if ( !File::exists(sconsfp.fullPath()) )
	return false;

#ifdef __win__
    BufferString scriptfile = FilePath::getTempFullPath( "mad_scons", "bat" );
    od_ostream ostrm( scriptfile );
    ostrm << "@echo off" << od_endl;
    ostrm << "Pushd " << fp.pathOnly() << od_endl;
    ostrm << sconsfp.fullPath() << od_endl;
    ostrm << "Popd" << od_endl;
#else
    BufferString scriptfile = FilePath::getTempFullPath( "mad_scons", 0 );
    od_ostream ostrm( scriptfile );
    ostrm << "#!/bin/csh -f" << od_endl;
    ostrm << "pushd " << fp.pathOnly() << od_endl;
    ostrm << sconsfp.fullPath() << od_endl;
    ostrm << "popd" << od_endl;
#endif
    ostrm.close();
    File::setPermissions( scriptfile, "744", 0 );

    str = "@";
    str += scriptfile;
    return true;
}


void MadStream::initRead( IOPar* par )
{
    const BufferString inptyp( par->find(sKey::Type()) );
    if ( inptyp == "None" || inptyp == "SU" )
	return;

    if ( inptyp == "Madagascar" )
    {
	const BufferString filenm = par->find( sKey::FileName() );
	BufferString inpstr( filenm );
	bool scons = false;
	par->getYN( sKeyScons, scons );
	if ( scons && !getScriptForScons(inpstr) )
	    return;
#ifdef __win__
	FilePath fp( GetEnvVar("RSFROOT") );
	if ( scons )
	    inpstr += " | ";
	else
	    inpstr = "@";
	inpstr += fp.add("bin").add("sfdd").fullPath();
	if ( !filenm.isEmpty() )
	    inpstr += " < \""; inpstr += filenm; inpstr += "\"";

	inpstr += " form=ascii_float out=stdout";
#endif
	const char* str = inpstr.isEmpty() ? 0 : inpstr.buf();
        istrm_ = new od_istream( str );

	fillHeaderParsFromStream();
	if (!headerpars_) mErrRet(tr("Error reading RSF header"));;

	const BufferString insrc( headerpars_->find(sKeyIn) );
	if ( insrc.isEmpty() || insrc == sKeyStdIn )
	    return;

        deleteAndNullPtr( istrm_ );

	istrm_ = new od_istream( insrc );
	if ( !istrm_->isOK() )
	    mErrRet( uiStrings::phrCannotRead(tr("RSF data file")));

	headerpars_->set( sKeyIn, sKeyStdIn );
	return;
    }

    const Seis::GeomType gt = Seis::geomTypeOf( inptyp );
    is2d_ = Seis::is2D( gt );
    isps_ = Seis::isPS( gt );
    MultiID inpid;
    if (!par->get(sKey::ID(), inpid)) mErrRet(tr("Input ID missing"));

    PtrMan<IOObj> ioobj = IOM().get( inpid );
    if (!ioobj) mErrRet(tr("Cannot find input data"));

    PtrMan<IOPar> subpar = par->subselect( sKey::Subsel() );
    PtrMan<Seis::SelData> seldata = Seis::SelData::get( *subpar );
    const Pos::GeomID geomid = seldata->geomID();
    if ( !isps_ )
    {
	seisrdr_ = new SeisTrcReader( *ioobj, geomid, &gt );
	if ( seldata && !seldata->isAll() )
	    seisrdr_->setSelData( seldata.release() );

	if ( !seisrdr_->prepareWork() )
	    mErrRet(seisrdr_->errMsg());
	fillHeaderParsFromSeis();
    }
    else
    {
	if ( is2d_ )
	    psrdr_ = SPSIOPF().get2DReader( *ioobj,
					     Survey::GM().getName(geomid) );
	else
	    psrdr_ = SPSIOPF().get3DReader( *ioobj );

	if (!psrdr_) mErrRet(uiStrings::sCantReadInp());

	fillHeaderParsFromPS( seldata );
    }
}


void MadStream::initWrite( IOPar* par )
{
    const BufferString outptyp( par->find(sKey::Type()) );
    const Seis::GeomType gt = Seis::geomTypeOf( outptyp );
    is2d_ = Seis::is2D( gt );
    isps_ = Seis::isPS( gt );

    istrm_ = new od_istream( od_stream::sStdIO() );
    MultiID outpid;
    if (!par->get(sKey::ID(), outpid))
	mErrRet(uiStrings::phrCannotRead( tr("paramter file")) );

    PtrMan<IOObj> ioobj = IOM().get( outpid );
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFindDBEntry(toUiString(outpid)) );

    PtrMan<IOPar> subpar = par->subselect( sKey::Subsel() );
    PtrMan<Seis::SelData> seldata = subpar ? Seis::SelData::get(*subpar)
					   : nullptr;
    const Pos::GeomID geomid = seldata ? seldata->geomID() : mUdfGeomID;
    if ( !isps_ )
    {
	seiswrr_ = new SeisTrcWriter( *ioobj, geomid, &gt );
	if ( !seiswrr_ )
	    mErrRet(toUiString("Internal: Cannot create writer"))

	if ( seldata && !seldata->isAll() )
	    seiswrr_->setSelData( seldata.release() );
    }
    else
    {
	pswrr_ = is2d_ ? SPSIOPF().get2DWriter(*ioobj,
						Survey::GM().getName(geomid))
		       : SPSIOPF().get3DWriter(*ioobj);
	if (!pswrr_) mErrRet(tr("Cannot write to output object"));
	if ( !is2d_ ) SPSIOPF().mk3DPostStackProxy( *ioobj );
    }

    fillHeaderParsFromStream();
}


BufferString MadStream::getPosFileName( bool forread ) const
{
    BufferString posfnm;
    if ( forread )
    {
	posfnm = pars_.find( sKeyPosFileName );
	if ( !posfnm.isEmpty() && File::exists(posfnm) )
	    return posfnm;

	else posfnm.setEmpty();
    }

    const BufferString typ =
	pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
				    sKey::Type()) );
    if ( typ == sKeyMadagascar )
    {
	BufferString outfnm =
	    pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
					sKey::FileName()) );
	FilePath fp( outfnm );
	fp.setExtension( "pos" );
	if ( !forread || File::exists(fp.fullPath()) )
	    posfnm = fp.fullPath();
    }
    else if ( !forread )
	posfnm = FilePath::getTempFullPath( "mad", "pos" );

    return posfnm;
}


uiString MadStream::sPosFile()
{
    return tr("Pos file");
}


#define mWriteToPosFile( obj ) \
    od_ostream strm( posfnm ); \
    if ( !strm.isOK() ) mErrRet( uiStrings::phrCannotCreate(sPosFile() )); \
    if ( !obj.write(strm,false) ) \
		{ mErrRet( uiStrings::phrCannotWrite(sPosFile() )); } \
    pars_.set( sKeyPosFileName, posfnm );

#ifdef __win__
#define mSetFormat \
    headerpars_->set( sKeyDataFormat, sKeyAsciiFloat ); \
    isbinary_ = false
#else
#define mSetFormat \
    headerpars_->set( sKeyDataFormat, sKeyNativeFloat ); \
    isbinary_ = true
#endif

void MadStream::fillHeaderParsFromSeis()
{
    if (!seisrdr_) mErrRet(uiStrings::sCantReadInp());

    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    bool needposfile = true;
    StepInterval<float> zrg;
    StepInterval<int> trcrg;
    int nrtrcs = 0;
    BufferString posfnm = getPosFileName( false );
    if ( is2d_ )
    {
	const IOObj* ioobj = seisrdr_->ioObj();
	if (!ioobj) mErrRet(tr("No nput object"));

	Seis2DDataSet dset( *ioobj );
	const Seis::SelData* seldata = seisrdr_->selData();
	if (!seldata) mErrRet(tr("Invalid data subselection"));

	const int lidx = dset.indexOf( seldata->geomID() );
	if (lidx < 0) mErrRet(tr("2D Line not found"));

	const Survey::Geometry* geom =
	    Survey::GM().getGeometry( seldata->geomID() );
	mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
	if ( !geom2d )
	    mErrRet( tr("Line geometry not available") );
	PosInfo::Line2DData l2dd = geom2d->data();

	if ( !seldata->isAll() )
	{
	    l2dd.limitTo( seldata->crlRange() );
	    StepInterval<float> gzrg( l2dd.zRange() );
	    gzrg.limitTo( seldata->zRange() );
	    l2dd.setZRange( gzrg );
	}

	nrtrcs = l2dd.positions().size();
	zrg = l2dd.zRange();
	mWriteToPosFile( l2dd )
    }
    else
    {
	SeisPacketInfo& pinfo = seisrdr_->seisTranslator()->packetInfo();
	zrg = pinfo.zrg;
	trcrg = pinfo.crlrg;

	mDynamicCastGet(const Seis::RangeSelData*,rangesel,seisrdr_->selData())
	if ( rangesel && !rangesel->isAll() )
	{
	    zrg.limitTo( rangesel->zRange() );
	    trcrg.limitTo( rangesel->crlRange() );
	}

	if ( pinfo.fullyrectandreg )
	    needposfile = false;
	else
	{
	    if (!pinfo.cubedata) mErrRet(tr("Incomplete Geometry "
					    "Information"));

	    PosInfo::CubeData newcd( *pinfo.cubedata );
	    if ( rangesel && !rangesel->isAll() )
		newcd.limitTo( rangesel->cubeSampling().hsamp_ );

	    needposfile = !newcd.isFullyRectAndReg();
	    if ( needposfile )
	    {
		nrtrcs = newcd.totalSize();
		mWriteToPosFile( newcd )
	    }
	}

	if ( !needposfile )
	{
	    StepInterval<int> inlrg = rangesel ?
		rangesel->cubeSampling().hsamp_.inlRange() : pinfo.inlrg;
	    headerpars_->set( "o3", inlrg.start );
	    headerpars_->set( "n3", inlrg.nrSteps()+1 );
	    headerpars_->set( "d3", inlrg.step );
	    if ( File::exists(posfnm) )
		File::remove( posfnm ); // While overwriting rsf
	}
    }

    if ( nrtrcs )
	headerpars_->set( "n2", nrtrcs );
    else
    {
	headerpars_->set( "o2", trcrg.start );
	headerpars_->set( "n2", trcrg.nrSteps()+1 );
	headerpars_->set( "d2", trcrg.step );
    }

    headerpars_->set( "o1", zrg.start );
    headerpars_->set( "n1", zrg.nrSteps()+1 );
    headerpars_->set( "d1", zrg.step );
    mSetFormat;
    headerpars_->set( sKeyIn, sKeyStdIn );
}


void MadStream::fillHeaderParsFromPS( const Seis::SelData* seldata )
{
    if (!psrdr_) mErrRet(uiStrings::sCantReadInp());

    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    StepInterval<float> zrg;
    int nrbids = 0;
    BufferString posfnm = getPosFileName( false );
    BinID firstbid;
    SeisTrcBuf trcbuf( true );
    if ( is2d_ )
    {
	mDynamicCastGet(SeisPS2DReader*,rdr,psrdr_);
	l2ddata_ = new PosInfo::Line2DData( rdr->posData() );
	if ( seldata && !seldata->isAll() )
	{
	    l2ddata_->limitTo( seldata->crlRange() );
	    StepInterval<float> ldzrg( l2ddata_->zRange() );
	    ldzrg.limitTo( seldata->zRange() );
	    l2ddata_->setZRange( ldzrg );
	}

	nrbids = l2ddata_->positions().size();
	if (!nrbids) mErrRet(tr("No data available in the given range"));

	mWriteToPosFile( (*l2ddata_) );
	firstbid.crl() = l2ddata_->positions()[0].nr_;

	if ( !rdr->getGather(firstbid,trcbuf) )
	    mErrRet(tr("No data to read"));
    }
    else
    {
	mDynamicCastGet(SeisPS3DReader*,rdr,psrdr_);
	cubedata_ = new PosInfo::CubeData( rdr->posData() );
	if ( seldata )
	{
	    TrcKeySampling hs;
	    hs.set( seldata->inlRange(), seldata->crlRange() );
	    cubedata_->limitTo( hs );
	}

	nrbids = cubedata_->totalSize();
	if (!nrbids) mErrRet(tr("No data available in the given range"));
	mWriteToPosFile( (*cubedata_) );

	int idx=0;
	while (idx<cubedata_->size() && (*cubedata_)[idx]
		&& !(*cubedata_)[idx]->segments_.size())
	    idx++;

	iter_ = new PosInfo::CubeDataIterator( *cubedata_ );
	firstbid.inl() = (*cubedata_)[idx]->linenr_;
	firstbid.crl() = (*cubedata_)[idx]->segments_[0].start;

	if ( !rdr->getGather(firstbid,trcbuf) )
	    mErrRet(tr("No data to read"));
    }

    nroffsets_ = trcbuf.size();
    if ( nroffsets_ < 2 )
	mErrRet(tr("First position has only one trace in the gather. \
		    Please ensure regular offset throught the data"))

    SeisTrc* firsttrc = trcbuf.get(0);
    SeisTrc* nexttrc = trcbuf.get(1);
    if ( !firsttrc || !nexttrc || !firsttrc->size() || !nexttrc->size() )
	mErrRet(tr("No data to read"));

    headerpars_->set( "n3", nrbids );

    headerpars_->set( "o2", firsttrc->info().offset );
    headerpars_->set( "d2", nexttrc->info().offset - firsttrc->info().offset );
    headerpars_->set( "n2", nroffsets_ );

    headerpars_->set( "o1", firsttrc->info().sampling.start );
    headerpars_->set( "d1", firsttrc->info().sampling.step );
    headerpars_->set( "n1", firsttrc->size() );
    mSetFormat;
    headerpars_->set( sKeyIn, sKeyStdIn );
}


static bool isAtEndOfRSFHeader( od_istream& strm )
{
    for ( int idx=0; idx<3; idx++ )
    {
	if ( strm.peek() == sKeyRSFEndOfHeader[idx] )
	    strm.ignore( 1 );
	else
	    return false;
    }

    return true;
}


void MadStream::fillHeaderParsFromStream()
{
    delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    if ( !istrm_ ) return;

    while ( istrm_->isOK() && !isAtEndOfRSFHeader(*istrm_) )
    {
	BufferString linebuf;
	if ( !istrm_->getLine(linebuf) )
	    break;

	char* valstr = linebuf.find( '=' );
	if ( !valstr )continue;

	*valstr = '\0'; valstr++;
	linebuf.trimBlanks();
	headerpars_->set( linebuf.buf(), valstr );
    }

    if ( !headerpars_->size() )
	mErrRet(tr("Empty or corrupt RSF Header"))

    BufferString dataformat;
    if ( headerpars_->get(sKeyDataFormat,dataformat)
	    && dataformat == sKeyAsciiFloat )
	isbinary_ = false;
}


int MadStream::getNrSamples() const
{
    int nrsamps = 0;
    if ( headerpars_ ) headerpars_->get( "n1", nrsamps );

    return nrsamps;
}


bool MadStream::isOK() const
{
    if (errMsg().isSet()) return false;

    return true;
}


uiString MadStream::errMsg() const
{
    return errmsg_;
}


bool MadStream::putHeader( od_ostream& strm )
{
    if (!headerpars_)
	mErrBoolRet(tr("Header parameters not found"));

    IOParIterator iter( *headerpars_ );
    BufferString key, val;
    while ( iter.next(key,val) )
	strm << "\t" << key << "=" << val << od_endl;

    strm << sKeyRSFEndOfHeader;
    return true;
}


bool MadStream::getNextPos( BinID& bid )
{
    if ( !is2d_ )
	return iter_ && iter_->next( bid );

    if ( !l2ddata_ || l2ddata_->isEmpty() ) return false;

    const TypeSet<PosInfo::Line2DPos>& posns = l2ddata_->positions();
    if ( !bid.crl() )
	{ bid.crl() = posns[0].nr_; return true; }

    int idx = 0;
    while ( idx<posns.size() && bid.crl() != posns[idx].nr_ )
	idx++;

    if ( idx+1 >= posns.size() )
	return false;

    bid.crl() = posns[idx+1].nr_;
    return true;
}


bool MadStream::getNextTrace( float* arr )
{
    if ( istrm_ && istrm_->isOK() )
    {
	const int nrsamps = getNrSamples();
	readRSFTrace( arr, nrsamps );
	return !istrm_->isBad();
    }
    else if ( seisrdr_ )
    {
	SeisTrc trc;
	const uiString errmsg = uiStrings::phrCannotRead( tr("traces") );
	const int rv = seisrdr_->get( trc.info() );
	if (rv < 0) mErrBoolRet( errmsg )
	else if ( rv == 0 ) return false;

	if (!seisrdr_->get(trc)) mErrBoolRet(errmsg);
	for ( int idx=0; idx<trc.size(); idx++ )
	    arr[idx] = trc.get( idx, 0 );

	return true;
    }
    else if ( psrdr_ )
    {
	if ( !trcbuf_ ) trcbuf_ = new SeisTrcBuf( true );

	if ( curtrcidx_ < 0 || curtrcidx_ >= nroffsets_ )
	{
	    if ( !getNextPos(curbid_) || !psrdr_->getGather(curbid_,*trcbuf_) )
		return false;

	    curtrcidx_ = 0;
	}

	const int nrsamps = getNrSamples();
	SeisTrc* trc = 0;
	if ( curtrcidx_ < trcbuf_->size() )
	    trc = trcbuf_->get( curtrcidx_ );

	for ( int idx=0; idx<nrsamps; idx++ )
	    arr[idx] = trc ? trc->get(idx,0) : 0;

	curtrcidx_++;
	return true;
    }

    mErrBoolRet(tr("No data source found"));
}


void MadStream::readRSFTrace( float* arr, int nrsamps ) const
{
    if ( isbinary_ )
    {
	istrm_->getBin( arr, nrsamps*sizeof(float) );
	return;
    }

    for ( int idx=0; idx<nrsamps; idx++ ) \
	    *istrm_ >> arr[idx];
}


uiString MadStream::sNoPositionsInPosFile()
{
    return tr("No positions in Pos File");
}


#define mReadFromPosFile( obj ) \
    BufferString posfnm = getPosFileName( true ); \
    if ( !posfnm.isEmpty() ) \
    { \
	haspos = true; \
	od_istream strm( posfnm ); \
	if ( !strm.isOK() ) mErrBoolRet(uiStrings::phrCannotOpen(sPosFile())); \
	if ( !obj.read(strm,false) ) \
	    mErrBoolRet( uiStrings::phrCannotRead(sPosFile()) ); \
	if ( obj.isEmpty() ) \
	    mErrBoolRet( sNoPositionsInPosFile() ); \
	strm.close(); \
	if ( FilePath(posfnm).pathOnly() == FilePath::getTempDir() ) \
	    File::remove( posfnm ); \
    }

bool MadStream::writeTraces( bool writetofile )
{
    if ( writetofile && ( ( isps_ && !pswrr_ ) || ( !isps_ && !seiswrr_ ) ) )
	mErrBoolRet(tr("Cannot initialize writing"));

    if ( is2d_ )
	return write2DTraces( writetofile );

    int inlstart=0, crlstart=1, inlstep=1, crlstep=1, nrinl=1, nrcrl=1, nrsamps;
    int nrtrcsperbinid=1, nrbinids=0;
    SamplingData<float> offsetsd( 0, 1 );
    SamplingData<float> sd;
    bool haspos = false;
    PosInfo::CubeData cubedata;
    mReadFromPosFile( cubedata );
    headerpars_->get( "o1", sd.start );
    headerpars_->get( "d1", sd.step );
    headerpars_->get( "n1", nrsamps );
    if ( !isps_ )
    {
	nroffsets_ = 1;
	headerpars_->get( "o2", crlstart );
	headerpars_->get( "o3", inlstart );
	headerpars_->get( "n2", nrcrl );
	headerpars_->get( "n3", nrinl );
	headerpars_->get( "d2", crlstep );
	headerpars_->get( "d3", inlstep );
    }
    else
    {
	if ( !haspos )
	    mErrBoolRet(tr("Geometry data missing"));

	headerpars_->get( "n2", nrtrcsperbinid );
	headerpars_->get( "n3", nrbinids );
	headerpars_->get( "o2", offsetsd.start );
	headerpars_->get( "d2", offsetsd.step );
    }

    if ( haspos )
	nrinl = cubedata.size();

    float* buf = new float[nrsamps];
    for ( int inlidx=0; inlidx<nrinl; inlidx++ )
    {
	const int inl = haspos ? cubedata[inlidx]->linenr_
				: inlstart + inlidx * inlstep;

	const int nrsegs = haspos ? cubedata[inlidx]->segments_.size() : 1;
	for ( int segidx=0; segidx<nrsegs; segidx++ )
	{
	    if ( haspos )
	    {
		StepInterval<int> seg = cubedata[inlidx]->segments_[segidx];
		crlstart = seg.start; crlstep = seg.step;
		nrcrl = seg.nrSteps() + 1;
	    }

	    for ( int crlidx=0; crlidx<nrcrl; crlidx++ )
	    {
		int crl = crlstart + crlidx * crlstep;
		for ( int trcidx=1; trcidx<=nrtrcsperbinid; trcidx++ )
		{
		    readRSFTrace( buf, nrsamps );
		    auto* trc = new SeisTrc( nrsamps );
		    trc->info().sampling = sd;
		    trc->info().seqnr_ = trcidx;
		    trc->info().setPos( BinID(inl,crl) );
		    trc->info().calcCoord();
		    if ( isps_ )
			trc->info().offset = offsetsd.atIndex( trcidx - 1 );

		    for ( int isamp=0; isamp<nrsamps; isamp++ )
			trc->set( isamp, buf[isamp], 0 );

		    if ( writetofile )
		    {
			if ( ( isps_ && !pswrr_->put (*trc) )
			    || ( !isps_ && !seiswrr_->put(*trc) ) )
			{
			    delete [] buf; delete trc;
			    mErrBoolRet(tr("Cannot write trace"));
			}
			delete trc;
		    }
		    else
		    {
			if ( !stortrcbuf_ )
			    stortrcbuf_ = new SeisTrcBuf( true );

			stortrcbuf_->add( trc );
		    }
		}
	    }
	}
    }

    delete [] buf;
    return true;
}


bool MadStream::write2DTraces( bool writetofile )
{
    PosInfo::Line2DData geom;
    bool haspos = false;
    mReadFromPosFile ( geom );
    if (!haspos) mErrBoolRet(tr("Position data not available"));

    const Pos::GeomID gid = Survey::GM().getGeomID( geom.lineName() );

    int nrtrcs=0, nrsamps=0, nroffsets=1;
    SamplingData<float> offsetsd( 0, 1 );
    SamplingData<float> sd;

    headerpars_->get( "o1", sd.start );
    headerpars_->get( "n1", nrsamps );
    headerpars_->get( "d1", sd.step );

    if ( !isps_ )
	headerpars_->get( "n2", nrtrcs );
    else
    {
	headerpars_->get( "n2", nroffsets );
	headerpars_->get( "o2", offsetsd.start );
	headerpars_->get( "d2", offsetsd.step );
	headerpars_->get( "n3", nrtrcs );
    }

    const TypeSet<PosInfo::Line2DPos>& posns = geom.positions();
    if ( nrtrcs != posns.size() )
	mErrBoolRet(tr("Line geometry doesn't match with data"));

    float* buf = new float[nrsamps];
    for ( int idx=0; idx<posns.size(); idx++ )
    {
	const int trcnr = posns[idx].nr_;
	for ( int offidx=0; offidx<nroffsets; offidx++ )
	{
	    readRSFTrace( buf, nrsamps );
	    auto* trc = new SeisTrc( nrsamps );
	    trc->info().sampling = sd;
	    trc->info().seqnr_ = trcnr;
	    trc->info().setGeomID( gid ).setTrcNr( trcnr );
	    trc->info().coord = posns[idx].coord_;
	    if ( isps_ )
		trc->info().offset = offsetsd.atIndex( offidx );

	    for ( int isamp=0; isamp<nrsamps; isamp++ )
		trc->set( isamp, buf[isamp], 0 );

	    if ( writetofile )
	    {
		if ( ( isps_ && !pswrr_->put (*trc) )
			|| ( !isps_ && !seiswrr_->put(*trc) ) )
		{
		    delete [] buf; delete trc;
		    mErrBoolRet(tr("Error writing traces"));
		}
		delete trc;
	    }
	    else
	    {
		if ( !stortrcbuf_ )
		    stortrcbuf_ = new SeisTrcBuf( true );

		stortrcbuf_->add( trc );
	    }
	}
    }

    delete [] buf;
    return true;
}
#undef mErrRet
