/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
-*/

static const char* rcsID = "$Id: seismulticubeps.cc,v 1.1 2008-08-28 12:14:54 cvsbert Exp $";

#include "seismulticubeps.h"
#include "seispsioprov.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisbuf.h"

class MultiCubeSeisPSIOProvider : public SeisPSIOProvider
{
public:
			MultiCubeSeisPSIOProvider() : SeisPSIOProvider("CBVS") {}
    SeisPS3DReader*	make3DReader( const char* fnm, int ) const
			{ return new MultiCubeSeisPSReader(fnm); }
    SeisPSWriter*	make3DWriter( const char* dirnm ) const
			{ return 0; }
    SeisPS2DReader*	make2DReader( const char* dirnm, const char* lnm ) const
			{ return 0; }
    SeisPSWriter*	make2DWriter( const char* dirnm, const char* lnm ) const
			{ return 0; }
    bool		getLineNames(const char*,BufferStringSet&) const
			{ return false; }
    static int		factid;
};

// This adds the Multicube type pre-stack seismics data storage to the factory
int MultiCubeSeisPSIOProvider::factid = SPSIOPF().add(
				new MultiCubeSeisPSIOProvider );


MultiCubeSeisPSReader( const char* fnm )
{
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
    {
	errmsg_ = "Data store definition file '";
	errmsg_ += fnm;
	errmsg_ += "' not readable";
	return;
    }

    BufferString bs;
    while ( StrmOper::readLine( *sd.istrm, &bs ) )
    {
	if ( bs.isEmpty() || *bs.buf() == '#' || !IOObj::isKey(bs) )
	    continue;

	PtrMan<IOObj> ioobj = IOM().get( MultiID(bs.buf()) );
	if ( !ioobj )
	    continue;

	SeisTrcReader* rdr = new SeisTrcReader( ioobj );
	if ( rdr->errMsg() && *rdr->errMsg() )
	    { errmsg_ = rdr->errMsg(); delete rdr; continue; }

	rdrs_ += rdr;
    }

    if ( rdrs_.isEmpty() )
	{ if ( errmsg_.isEmpty() ) errmsg_ = "No valid cubes found"; }
}
