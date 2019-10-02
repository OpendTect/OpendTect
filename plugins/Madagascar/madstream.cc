/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
-*/


#include "madstream.h"
#include "cubedata.h"
#include "cubesubsel.h"
#include "trckeyzsampling.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "horsubsel.h"
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
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seispacketinfo.h"
#include "seisstorer.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "uistrings.h"
#include "od_iostream.h"
#include <iostream>


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
    , seisprov_(0), seisstorer_(0)
    , psrdr_(0),pswrr_(0)
    , trcbuf_(0),curtrcidx_(-1)
    , stortrcbuf_(0)
    , stortrcbufismine_(true)
    , iter_(0),cubedata_(0),l2ddata_(0)
    , headerpars_(0)
    , errmsg_(*new uiString(uiString::empty()))
    , iswrite_(false)
{
    par.getYN( sKeyWrite, iswrite_ );
    if ( iswrite_ )
    {
	PtrMan<IOPar> outpar = par.subselect( sKeyOutput );
	if ( !outpar )
	    mErrRet( uiStrings::sParsMissing()
		     .addMoreInfo(uiStrings::sOutput()) )

	initWrite( outpar );
    }
    else
    {
	PtrMan<IOPar> inpar = par.subselect( sKeyInput );
	if ( !inpar )
	    mErrRet( uiStrings::sParsMissing().addMoreInfo(uiStrings::sInput()))

	initRead( inpar );
    }
}


MadStream::~MadStream()
{
    deleteAndZeroPtr( istrm_ );
    if ( ostrm_ )
    {
	ostrm_->flush();
        deleteAndZeroPtr(ostrm_);
    }

    delete seisprov_; delete seisstorer_;
    delete psrdr_; delete pswrr_;
    delete trcbuf_; delete iter_; delete cubedata_; delete l2ddata_;
    delete &errmsg_;
    if ( headerpars_ ) delete headerpars_;
    if ( stortrcbuf_ && stortrcbufismine_ ) delete  stortrcbuf_;
}


static bool getScriptForScons( BufferString& str )
{
    File::Path fp( str.buf() );
    if ( fp.fileName() != "SConstruct" )
	return false;

    const BufferString rsfroot = GetEnvVar("RSFROOT");
    const File::Path sconsfp( rsfroot, "bin", "scons" );
    if ( !File::exists(sconsfp.fullPath()) )
	return false;

#ifdef __win__
    BufferString scriptfile = File::Path::getTempFullPath( "mad_scons", "bat" );
    od_ostream ostrm( scriptfile );
    ostrm << "@echo off" << od_endl;
    ostrm << "Pushd " << fp.pathOnly() << od_endl;
    ostrm << sconsfp.fullPath() << od_endl;
    ostrm << "Popd" << od_endl;
#else
    BufferString scriptfile = File::Path::getTempFullPath( "mad_scons", 0 );
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
    BufferString inptyp( par->find(sKey::Type()) );
    if ( inptyp == "None" || inptyp == "SU" )
	return;

    if ( inptyp == "Madagascar" )
    {
	const char* filenm = par->find( sKey::FileName() );

	BufferString inpstr( filenm );
	bool scons = false;
	par->getYN( sKeyScons, scons );
	if ( scons && !getScriptForScons(inpstr) )
	    return;
#ifdef __win__
	File::Path fp( GetEnvVar("RSFROOT") );
	if ( scons )
	    inpstr += " | ";
	else
	    inpstr = "@";
	inpstr += fp.add("bin").add("sfdd").fullPath();
	if ( filenm && *filenm )
	    inpstr += " < \""; inpstr += filenm; inpstr += "\"";

	inpstr += " form=ascii_float out=stdout";
#endif
	const char* str = inpstr.isEmpty() ? 0 : inpstr.buf();
	istrm_ = new od_istream( str );

	fillHeaderParsFromStream();
	if ( !headerpars_ )
	    mErrRet(uiStrings::phrCannotRead(tr("RSF header")));

	BufferString insrc( headerpars_->find(sKeyIn) );
	if ( insrc == "" || insrc == sKeyStdIn )
	    return;

        deleteAndZeroPtr( istrm_ );
	istrm_ = new od_istream( insrc );
	if ( !istrm_->isOK() )
	    mErrRet( uiStrings::phrCannotRead(tr("RSF data file")));

	headerpars_->set( sKeyIn, sKeyStdIn );
	return;
    }

    Seis::GeomType gt = Seis::geomTypeOf( inptyp );
    is2d_ = gt == Seis::Line || gt == Seis::LinePS;
    isps_ = gt == Seis::VolPS || gt == Seis::LinePS;
    DBKey inpid;
    if ( !par->get(sKey::ID(), inpid) )
	mErrRet(tr("Input ID missing"));

    PtrMan<IOObj> ioobj = getIOObj( inpid );
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFindDBEntry(inpid) );

    PtrMan<IOPar> subpar = par->subselect( sKey::Subsel() );
    auto* seldata = Seis::SelData::get( *subpar );
    if ( !isps_ )
    {
	uiRetVal uirv;
	seisprov_ = Seis::Provider::create( inpid, &uirv );
	if ( !seisprov_ )
	    mErrRet( uirv );
	seisprov_->setSelData( seldata );
	fillHeaderParsFromSeis();
    }
    else
    {
	const Pos::GeomID geomid = seldata->geomID();
	if ( is2d_ )
	    psrdr_ = SPSIOPF().get2DReader( *ioobj, geomid.name() );
	else
	    psrdr_ = SPSIOPF().get3DReader( *ioobj );

	if ( !psrdr_ )
	    mErrRet(uiStrings::phrCannotReadInp());

	fillHeaderParsFromPS( seldata );
    }
}


void MadStream::initWrite( IOPar* par )
{
    BufferString outptyp( par->find(sKey::Type()) );
    Seis::GeomType gt = Seis::geomTypeOf( outptyp );

    is2d_ = gt == Seis::Line || gt == Seis::LinePS;
    isps_ = gt == Seis::VolPS || gt == Seis::LinePS;
    istrm_ = new od_istream( od_stream::sStdIO() );
    DBKey outpid;
    if (!par->get(sKey::ID(), outpid))
	mErrRet(uiStrings::phrCannotRead( tr("paramter file")) );

    PtrMan<IOObj> ioobj = getIOObj( outpid );
    if ( !ioobj )
	mErrRet( uiStrings::phrCannotFindDBEntry(outpid) );

    PtrMan<IOPar> subpar = par->subselect( sKey::Subsel() );
    Seis::SelData* seldata = subpar ? Seis::SelData::get(*subpar) : 0;
    if ( !isps_ )
    {
	seisstorer_ = ioobj ? new Seis::Storer( *ioobj ) : 0;
	if ( !seisstorer_ )
	    mErrRet(mINTERNAL("Cannot create Storer"))
    }
    else
    {
	const Pos::GeomID geomid = seldata ? seldata->geomID() : mUdfGeomID;
	pswrr_ = is2d_ ? SPSIOPF().get2DWriter(*ioobj,geomid.name())
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

    BufferString typ =
	pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
				    sKey::Type()) );
    if ( typ == sKeyMadagascar )
    {
	BufferString outfnm =
	    pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
					sKey::FileName()) );
	File::Path fp( outfnm );
	fp.setExtension( "pos" );
	if ( !forread || File::exists(fp.fullPath()) )
	    posfnm = fp.fullPath();
    }
    else if ( !forread )
	posfnm = File::Path::getTempFullPath( "mad", "pos" );

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
    if ( !seisprov_ )
	mErrRet(uiStrings::phrCannotReadInp());

    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    bool needposfile = true;
    StepInterval<float> zrg;
    StepInterval<int> trcrg;
    int nrtrcs = 0;
    BufferString posfnm = getPosFileName( false );
    if ( is2d_ )
    {
	PtrMan<IOObj> ioobj = getIOObj( seisprov_->dbKey() );
	if (!ioobj) mErrRet(tr("No input object"));

	Seis2DDataSet dset( *ioobj );
	const Seis::SelData* seldata = seisprov_->selData();
	if (!seldata) mErrRet(tr("Invalid data subselection"));

	const int lidx = dset.indexOf( seldata->geomID() );
	if (lidx < 0) mErrRet(tr("2D Line not found"));

	const auto& geom2d = SurvGeom::get2D( seldata->geomID() );
	if ( geom2d.isEmpty() )
	    mErrRet( tr("Line geometry not available") );

	PosInfo::Line2DData l2dd = geom2d.data();
	if ( !seldata->isAll() )
	{
	    const auto crlrg = seldata->crlRange();
	    l2dd.limitTo( crlrg.start, crlrg.stop );
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
	Survey::FullSubSel fss;
	seisprov_->getFullSubSel( fss );
	const TrcKeyZSampling tkzs( fss );
	zrg = tkzs.zsamp_;
	trcrg = tkzs.hsamp_.trcRange();

	auto* rgsel = seisprov_->selData()->asRange();
	if ( rgsel && !rgsel->isAll() )
	{
	    zrg.limitTo( rgsel->zRange() );
	    trcrg.limitTo( rgsel->crlRange() );
	}

	const auto& prov3d = *seisprov_->as3D();
	PosInfo::CubeData newcd = prov3d.possibleCubeData();
	if ( rgsel && !rgsel->isAll() )
	    newcd.limitTo( rgsel->horSubSel() );

	needposfile = !newcd.isFullyRegular();
	if ( needposfile )
	{
	    nrtrcs = newcd.totalSize();
	    mWriteToPosFile( newcd )
	}

	if ( !needposfile )
	{
	    StepInterval<int> inlrg;
	    if ( rgsel )
		inlrg = rgsel->fullSubSel().inlRange();
	    else
		newcd.getInlRange( inlrg );

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
    if ( !psrdr_ )
	mErrRet( uiStrings::phrCannotReadInp() );

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
	    const auto crlrg = seldata->crlRange();
	    l2ddata_->limitTo( crlrg.start, crlrg.stop );
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
	    CubeHorSubSel css( seldata->inlRange(), seldata->crlRange() );
	    cubedata_->limitTo( css );
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

    headerpars_->set( "o2", firsttrc->info().offset_ );
    headerpars_->set( "d2",nexttrc->info().offset_-firsttrc->info().offset_);
    headerpars_->set( "n2", nroffsets_ );

    headerpars_->set( "o1", firsttrc->info().sampling_.start );
    headerpars_->set( "d1", firsttrc->info().sampling_.step );
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
    return errMsg().isEmpty();
}


uiString MadStream::errMsg() const
{
    return errmsg_;
}


bool MadStream::putHeader( od_ostream& strm )
{
    if (!headerpars_) mErrBoolRet(tr("Header parameters not found"));

    for ( int idx=0; idx<headerpars_->size(); idx++ )
    {
	strm << "\t" << headerpars_->getKey(idx);
	strm << "=" << headerpars_->getValue(idx) << od_endl;
    }

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
    else if ( seisprov_ )
    {
	SeisTrc trc;
	const uiRetVal uirv = seisprov_->getNext( trc );
	if ( !uirv.isOK() )
	{
	    if ( !isFinished(uirv) )
		errmsg_ = uirv;
	    return false;
	}

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
	if ( !strm.isOK() ) \
	    mErrBoolRet(uiStrings::phrCannotOpen(sPosFile())); \
	if ( !obj.read(strm,false) ) \
	    mErrBoolRet( uiStrings::phrCannotRead(sPosFile()) ); \
	if ( obj.isEmpty() ) \
	    mErrBoolRet( sNoPositionsInPosFile() ); \
	strm.close(); \
	if ( File::Path(posfnm).pathOnly() == File::Path::getTempDir() ) \
	    File::remove( posfnm ); \
    }

bool MadStream::writeTraces( bool writetofile )
{
    if ( writetofile && ( ( isps_ && !pswrr_ ) || ( !isps_ && !seisstorer_ ) ) )
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
		    SeisTrc* trc = new SeisTrc( nrsamps );
		    trc->info().sampling_ = sd;
		    trc->info().setPos( BinID(inl,crl) );
		    if ( isps_ )
			trc->info().offset_ = offsetsd.atIndex( trcidx - 1 );

		    for ( int isamp=0; isamp<nrsamps; isamp++ )
			trc->set( isamp, buf[isamp], 0 );

		    if ( writetofile )
		    {
			if ( ( isps_ && !pswrr_->put (*trc) )
			    || ( !isps_ && !seisstorer_->put(*trc).isOK() ) )
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
	    SeisTrc* trc = new SeisTrc( nrsamps );
	    trc->info().sampling_ = sd;
	    trc->info().coord_ = posns[idx].coord_;
	    trc->info().setTrcNr( trcnr );
	    if ( isps_ )
		trc->info().offset_ = offsetsd.atIndex( offidx );

	    for ( int isamp=0; isamp<nrsamps; isamp++ )
		trc->set( isamp, buf[isamp], 0 );

	    if ( writetofile )
	    {
		if ( ( isps_ && !pswrr_->put (*trc) )
			|| ( !isps_ && !seisstorer_->put(*trc).isOK() ) )
		{
		    delete [] buf; delete trc;
		    mErrBoolRet(uiStrings::phrErrDuringWrite());
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
