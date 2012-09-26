/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : April 2001
 * FUNCTION : CBVS pack writer
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "cbvswritemgr.h"
#include "cbvswriter.h"
#include "strmprov.h"
#include "filepath.h"
#include "survinfo.h"
#include "envvars.h"


BufferString CBVSIOMgr::baseFileName( const char* fnm )
{
    BufferString ret( fnm );
    char* caretptr = strchr( ret.buf(), '^' );
    if ( !caretptr ) return ret;

    char* dotptr = strchr( caretptr, '.' );
    BufferString ext( dotptr );
    *caretptr = '\0';
    ret += ext;
    return ret;
}


BufferString CBVSIOMgr::getFileName( const char* basefname, int curnr )
{
    if ( curnr == 0 ) return basefname;

    FilePath fp( basefname );
    BufferString fname = fp.fileName();

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

    fp.setFileName( fname );
    return fp.fullPath();
}


int CBVSIOMgr::getFileNr( const char* fnm )
{
    if ( !fnm || !*fnm ) return 0;

    const char* caretptr = strrchr( fnm, '^' );
    if ( !caretptr ) return 0;

    BufferString nrstr( caretptr + 1 );
    char* dotptr = strchr( nrstr.buf(), '.' );
    if ( dotptr ) *dotptr = '\0';

    return toInt( nrstr.buf() );
}


CBVSIOMgr::~CBVSIOMgr()
{
    deepErase( fnames_ );
}


void VBrickSpec::setStd( bool yn )
{
    maxnrslabs = 30;
    if ( !yn )
    {
	nrsamplesperslab = -1;
	return;
    }

    if ( GetEnvVar("DTECT_CBVS_SAMPLES_PER_SLAB") )
    {
	nrsamplesperslab = toInt(GetEnvVar("DTECT_CBVS_SAMPLES_PER_SLAB"));
	maxnrslabs = 200;
    }
    else
    {
	const int nrsamps = SI().zRange(false).nrSteps() + 1;
	nrsamplesperslab = nrsamps / maxnrslabs;
	if ( nrsamps % maxnrslabs )
	    nrsamplesperslab++;
	if ( nrsamplesperslab < 10 )
	    nrsamplesperslab = 10;
    }
}


CBVSWriteMgr::CBVSWriteMgr( const char* fnm, const CBVSInfo& i,
			    const PosAuxInfo* pai, VBrickSpec* bs,
			    bool sf, CBVSIO::CoordPol cp )
	: CBVSIOMgr(fnm)
	, info_(i)
    	, single_file(sf)
    	, coordpol_(cp)
{
    const int totsamps = info_.nrsamples_;
    if ( totsamps < 1 ) return;

    VBrickSpec spec; if ( bs ) spec = *bs;
    if ( single_file
      || spec.nrsamplesperslab < 0
      || spec.nrsamplesperslab >= totsamps
      || spec.maxnrslabs < 2 )
    {
	std::ostream* strm = mkStrm();
	if ( !strm ) return;
	CBVSWriter* wr = new CBVSWriter( strm, info_, pai, coordpol_ );
	
	writers_ += wr;
	endsamps_ += totsamps-1;
	return;
    }

    int nrnormsamps = totsamps - 1;
    int nrnormwrs = nrnormsamps / spec.nrsamplesperslab;
    int extrasamps = nrnormsamps % spec.nrsamplesperslab;
    if ( extrasamps ) nrnormwrs++;
    if ( nrnormwrs > spec.maxnrslabs )
	nrnormwrs = spec.maxnrslabs;
    spec.nrsamplesperslab = nrnormsamps / nrnormwrs;
    if ( spec.nrsamplesperslab < 1 )
	spec.nrsamplesperslab = 1;
    extrasamps = nrnormsamps - nrnormwrs * spec.nrsamplesperslab;

    CBVSInfo inf( info_ );
    for ( int endsamp, startsamp=0; startsamp<totsamps; startsamp=endsamp+1 )
    {
	endsamp = startsamp == 0 ? 0 : startsamp + spec.nrsamplesperslab - 1;

	if ( startsamp && extrasamps )
	    { endsamp++; extrasamps--; }
	if ( endsamp >= totsamps ) endsamp = totsamps-1;

	inf.sd_.start = info_.sd_.start + startsamp * info_.sd_.step;
	inf.sd_.start = info_.sd_.start + startsamp * info_.sd_.step;
	inf.nrsamples_ = endsamp - startsamp + 1;

	std::ostream* strm = mkStrm();
	if ( !strm )
	    { cleanup(); return; }
	CBVSWriter* wr = new CBVSWriter( strm, inf, pai, coordpol_ );
	writers_ += wr;

	if ( writers_.size() == 1 )
	    inf.auxinfosel_.setAll( false );

	endsamps_ += endsamp;
    }
}


std::ostream* CBVSWriteMgr::mkStrm()
{
    BufferString* fname = new BufferString( single_file ? basefname_
	    				  : getFileName(curnr_) );
    curnr_++;
    StreamData sd = StreamProvider((const char*)*fname).makeOStream();

    if ( sd.usable() && sd.ostrm->good() )
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


void CBVSWriteMgr::ensureConsistent()
{
    for ( int idx=0; idx<writers_.size(); idx++ )
	writers_[idx]->ciaoForNow();
}


const char* CBVSWriteMgr::errMsg_() const
{
    for ( int idx=0; idx<writers_.size(); idx++ )
    {
	const char* s = writers_[idx]->errMsg();
	if ( s ) return s;
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
    if ( writers_.isEmpty() ) return false;

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
	    if ( single_file )
	    {
		ret = -1;
		writer->setErrMsg( "Cannot write more data to seismic file" );
	    }
	    else
	    {
		std::ostream* strm = mkStrm();
		if ( !strm ) return false;

		if ( info_.geom_.fullyrectandreg )
		    info_.geom_.start.inl = writer->survGeom().stop.inl
					 + info_.geom_.step.inl;

		writer->forceLineStep( writer->survGeom().step );
		CBVSWriter* newwriter = new CBVSWriter( strm, *writer, info_ );
		writers_ += newwriter;
		writers_ -= writer;
		delete writer;
		writer = newwriter;

		ret = writer->put( data );
	    }
	}
    }

    return ret == -1 ? false : true;
}
