/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2014
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seiscopy.h"
#include "seistrc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrcprop.h"
#include "seisselectionimpl.h"
#include "scaler.h"
#include "survgeom.h"
#include "ioobj.h"


SeisLineSetCopier::SeisLineSetCopier( const IOObj& inobj, const IOObj& outobj,
			    const IOPar& par )
    : Executor("Copying 2D Seismic Data")
    , lineidx_(-1)
    , inioobj_(*inobj.clone())
    , outioobj_(*outobj.clone())
    , rdr_(0),wrr_(0)
    , seldata_(*new Seis::RangeSelData)
    , scaler_(0)
    , totalnr_(0)
    , nrdone_(0)
    , msg_("Copying traces")
{
    PtrMan<IOPar> lspar = par.subselect( sKey::Line() );
    if ( !lspar || lspar->isEmpty() )
	{ msg_ = "Internal: Required data missing"; return; }

    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar || linepar->isEmpty() )
	    break;

	Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	if ( !linepar->get(sKey::GeomID(),geomid) )
	    continue;

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( !linepar->get(sKey::TrcRange(),trcrg) ||
		!linepar->get(sKey::ZRange(),zrg))
	    continue;

	selgeomids_ += geomid;
	trcrgs_ += trcrg;
	zrgs_ += zrg;
    }

    FixedString scalestr = par.find( sKey::Scale() );
    if ( !scalestr.isEmpty() )
	scaler_ = Scaler::get( scalestr );

    for ( int idx=0; idx<trcrgs_.size(); idx++ )
	totalnr_ += ( trcrgs_[idx].nrSteps() + 1 );

    if ( totalnr_ < 1 )
	msg_ = "No traces to copy";
}


SeisLineSetCopier::~SeisLineSetCopier()
{
    delete rdr_; delete wrr_;
    delete scaler_;
    delete &seldata_;
    delete (IOObj*)(&inioobj_);
    delete (IOObj*)(&outioobj_);
}


bool SeisLineSetCopier::initNextLine()
{
    delete rdr_; rdr_ = new SeisTrcReader( &inioobj_ );
    delete wrr_; wrr_ = new SeisTrcWriter( &outioobj_ );

    lineidx_++;
    if ( lineidx_ >= selgeomids_.size() || lineidx_ >= trcrgs_.size() )
	return false;

    seldata_.cubeSampling().hrg.setCrlRange( trcrgs_[lineidx_] );
    seldata_.cubeSampling().zsamp_ = zrgs_[lineidx_];
    seldata_.setGeomID( selgeomids_[lineidx_] );
    rdr_->setSelData( seldata_.clone() );
    wrr_->setSelData( seldata_.clone() );
    if ( !rdr_->prepareWork() )
	{ msg_ = rdr_->errMsg(); return false; }

    return true;
}


uiString SeisLineSetCopier::uiNrDoneText() const
{
    return "Number of traces copied";
}


int SeisLineSetCopier::nextStep()
{
    if ( lineidx_ < 0 && !initNextLine() )
	return ErrorOccurred();

    SeisTrc trc;
    const int res = rdr_->get( trc.info() );
    if ( res < 0 )
	{ msg_ = rdr_->errMsg(); return ErrorOccurred(); }
    if ( res == 0 )
	return initNextLine() ? MoreToDo() : Finished();

    if ( !rdr_->get(trc) )
	{ msg_ = rdr_->errMsg(); return ErrorOccurred(); }

    if ( scaler_ )
    {
	SeisTrcPropChg stpc( trc );
	stpc.scale( *scaler_ );
    }

    if ( !wrr_->put(trc) )
	{ msg_ = wrr_->errMsg(); return ErrorOccurred(); }

    nrdone_++;
    return MoreToDo();
}
