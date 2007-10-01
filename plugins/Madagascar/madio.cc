/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : June 2007
-*/

static const char* rcsID = "$Id: madio.cc,v 1.1 2007-10-01 15:20:08 cvsbert Exp $";

#include "madio.h"
#include "keystrs.h"
#include "filegen.h"
#include "filepath.h"
#include "strmprov.h"
#include "envvars.h"
#include "iopar.h"
#include "seisread.h"
#include "seistrcsel.h"

const char* ODMad::FileSpec::sKeyDataPath = "Data Path";


static BufferString getDataFnm( const char* fnm, const char* dir, bool forread )
{
    FilePath fp( fnm ); fp.setPath( dir );
    const BufferString ret( fp.fullPath() );
    if ( !forread )
    {
	if ( File_isWritable(dir) )
	    return ret;
    }
    else if ( !File_isEmpty(ret) )
	return ret;

    return BufferString();
}


static BufferString getDataPath( const char* dp, const char* fnm, bool forread )
{
    BufferString ret( dp );
    if ( dp ) return ret;

    ret = getDataFnm( fnm, ODMad::FileSpec::defDataPath(), forread );
    ret = FilePath( ret ).pathOnly();
    return ret;
}


ODMad::FileSpec::FileSpec( bool fr, const char* fnm, const char* dp )
    : forread_(fr)
{
    FilePath fp( fname_ );
    if ( !fp.isAbsolute() )
	fp.setPath( getDataPath(dp,fname_,fr) );
}


const char* ODMad::FileSpec::defDataPath()
{
    static BufferString* ret = 0;
    if ( ret ) return ret->buf();

    ret = new BufferString( GetEnvVar("DATAPATH") );
    if ( File_isDirectory(ret->buf()) )
	return ret->buf();

    //TODO support ~/.datapath
    // ~/.datapath may contain: [machine_name] datapath=/path/to_file/

    return ret->buf();
}


StreamData ODMad::FileSpec::open() const
{
    return forread_ ? StreamProvider(fname_).makeIStream()
		   : StreamProvider(fname_).makeOStream();
}


ODMad::MadSeisInp::MadSeisInp()
    : fspec_(true)
    , geom_(Seis::Vol)
    , seldata_(*new SeisSelData)
{
}


ODMad::MadSeisInp::~MadSeisInp()
{
    delete &seldata_;
}


bool ODMad::MadSeisInp::usePar( const IOPar& iop )
{
    geom_ = Seis::geomTypeOf( iop.find(sKey::Geometry) );
    seldata_.usePar( iop );

    BufferString fnm( fspec_.fileName() );
    iop.get( sKey::FileName, fnm );
    if ( fnm.isEmpty() )
    {
	errmsg_ = "No filename specified";
	return false;
    }

    BufferString dp( FileSpec::defDataPath() );
    iop.get( FileSpec::sKeyDataPath, dp );
    fspec_ = FileSpec( fnm, dp );
    return true;
}


bool ODMad::MadSeisInp::get( SeisTrc& trc ) const
{
    if ( !sd_.usable() )
    {
	const_cast<StreamData&>(sd_) = fspec_.open();
	if ( !sd_.usable() )
	{
	    errmsg_ = "Cannot open specified file name";
	    return false;
	}
	// TODO start reading Madagascar file with subselection
    }

    errmsg_ = "TODO: Not implemented: read next trace";
    return false;
}


bool ODMad::ODSeisInp::usePar( const IOPar& iop )
{
    if ( rdr_ ) delete rdr_;
    rdr_ = new SeisTrcReader;
    rdr_->usePar( iop );
    if ( rdr_->errMsg() && *rdr_->errMsg() )
    {
	errmsg_ = rdr_->errMsg();
	return false;
    }

    return true;
}


bool ODMad::ODSeisInp::get( SeisTrc& trc ) const
{
    if ( !rdr_ ) return false;

    if ( !rdr_->get(trc) )
    {
	errmsg_ = rdr_->errMsg();
	return false;
    }

    return true;
}
