/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
-*/

static const char* rcsID = "$Id: madstream.cc,v 1.6 2008-04-28 06:36:02 cvsraman Exp $";

#include "madstream.h"
#include "cubesampling.h"
#include "filegen.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo.h"
#include "ptrman.h"
#include "seis2dline.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "strmprov.h"
#include "survinfo.h"


using namespace ODMad;

static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyProc = "Proc";
static const char* sKeyWrite = "Write";
static const char* sKeyIn = "in";
static const char* sKeyStdIn = "stdin";
static const char* sKeyPosFileName = "Pos File Name";

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return; }
#define mErrBoolRet(s) { errmsg_ = s; return false; }

MadStream::MadStream( IOPar& par )
    : pars_(par)
    , is2d_(false),isps_(false)
    , istrm_(0),ostrm_(0)
    , seisrdr_(0),seiswrr_(0)
    , headerpars_(0)
    , errmsg_(*new BufferString(""))
    , iswrite_(false)
{
    par.getYN( sKeyWrite, iswrite_ );
    if ( iswrite_ )
    {
	PtrMan<IOPar> outpar = par.subselect( sKeyOutput );
	if ( !outpar ) mErrRet( "Output parameters missing" );

	initWrite( outpar );
    }
    else
    {
	PtrMan<IOPar> inpar = par.subselect( sKeyInput );
	if ( !inpar ) mErrRet( "Input parameters missing" );

	initRead( inpar );
    }
}


void MadStream::initRead( IOPar* par )
{
    BufferString inptyp = par->find( sKey::Type );
    if ( inptyp == "None" || inptyp == "Madagascar" )
    {
	if ( inptyp=="None" )
	    istrm_ = &std::cin;
	else
	{
	    const char* filenm = par->find( sKey::FileName );
	    istrm_ = StreamProvider(filenm).makeIStream().istrm;
	}

	fillHeaderParsFromStream();
	if ( !headerpars_ ) mErrRet( "Error reading RSF header" );;

	BufferString insrc = headerpars_->find( sKeyIn );
	if ( insrc == "" || insrc == sKeyStdIn ) return;

	StreamData sd = StreamProvider(insrc).makeIStream();
	if ( !sd.usable() ) mErrRet( "Cannot read RSF data file" );;

	if ( istrm_ && istrm_ != &std::cin ) delete istrm_;

	istrm_ = sd.istrm;
	headerpars_->set( sKeyIn, sKeyStdIn );
	return;
    }

    Seis::GeomType gt = Seis::geomTypeOf( inptyp );
    if ( gt == Seis::Vol || gt == Seis::Line )
    {
	is2d_ = gt == Seis::Line;
	MultiID inpid;
	if ( !par->get(sKey::ID,inpid) ) mErrRet( "Input ID missing" );

	PtrMan<IOObj> ioobj = IOM().get( inpid );
	if ( !ioobj ) mErrRet( "Cannot find input data" );

	PtrMan<IOPar> subpar = par->subselect( sKey::Selection );
	Seis::SelData* seldata = Seis::SelData::get( *subpar );
	mDynamicCastGet(Seis::RangeSelData*,rangesel,seldata)
	if ( !rangesel ) mErrRet( "subselection??" );

	CubeSampling cs;
	if ( !is2d_ )
	{
	    SeisIOObjInfo info( ioobj );
	    info.getRanges( cs );
	    rangesel->cubeSampling().limitTo( cs );
	}

	seisrdr_ = new SeisTrcReader( ioobj );
	seisrdr_->setSelData( rangesel );
	seisrdr_->prepareWork();
	fillHeaderParsFromSeis();
    }
    else
    {
	errmsg_ = "Input Type: ";
	errmsg_ += inptyp;
	errmsg_ += " not supported";
	return;
    }
}
 

void MadStream::initWrite( IOPar* par )
{
    BufferString outptyp = par->find( sKey::Type );
    Seis::GeomType gt = Seis::geomTypeOf( outptyp );
    if ( gt == Seis::Vol || gt == Seis::Line )
    {
	is2d_ = gt == Seis::Line;
	istrm_ = &std::cin;
	istrm_->sync();
	MultiID outpid;
	if ( !par->get(sKey::ID,outpid) ) mErrRet( "Output data ID missing" );

	PtrMan<IOObj> ioobj = IOM().get( outpid );
	if ( !ioobj ) mErrRet( "Cannot find output object" );

	seiswrr_ = new SeisTrcWriter( ioobj );
	if ( !seiswrr_ ) mErrRet( "Cannot write to output object" );

	if ( is2d_ )
	{
	    PtrMan<IOPar> subpar = par->subselect( sKey::Selection );
	    Seis::SelData* seldata = Seis::SelData::get( *subpar );
	    const char* attrnm = par->find( sKey::Attribute );
	    if ( attrnm && *attrnm )
		seldata->lineKey().setAttrName( attrnm );

	    seiswrr_->setSelData( seldata );
	}

	fillHeaderParsFromStream();
    }
    else
    {
	errmsg_ = "Output Type: ";
	errmsg_ += outptyp;
	errmsg_ += " not supported";
	return;
    }
}

MadStream::~MadStream()
{
    if ( istrm_ && istrm_ != &std::cin ) delete istrm_;
    if ( ostrm_ )
    {
	ostrm_->flush();
	if ( ostrm_ != &std::cout && ostrm_ != &std::cerr ) delete ostrm_;
    }

    delete seisrdr_; delete seiswrr_;
    delete errmsg_;
}


BufferString MadStream::getPosFileName( bool forread ) const
{
    BufferString posfnm;
    if ( forread )
    {
	posfnm = pars_.find( sKeyPosFileName );
	if ( !posfnm.isEmpty() && File_exists(posfnm) )
	    return posfnm;
	else posfnm.setEmpty();
    }

    BufferString typ = 
	pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
		    		    sKey::Type) );
    if ( typ == sKeyMadagascar )
    {
	BufferString outfnm =
	    pars_.find( IOPar::compKey( forread ? sKeyInput : sKeyOutput,
		    			sKey::FileName) );
	FilePath fp( outfnm );
	fp.setExtension( "pos" );
	if ( !forread || File_exists(fp.fullPath()) )
	    posfnm = fp.fullPath();
    }
    else if ( !forread )
	posfnm = FilePath::getTempName( "pos" );

    return posfnm;
}


#define mWriteToPosFile( obj ) \
    StreamData sd = StreamProvider( posfnm ).makeOStream(); \
    if ( !sd.usable() ) mErrRet( "Cannot create Pos file" ); \
    if ( !obj.write(*sd.ostrm) ) \
    { sd.close(); mErrRet( "Cannot write to Pos file" ); } \
    sd.close(); \
    pars_.set( sKeyPosFileName, posfnm );


void MadStream::fillHeaderParsFromSeis()
{
    if ( !seisrdr_ ) mErrRet( "Cannot read input data" );

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
	if ( !ioobj ) mErrRet( "No nput object" );

	Seis2DLineSet lset( *ioobj );
	const Seis::SelData* seldata = seisrdr_->selData();
	if ( !seldata ) mErrRet( "Invalid data subselection" );

	const int lidx = lset.indexOf( seldata->lineKey() );
	if ( lidx < 0 ) mErrRet( "2D Line not found" );

	PosInfo::Line2DData geom;
	if ( !lset.getGeometry(lidx,geom) )
	    mErrRet( "Line geometry not available" );

	if ( !seldata->isAll() )
	{
	    geom.limitTo( seldata->crlRange() );
	    geom.zrg.limitTo( seldata->zRange() );
	}

	nrtrcs = geom.posns.size();
	zrg = geom.zrg;
	mWriteToPosFile( geom )
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
	    if ( !pinfo.cubedata ) mErrRet( "Incomplete Geometry Information" );

	    PosInfo::CubeData newcd( *pinfo.cubedata );
	    if ( rangesel && !rangesel->isAll() )
		newcd.limitTo( rangesel->cubeSampling().hrg );

	    needposfile = !newcd.isFullyRectAndReg();
	    if ( needposfile )
	    {
		nrtrcs = newcd.totalSize();
		mWriteToPosFile( newcd )
	    }
	    
	    newcd.erase();		// Because the pointers are not mine!
	}
	
	if ( !needposfile )
	{
	    StepInterval<int> inlrg =
		rangesel ? rangesel->cubeSampling().hrg.inlRange() : pinfo.inlrg;
	    headerpars_->set( "o3", inlrg.start );
	    headerpars_->set( "n3", inlrg.nrSteps()+1 );
	    headerpars_->set( "d3", inlrg.step );
	    if ( File_exists(posfnm) )
		StreamProvider(posfnm).remove(); // While overwriting rsf
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
}


void MadStream::fillHeaderParsFromStream()
{
    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    char linebuf[256], tag[4], *ptr;
    while ( istrm_ && !istrm_->bad() && !istrm_->eof() )
    {
	int idx = 0, nullcount = 0;
	while( nullcount<3 )
	{
	    char c;
	    istrm_->get( c );
	    if ( c == '\n' ) { linebuf[idx] = '\0'; break; }
	    linebuf[idx++] = c;
	    if ( c==sKeyRSFEndOfHeader[nullcount] )
		nullcount++;
	}

	if ( nullcount > 2 ) break;

	ptr = linebuf + 1;
	if ( *(ptr+2)!='=' )continue;

	char* valptr = ptr + 3;
	*(ptr+2) = '\0';
	if ( !strcmp(ptr,"in") )
	{
	    valptr++;
	    const int sz = strlen( valptr );
	    *( valptr + sz - 1 ) = '\0';
	}

	headerpars_->set( ptr, valptr );
    }
}


int MadStream::getNrSamples() const
{
    int nrsamps = 0;
    if ( headerpars_ ) headerpars_->get( "n1", nrsamps );

    return nrsamps;
}


bool MadStream::isOK() const
{
    if ( errMsg() ) return false;

    return true;
}


const char* MadStream::errMsg() const
{
    if ( errmsg_ == "" ) return 0;
    else 
	return errmsg_.buf();
}


bool MadStream::putHeader( std::ostream& strm )
{
    if ( !headerpars_ ) mErrBoolRet( "Header parameters not found" );

    for ( int idx=0; idx<headerpars_->size(); idx++ )
    {
	strm << "\t" << headerpars_->getKey(idx);
	strm << "=" << headerpars_->getValue(idx) << std::endl;
    }

    strm << "\t" << "data_format=\"native_float\"" << std::endl;
    strm << "\t" << "in=\"stdout\"" << std::endl;
    strm << "\t" << "in=\"stdin\"" << std::endl;

    strm << sKeyRSFEndOfHeader;
    return true;
}


bool MadStream::getNextTrace( float* arr )
{
    if ( istrm_ && !istrm_->bad() )
    {
	const int nrsamps = getNrSamples();
	istrm_->read( (char*)arr, nrsamps*sizeof(float) );
	return !istrm_->eof();
    }
    else if ( seisrdr_ )
    {
	SeisTrc trc;
	const int rv = seisrdr_->get( trc.info() );
	if ( rv < 0 ) mErrBoolRet( "Cannot read traces" )
	else if ( rv == 0 ) return false;

	if ( !seisrdr_->get(trc) ) mErrBoolRet( "Cannot read traces" );
	for ( int idx=0; idx<trc.size(); idx++ )
	    arr[idx] = trc.get( idx, 0 );

	return true;
    }

    mErrBoolRet( "No data source found" );
}


#define mReadFromPosFile( obj ) \
    BufferString posfnm = getPosFileName( true ); \
    if ( !posfnm.isEmpty() ) \
    { \
	haspos = true; \
	StreamProvider sp( posfnm ); \
	StreamData possd = sp.makeIStream(); \
	if ( !possd.usable() ) mErrBoolRet("Cannot Open Pos File"); \
       	if ( !obj.read(*possd.istrm) ) mErrBoolRet("Cannot Read Pos File"); \
	possd.close(); \
	FilePath fp( posfnm ); \
	if ( fp.pathOnly() == FilePath::getTempDir() ) \
	    sp.remove(); \
    }

bool MadStream::writeTraces()
{
    if ( !seiswrr_ )
	mErrBoolRet( "Cannot initialize writing" );

    if ( is2d_ )
	return write2DTraces();

    int inlstart=0, crlstart=1, inlstep=1, crlstep=1, nrinl=1, nrcrl, nrsamps;
    float zstart, zstep;
    bool haspos = false;
    PosInfo::CubeData cubedata;
    mReadFromPosFile( cubedata );
    headerpars_->get( "o1", zstart );
    headerpars_->get( "o2", crlstart );
    headerpars_->get( "o3", inlstart );
    headerpars_->get( "n1", nrsamps );
    headerpars_->get( "n2", nrcrl );
    headerpars_->get( "n3", nrinl );
    headerpars_->get( "d1", zstep );
    headerpars_->get( "d2", crlstep );
    headerpars_->get( "d3", inlstep );
    SamplingData<float> sd( zstart, zstep );
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
		std::cin.read( (char*)buf, nrsamps*sizeof(float) );
		SeisTrc trc( nrsamps );
		trc.info().sampling = sd;
		trc.info().binid = BinID( inl, crl );

		for ( int isamp=0; isamp<nrsamps; isamp++ )
		    trc.set( isamp, buf[isamp], 0 );

		if ( !seiswrr_->put (trc) )
		{ delete [] buf; mErrBoolRet("Cannot write trace"); }
	    }
	}
    }

    delete [] buf;
    return true;
}


bool MadStream::write2DTraces()
{
    PosInfo::Line2DData geom;
    bool haspos = false;
    mReadFromPosFile ( geom );
    if ( !haspos ) mErrBoolRet( "Position data not available" );;

    int trcstart=1, trcstep=1, nrtrcs=0, nrsamps=0;
    float zstart, zstep;

    headerpars_->get( "o1", zstart );
    headerpars_->get( "o2", trcstart );
    headerpars_->get( "n1", nrsamps );
    headerpars_->get( "n2", nrtrcs );
    headerpars_->get( "d1", zstep );
    headerpars_->get( "d2", trcstep );
    SamplingData<float> sd( zstart, zstep );

    if ( nrtrcs != geom.posns.size() )
	mErrBoolRet( "Line geometry doesn't match with data" );

    float* buf = new float[nrsamps];
    for ( int idx=0; idx<geom.posns.size(); idx++ )
    {
	const int trcnr = geom.posns[idx].nr_;
	std::cin.read( (char*)buf, nrsamps*sizeof(float) );
	SeisTrc trc( nrsamps );
	trc.info().sampling = sd;
	trc.info().coord = geom.posns[idx].coord_;
	trc.info().binid.crl = trcnr;
	trc.info().nr = trcnr;

	for ( int isamp=0; isamp<nrsamps; isamp++ )
	    trc.set( isamp, buf[isamp], 0 );

	if ( !seiswrr_->put (trc) )
	{ delete [] buf; mErrBoolRet("Error writing traces"); }
    }

    delete [] buf;
    return true;
}
#undef mErrRet
