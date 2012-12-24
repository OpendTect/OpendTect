/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Apr 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seiscube2linedata.h"
#include "keystrs.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seis2dline.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "survinfo.h"
#include "ioobj.h"


class Cube2LineDataLineKeyProvider : public LineKeyProvider
{
public:
Cube2LineDataLineKeyProvider( SeisCube2LineDataExtracter& lde ) : lde_(lde) {}

LineKey lineKey() const
{
    return LineKey( lde_.usedlinenames_.isEmpty() ?
		    sKey::EmptyString().str() : lde_.usedlinenames_[0]->str(),
		    lde_.attrnm_ );
}

    SeisCube2LineDataExtracter&	lde_;

};


SeisCube2LineDataExtracter::SeisCube2LineDataExtracter(
			const IOObj& cubein, const IOObj& lsout,
			const char* attrnm, const BufferStringSet* lnms )
    : Executor("Extract 3D data into 2D lines")
    , attrnm_(attrnm)
    , tbuf_(*new SeisTrcBuf(true))
    , rdr_(*new SeisTrcReader(&cubein))
    , ls_(*new Seis2DLineSet(lsout))
    , wrr_(*new SeisTrcWriter(&lsout))
    , nrdone_(0)
    , totalnr_( 0 )
    , c2ldlkp_(0)
{
    if ( lnms ) lnms_ = *lnms;
}


SeisCube2LineDataExtracter::~SeisCube2LineDataExtracter()
{
    closeDown();
    delete &tbuf_; delete &wrr_; delete &rdr_; delete &ls_;
}


void SeisCube2LineDataExtracter::closeDown()
{
    tbuf_.deepErase();
    rdr_.close(); wrr_.close();
    deepErase( fetchers_ );
    usedlinenames_.erase();
    delete c2ldlkp_; c2ldlkp_ = 0;
    wrr_.setLineKeyProvider(0);
}


int SeisCube2LineDataExtracter::nextStep()
{
    if ( fetchers_.isEmpty() )
    {
	if ( !rdr_.prepareWork() )
	    msg_ = rdr_.errMsg();
	else if ( ls_.nrLines() < 1 )
	    msg_ = "Empty or invalid Line Set";
	else
	{
	    c2ldlkp_ = new Cube2LineDataLineKeyProvider( *this );
	    wrr_.setLineKeyProvider( c2ldlkp_ );
	    msg_ = "Handling traces";
	}

	return getFetchers()
	    ? MoreToDo()
	    : ErrorOccurred();
    }

    int res = fetchers_[0]->doStep();
    if ( res != 1 )
    {
	if ( res > 1 )
	    return res;
	else if ( res == 0 )
	{
	    delete fetchers_.removeSingle( 0 );
	    usedlinenames_.removeSingle( 0 );
	    return fetchers_.isEmpty() ? Finished() : MoreToDo();
	}
	else
	{
	    msg_ = fetchers_[0]->message();
	    return ErrorOccurred();
	}
    }

    res = handleTrace();
    msg_ = fetchers_[0]->message();
    return res;
}


bool SeisCube2LineDataExtracter::getFetchers()
{
    totalnr_ = 0;
    usedlinenames_.erase();
    
    for ( int lidx=0; lidx<ls_.nrLines(); lidx++ )
    {
	const BufferString lnm = ls_.lineName( lidx );
	if ( usedlinenames_.isPresent( lnm ) )
	    continue;
	if ( !lnms_.isEmpty() && !lnms_.isPresent(lnm) )
	    continue;

	int inplidx = -1;
	const LineKey deflk( lnm );
	if ( ls_.lineKey(lidx) == deflk )
	    inplidx = lidx;
	else
	{
	    for ( int iln=0; iln<ls_.nrLines(); iln++ )
	    {
		if ( ls_.lineKey(iln) == deflk )
		    { inplidx = iln; break; }
	    }
	}
	if ( inplidx < 0 )
	    inplidx = lidx;

	Executor* fetcher = ls_.lineFetcher( inplidx, tbuf_, 1 );
	if ( fetcher )
	{
	    totalnr_ += fetcher->totalNr();
	    fetchers_ += fetcher;
	    usedlinenames_.add( lnm );
	}
    }

    return !fetchers_.isEmpty();
}


int SeisCube2LineDataExtracter::handleTrace()
{
    SeisTrc* trc = tbuf_.remove( 0 );
    SeisTrcInfo ti( trc->info() );
    delete trc;

    if ( !rdr_.seisTranslator()->goTo( SI().transform(ti.coord) ) )
	return MoreToDo();

    SeisTrc trc3d;
    if ( !rdr_.get(trc3d) )
	return MoreToDo();

    ti.sampling = trc3d.info().sampling;
    trc3d.info() = ti;
    if ( !wrr_.put(trc3d) )
	{ msg_ = wrr_.errMsg(); return ErrorOccurred(); }

    nrdone_++;
    return MoreToDo();
}
