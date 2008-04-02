/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
-*/

static const char* rcsID = "$Id: madstream.cc,v 1.2 2008-04-02 11:44:37 cvsraman Exp $";

#include "madstream.h"
#include "cubesampling.h"
#include "strmprov.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seiswrite.h"


using namespace ODMad;

static const char* sKeyRSFEndOfHeader = "\014\014\004";
static const char* sKeyMadagascar = "Madagascar";
static const char* sKeyInput = "Input";
static const char* sKeyOutput = "Output";
static const char* sKeyProc = "Proc";
static const char* sKeyWrite = "Write";
static const char* sKeyIn = "in";
static const char* sKeyStdIn = "\"stdin\"";

#undef mErrRet
#define mErrRet(s) { errmsg_ = s; return; }

MadStream::MadStream( const IOPar& par )
    : istrm_(0)
    , ostrm_(0)
    , seisrdr_(0)
    , seiswrr_(0)
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

	fillHeaderPars();
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
    if ( gt == Seis::Vol )
    {
	MultiID inpid;
	if ( !par->get(sKey::ID,inpid) ) mErrRet( "Input ID missing" );

	PtrMan<IOObj> ioobj = IOM().get( inpid );
	if ( !ioobj ) mErrRet( "Cannot find input data" );

	CubeSampling cs;
	SeisIOObjInfo seisinfo( ioobj );
	seisinfo.getRanges( cs );

	PtrMan<IOPar> subpar = par->subselect( sKey::Selection );
	Seis::SelData* seldata = Seis::SelData::get( *subpar );
	mDynamicCastGet(Seis::RangeSelData*,rangesel,seldata)
	if ( rangesel ) cs.limitTo( rangesel->cubeSampling() );

	seisrdr_ = new SeisTrcReader( ioobj );
	seisrdr_->setSelData( seldata );
	seisrdr_->prepareWork();
	fillHeaderPars( cs );
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
    if ( gt == Seis::Vol )
    {
	istrm_ = &std::cin;
	MultiID outpid;
	if ( !par->get(sKey::ID,outpid) ) mErrRet( "Output data ID missing" );

	PtrMan<IOObj> ioobj = IOM().get( outpid );
	if ( !ioobj ) mErrRet( "Cannot find output object" );

	seiswrr_ = new SeisTrcWriter( ioobj );
	if ( !seiswrr_ ) mErrRet( "Cannot write to output object" );

	fillHeaderPars();
    }
    else
    {
	errmsg_ = "Output Type: ";
	errmsg_ += outptyp;
	errmsg_ += " not supported";
	return;
    }
}
#undef mErrRet

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


void MadStream::fillHeaderPars( const CubeSampling& cs )
{
    if ( headerpars_ ) delete headerpars_; headerpars_ = 0;

    headerpars_ = new IOPar;
    headerpars_->set( "o1", cs.zrg.start );
    headerpars_->set( "o2", cs.hrg.start.crl );
    headerpars_->set( "o3", cs.hrg.start.inl );

    headerpars_->set( "n1", cs.nrZ() );
    headerpars_->set( "n2", cs.nrCrl() );
    headerpars_->set( "n3", cs.nrInl() );

    headerpars_->set( "d1", cs.zrg.step );
    headerpars_->set( "d2", cs.hrg.step.crl );
    headerpars_->set( "d3", cs.hrg.step.inl );
}


void MadStream::fillHeaderPars()
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
    if ( !headerpars_ ) return false;

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
	if ( seisrdr_->get(trc.info())!=1 || !seisrdr_->get(trc) ) return false;
	for ( int idx=0; idx<trc.size(); idx++ )
	    arr[idx] = trc.get( idx, 0 );

	return true;
    }

    return false;
}


bool MadStream::writeTraces()
{
    int inlstart, crlstart, inlstep=1, crlstep=1, nrinl, nrcrl, nrsamps;
    float zstart, zstep;
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
    float* buf = new float[nrsamps];
    for ( int inlidx=0; inlidx<nrinl; inlidx++ )
    {
	const int inl = inlstart + inlidx * inlstep;
	for ( int crlidx=0; crlidx<nrcrl; crlidx++ )
	{
	    const int crl = crlstart + crlidx * crlstep;
	    std::cin.read( (char*)buf, nrsamps*sizeof(float) );
	    SeisTrc trc( nrsamps );
	    trc.info().sampling = sd;
	    trc.info().binid = BinID( inl, crl );

	    for ( int isamp=0; isamp<nrsamps; isamp++ )
		trc.set( isamp, buf[isamp], 0 );

	    if ( !seiswrr_->put (trc) )
		return false;

	}
    }

    return true;
}
