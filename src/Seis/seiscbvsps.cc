/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seiscbvsps.cc,v 1.1 2004-12-30 11:28:25 bert Exp $";

#include "seiscbvsps.h"
#include "seiscbvs.h"
#include "seisbuf.h"
#include "filepath.h"
#include "filegen.h"
#include "survinfo.h"
#include "iopar.h"


SeisCBVSPSReader::SeisCBVSPSReader( const char* dirnm )
    	: pars_(*new IOPar)
	, dirnm_(dirnm)
{
    FilePath fp( dirnm_ ); fp.add( "index.txt" );
    BufferString fnm( fp.fullPath() );
    if ( !File_exists(fnm) )
    {
	if ( !File_isDirectory(dirnm) )
	{
	    errmsg_ = "Directory '"; errmsg_ += dirnm;
	    errmsg_ += "' does not exist";
	}
	else
	{
	    errmsg_ = "No 'index.txt' in '"; errmsg_ += dirnm;
	    errmsg_ += "'";
	}
	return;
    }

    if ( !const_cast<IOPar&>(pars_).read(fnm) )
    {
	errmsg_ = "Corrupted 'index.txt' in '"; errmsg_ += dirnm;
	errmsg_ += "'";
	return;
    }
    else if ( pars_.size() < 1 )
	{ errmsg_ = "Empty data store"; return; }
}


SeisCBVSPSReader::~SeisCBVSPSReader()
{
    delete &pars_;
}


bool SeisCBVSPSReader::getGather( const BinID& bid, SeisTrcBuf& gath ) const
{
    BufferString key = bid.inl;
    const char* res = pars_.find( key );
    if ( !res )
	{ errmsg_ = "Inline not present"; return false; }

    FilePath fp( dirnm_ );
    fp.add( res );

    errmsg_ = "";
    CBVSSeisTrcTranslator* tr = CBVSSeisTrcTranslator::make( fp.fullPath(),
	    					false, false, &errmsg_ );
    if ( !tr )
	return false;

    if ( !tr->goTo( BinID(bid.crl,0) ) )
	{ errmsg_ = "Crossline not present"; return false; }

    const Coord coord = SI().transform( bid );
    while ( true )
    {
	SeisTrc* trc = new SeisTrc;
	if ( !tr->read(*trc) )
	{
	    delete trc;
	    errmsg_ = tr->errMsg();
	    return errmsg_ == "";
	}
	else if ( trc->info().binid.inl != bid.crl )
	    { delete trc; return true; }

	trc->info().nr = trc->info().binid.crl;
	trc->info().binid = bid; trc->info().coord = coord;
	gath.add( trc );
    }

    // Not reached
    return true;
}
