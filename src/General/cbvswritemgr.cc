/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS pack writer
-*/

static const char* rcsID = "$Id: cbvswritemgr.cc,v 1.15 2002-11-27 17:05:10 bert Exp $";

#include "cbvswritemgr.h"
#include "cbvswriter.h"
#include "strmprov.h"
#include "filegen.h"

BufferString CBVSIOMgr::getFileName( const char* basefname, int curnr )
{
    if ( curnr == 0 ) return basefname;

    BufferString fname( basefname );
    fname = File_getFileName( fname );
    BufferString pathname( basefname );
    pathname = File_getPathOnly( pathname );

    char* ptr = strrchr( fname.buf(), '.' );
    BufferString ext;
    if ( ptr )
	{ ext = ptr; *ptr = '\0'; }

    if ( curnr < 0 )
	fname += "^aux";
    else
    {
	fname += curnr < 10 ? "^0" : "^";
	fname += curnr;
    }

    if ( ptr ) fname += ext;

    fname = File_getFullPath( pathname, fname );
    return fname;
}


CBVSIOMgr::~CBVSIOMgr()
{
    deepErase( fnames_ );
}


void VBrickSpec::setStd( bool yn )
{
    nrsamplesperslab = yn ? 75 : -1;
    maxnrslabs = 20;
    if ( yn && getenv("dGB_CBVS_SAMPLES_PER_SLAB") )
    {
	nrsamplesperslab = atoi(getenv("dGB_CBVS_SAMPLES_PER_SLAB"));
	maxnrslabs = 256;
    }
}


CBVSWriteMgr::CBVSWriteMgr( const char* fnm, const CBVSInfo& i,
			    const PosAuxInfo* pai, VBrickSpec* bs )
	: CBVSIOMgr(fnm)
	, info_(i)
{
    const BasicComponentInfo& cinf0 = *info_.compinfo[0];
    const int totsamps = cinf0.nrsamples;
    if ( totsamps < 1 ) return;

    VBrickSpec spec; if ( bs ) spec = *bs;
    if ( spec.nrsamplesperslab < 0
      || spec.nrsamplesperslab >= totsamps
      || spec.maxnrslabs < 2 )
    {
	ostream* strm = mkStrm();
	if ( !strm ) return;
	CBVSWriter* wr = new CBVSWriter( strm, info_, pai );
	wr->setByteThreshold( 1900000000 );
	writers_ += wr;
	endsamps_ += totsamps-1;
	return;
    }

    int nrwrs = totsamps / spec.nrsamplesperslab;
    int extrasamps = totsamps % spec.nrsamplesperslab;
    if ( extrasamps ) nrwrs++;
    if ( nrwrs > spec.maxnrslabs )
	nrwrs = spec.maxnrslabs;
    spec.nrsamplesperslab = totsamps / nrwrs;
    extrasamps = totsamps - nrwrs * spec.nrsamplesperslab;

    CBVSInfo inf( info_ );
    for ( int endsamp = -1, startsamp=0;
	    startsamp<totsamps;
	    startsamp=endsamp+1 )
    {
	endsamp = startsamp + spec.nrsamplesperslab - 1;
	if ( extrasamps )
	    { endsamp++; extrasamps--; }
	if ( endsamp >= totsamps ) endsamp = totsamps-1;

	for ( int idx=0; idx<inf.compinfo.size(); idx++ )
	{
	    inf.compinfo[idx]->sd.start = cinf0.sd.start
					+ startsamp * cinf0.sd.step;
	    inf.compinfo[idx]->nrsamples = endsamp - startsamp + 1;
	}

	ostream* strm = mkStrm();
	if ( !strm )
	    { cleanup(); return; }
	CBVSWriter* wr = new CBVSWriter( strm, inf, pai );
	writers_ += wr;

	if ( writers_.size() == 1 )
	    inf.auxinfosel.setAll( false );

	endsamps_ += endsamp;
    }
}


ostream* CBVSWriteMgr::mkStrm()
{
    BufferString* fname = new BufferString( getFileName(curnr_) );
    curnr_++;
    StreamData sd = StreamProvider((const char*)*fname).makeOStream();

    if ( sd.usable() )
	fnames_ += fname;
    else
    {
	errmsg_ = "Cannot open '"; errmsg_ += *fname; errmsg_ += "' for write";
	sd.close();
	delete fname;
    }

    return sd.ostrm;
}


CBVSWriteMgr::~CBVSWriteMgr()
{
    close();
    cleanup();
}


void CBVSWriteMgr::cleanup()
{
    deepErase( writers_ );
}


void CBVSWriteMgr::close()
{
    for ( int idx=0; idx<writers_.size(); idx++ )
	writers_[idx]->close();
}


const char* CBVSWriteMgr::errMsg_() const
{
    for ( int idx=0; idx<writers_.size(); idx++ )
    {
	const char* s = writers_[idx]->errMsg();
	if ( s && *s ) return s;
    }
    return 0;
}


int CBVSWriteMgr::nrComponents() const
{
    return writers_.size() ? writers_[0]->nrComponents() : 0;
}


const BinID& CBVSWriteMgr::binID() const
{
    static BinID binid00(0,0);
    return writers_.size() ? writers_[0]->binID() : binid00;
}


unsigned long CBVSWriteMgr::bytesPerFile() const
{
    return writers_.size() ? writers_[0]->byteThreshold() : 0;
}


void CBVSWriteMgr::setBytesPerFile( unsigned long b )
{
    if ( writers_.size() == 1 )
	writers_[0]->setByteThreshold( b );
}


bool CBVSWriteMgr::put( void** data )
{
    if ( !writers_.size() ) return false;

    int ret = 0;
    if ( writers_.size() > 1 )
    {
	for ( int idx=0; idx<writers_.size(); idx++ )
	{
	    ret = writers_[idx]->put( data, idx ? endsamps_[idx-1]+1 : 0 );
	    if ( ret < 0 )
		break;
	}
	if ( ret > 0 ) ret = 0;
    }
    else
    {
	CBVSWriter* writer = writers_[0];
	ret = writer->put( data );
	if ( ret == 1 )
	{
	    ostream* strm = mkStrm();
	    if ( !strm ) return false;

	    if ( info_.geom.fullyrectandreg )
		info_.geom.start.inl = writer->survGeom().stop.inl
				     + info_.geom.step.inl;

	    writer->forceXlineStep( writer->survGeom().step.crl );
	    CBVSWriter* newwriter = new CBVSWriter( strm, *writer, info_ );
	    writers_ += newwriter;
	    writers_ -= writer;
	    delete writer;
	    writer = newwriter;

	    ret = writer->put( data );
	}
    }

    return ret == -1 ? false : true;
}
