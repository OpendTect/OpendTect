/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2008
-*/
static const char* rcsID = "$Id: segyresorter.cc,v 1.3 2011-03-25 15:02:34 cvsbert Exp $";

#include "segyresorter.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "seisposkey.h"
#include "strmprov.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "ioman.h"

SEGY::ReSorter::Setup::Setup( Seis::GeomType gt, const MultiID& ky,
			      const char* fnm )
    : geom_(gt)
    , inpkey_(ky)
    , outfnm_(fnm)
    , nridxsperfile_(-1)
{
}


SEGY::ReSorter::ReSorter( const SEGY::ReSorter::Setup& su, const char* lnm )
    : Executor("Re-sorting")
    , cdp_(*new PosInfo::CubeDataPos)
    , setup_(su)
    , msg_("Handling traces")
    , nrdone_(0)
    , drdr_(0)
    , filefirstidx_(-1)
{
    IOObj* ioobj = IOM().get( setup_.inpkey_ );
    if ( !ioobj )
	msg_ = "Cannot find provided input in data manager";
    else
    {
	switch ( setup_.geom_ )
	{
	    case Seis::Vol:
	    {
		Translator* tr = ioobj->getTranslator();
		mDynamicCastGet(SEGYDirectSeisTrcTranslator*,str,tr)
		if ( str )
		    drdr_ = str;
		else
		{
		    msg_ = "Input must be scanned SEG-Y cube";
		    delete tr;
		}
	    }
	    break;
	    case Seis::VolPS:
	    {
		drdr_ = new SEGYDirect3DPSReader( ioobj->fullUserExpr(true) );
	    }
	    break;
	    case Seis::LinePS:
		drdr_ = new SEGYDirect2DPSReader( ioobj->fullUserExpr(true),
		       				  lnm );
	    break;
	    case Seis::Line:
		msg_ = "2D seismics not supported";
	    break;
	}
    }
    delete ioobj;

    if ( drdr_ && drdr_->errMsg() && *drdr_->errMsg() )
	{ msg_ = drdr_->errMsg(); delete drdr_; drdr_ = 0; }

    const SEGY::DirectDef& dd = *drdr_->getDef();
    if ( dd.isEmpty() )
	{ msg_ = "Empty innput scan"; delete drdr_; drdr_ = 0; }

    if ( !drdr_ )
	return;

    if ( Seis::is2D(setup_.geom_) )
	totnr_ = dd.lineData().positions().size();
    else
	totnr_ = dd.cubeData().totalSize();
}


SEGY::ReSorter::~ReSorter()
{
    wrapUp();
    delete &cdp_;
}


int SEGY::ReSorter::wrapUp()
{
    delete drdr_; drdr_ = 0;
    cdp_.toPreStart();
    return Finished();
}


int SEGY::ReSorter::nextStep()
{
    if ( !drdr_ )
	return ErrorOccurred();

    BinID bid;
    if ( !getCurPos(bid) )
	return wrapUp();

    if ( !createOutput(bid) )
	return ErrorOccurred();

    return toNext() ? MoreToDo() : Finished();
}


bool SEGY::ReSorter::toNext()
{
    nrdone_++;
    const SEGY::DirectDef& dd = *drdr_->getDef();
    if ( Seis::is2D(setup_.geom_) )
	return nrdone_ >= dd.lineData().positions().size();
    else
	return dd.cubeData().toNext(cdp_);
}


bool SEGY::ReSorter::getCurPos( BinID& bid )
{
    const SEGY::DirectDef& dd = *drdr_->getDef();

    if ( Seis::is2D(setup_.geom_) )
    {
	const TypeSet<PosInfo::Line2DPos>& posns = dd.lineData().positions();
	if ( nrdone_ >= posns.size() )
	    return false;
	bid.inl = 1;
	bid.crl = posns[(int)nrdone_].nr_;
    }
    else
    {
	const PosInfo::CubeData& cd = dd.cubeData();
	if ( !cd.isValid(cdp_) )
	    return false;
	bid = cd.binID( cdp_ );
    }

    return true;
}


bool SEGY::ReSorter::createOutput( const BinID& bid )
{
    const bool is2d = Seis::is2D( setup_.geom_ );
    const bool isps = Seis::isPS( setup_.geom_ );

    Seis::PosKey pk( setup_.geom_ );
    if ( is2d )
	pk.setTrcNr( bid.crl );
    else
	pk.setBinID( bid );

    const SEGY::DirectDef& dd = *drdr_->getDef();
    for ( int iocc=0; ; iocc++ )
    {
	FileDataSet::TrcIdx tidx = dd.findOcc( pk, iocc );
	if ( !tidx.isValid() )
	    break;
    }

    if ( filefirstidx_ < 0
      || (!is2d && isps && setup_.nridxsperfile_ > 0
	  && bid.inl - filefirstidx_ >= setup_.nridxsperfile_) )
    {
	if ( !openOutputFile(bid.inl) )
	    return false;
    }

    // TODO
    return true;
}


bool SEGY::ReSorter::openOutputFile( int inl )
{
    return false;
}
