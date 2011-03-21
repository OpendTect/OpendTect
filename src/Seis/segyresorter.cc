/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2008
-*/
static const char* rcsID = "$Id: segyresorter.cc,v 1.1 2011-03-21 16:16:04 cvsbert Exp $";

#include "segyresorter.h"
#include "segydirecttr.h"
#include "segytr.h"
#include "seistrc.h"
#include "strmprov.h"
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
    {
	msg_ = drdr_->errMsg();
	delete drdr_; drdr_ = 0;
    }
    if ( !drdr_ )
	return;

    const SEGY::DirectDef& dd = *drdr_->getDef();
}


SEGY::ReSorter::~ReSorter()
{
    delete drdr_;
}


int SEGY::ReSorter::nextStep()
{
    if ( !drdr_ )
	return ErrorOccurred();

    return Finished();
}
