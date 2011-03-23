/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2008
-*/
static const char* rcsID = "$Id: segyresorter.cc,v 1.2 2011-03-23 12:00:18 cvsbert Exp $";

#include "segyresorter.h"
#include "segydirectdef.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "seistrc.h"
#include "strmprov.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "ioman.h"

SEGY::ReSorter::Setup::Setup( Seis::GeomType gt, const MultiID& ky,
			      const char* fnm )
    : geom_(gt)
    , inpkey_(ky)
    , outfnm_(fnm)
    , nridxsperfile_(1)
    , sortkey1_(Seis::is2D(gt) ? Crl : Inl)
    , sortkey2_(Crl)
{
}


SEGY::ReSorter::ReSorter( const SEGY::ReSorter::Setup& su, const char* lnm )
    : Executor("Re-sorting")
    , setup_(su)
    , trc_(*new SeisTrc)
    , msg_("Handling traces")
    , nrdone_(0)
    , drdr_(0)
    , inlidx_(0)
    , segidx_(0)
    , crlidx_(0)
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
}


int SEGY::ReSorter::wrapUp()
{
    delete drdr_; drdr_ = 0;
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

    crlidx_++;
    nrdone_++;
    return MoreToDo();
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
	if ( inlidx_ >= cd.size() )
	    return false;

	if ( crlidx_ > cd[inlidx_]->segments_[segidx_].nrSteps() )
	{
	    segidx_++; crlidx_ = 0;
	    if ( segidx_ >= cd[inlidx_]->segments_.size() )
	    {
		inlidx_++; segidx_ = 0;
		if ( inlidx_ >= cd.size() )
		    return false;
	    }
	}
	const PosInfo::LineData& ld = *cd[inlidx_];
	bid.inl = ld.linenr_;
	bid.crl = ld.segments_[segidx_].atIndex( crlidx_ );;
    }

    return true;
}


bool SEGY::ReSorter::createOutput( const BinID& bid )
{
    // TODO
    return true;
}
