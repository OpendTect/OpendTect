/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2014
-*/


#include "seiscopy.h"
#include "seis2ddata.h"
#include "seistrc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seistrcprop.h"
#include "seissingtrcproc.h"
#include "seisselectionimpl.h"
#include "scaler.h"
#include "survgeom.h"
#include "ioobj.h"
#include "veldesc.h"
#include "velocitycalc.h"

static uiString sNrTrcsCopied = toUiString("Number of traces copied");


#define mNoVelocity 0
#define mVelocityIntv 1
#define mVelocityRMS 2
#define mVelocityAvg 3


static int getVelType( const IOPar& iop )
{
    if ( !iop.isTrue(VelocityDesc::sKeyIsVelocity()) )
	return mNoVelocity;

    const FixedString typestr = iop.find( VelocityDesc::sKeyVelocityType() );
    if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::RMS] )
	return mVelocityRMS;
    else if ( typestr == VelocityDesc::TypeNames()[VelocityDesc::Avg] )
	return mVelocityAvg;

    return mVelocityIntv;
}


SeisCubeCopier::SeisCubeCopier( const IOObj& inobj, const IOObj& outobj,
				const IOPar& par, int compnr )
    : Executor("Copying 3D Cube")
    , stp_(new SeisSingleTraceProc(inobj,outobj,"Cube copier",&par))
    , compnr_(compnr)
{
    init();
}


SeisCubeCopier::SeisCubeCopier( SeisSingleTraceProc* tp, int compnr )
    : Executor("Copying 3D Cube")
    , stp_(tp)
    , compnr_(compnr)
{
    init();
}


void SeisCubeCopier::init()
{
    veltype_ = mNoVelocity;
    if ( !stp_ )
	return;

    const SeisTrcWriter& wrr = stp_->writer();
    if ( wrr.ioObj() )
	veltype_ = getVelType( wrr.ioObj()->pars() );

    if ( !stp_->reader(0) )
    {
	errmsg_ = stp_->uiMessage();
	if ( errmsg_.isEmpty() )
	    errmsg_ = uiStrings::phrCannotRead( uiStrings::sInput() );
	delete stp_; stp_ = 0;
    }

    if ( stp_ && (compnr_>=0 || veltype_>0) )
	stp_->proctobedone_.notify( mCB(this,SeisCubeCopier,doProc) );
}


SeisCubeCopier::~SeisCubeCopier()
{
    delete stp_;
}


uiString SeisCubeCopier::uiNrDoneText() const
{
    return sNrTrcsCopied;
}


od_int64 SeisCubeCopier::totalNr() const
{
    return stp_ ? stp_->totalNr() : -1;
}


od_int64 SeisCubeCopier::nrDone() const
{
    return stp_ ? stp_->nrDone() : 0;
}


uiString SeisCubeCopier::uiMessage() const
{
    return stp_ ? stp_->uiMessage() : errmsg_;
}


int SeisCubeCopier::nextStep()
{
    return stp_ ? stp_->doStep() : ErrorOccurred();
}


void SeisCubeCopier::doProc( CallBacker* )
{
    SeisTrc& trc = stp_->getTrace();
    const int trcsz = trc.size();

    if ( veltype_ > 0 )
    {
	mAllocVarLenArr( float, vout, trcsz );
	if ( !mIsVarLenArrOK(vout) )
	    return;

	const SeisTrc& intrc = stp_->getInputTrace();
	const int sizein = intrc.size();
	const SamplingData<float> sdin =
				getDoubleSamplingData( intrc.info().sampling );
	mAllocVarLenArr( double, timevals, sizein )
	if ( !mIsVarLenArrOK(timevals) ) return;
	for ( int idx=0; idx<sizein; idx++ )
	    timevals[idx] = sdin.atIndex( idx );

	const int nrcomps = trc.nrComponents();
	const SamplingData<double> sdout =
				getDoubleSamplingData( trc.info().sampling );
	const Scaler* scaler = stp_->scaler();

	for ( int icomp=0; icomp<nrcomps; icomp++ )
	{
	    TypeSet<float> trcvals;
	    for ( int idx=0; idx<sizein; idx++ )
		trcvals += intrc.get( idx, icomp );

	    const float* vin = trcvals.arr();
	    if ( veltype_ == mVelocityIntv )
		sampleVint( vin, timevals, sizein, sdout, vout, trcsz );
	    else if ( veltype_ == mVelocityRMS )
		sampleVrms( vin, 0., 0, timevals, sizein, sdout, vout, trcsz );
	    else if ( veltype_ == mVelocityAvg )
		sampleVavg( vin, timevals, sizein, sdout, vout, trcsz );

	    for ( int idx=0; idx<trcsz; idx++ )
	    {
		float trcval = vout[idx];
		if ( scaler )
		    trcval = (float)scaler->scale( trcval );
		trc.set( idx, trcval, icomp );
	    }
	}
    }

    if ( compnr_ >= 0 )
    {
	SeisTrc tmp( trc );
	while ( trc.nrComponents() > 1 )
	    trc.data().delComponent( 0 );

	for ( int idx=0; idx<trcsz; idx++ )
	    trc.set( idx, tmp.get(idx,compnr_), 0 );
    }
}


Seis2DCopier::Seis2DCopier( const IOObj& inobj, const IOObj& outobj,
			    const IOPar& par )
    : Executor("Copying 2D Seismic Data")
    , inioobj_(*inobj.clone())
    , outioobj_(*outobj.clone())
    , rdr_(0)
    , wrr_(0)
    , seldata_(*new Seis::RangeSelData)
    , lineidx_(-1)
    , scaler_(0)
    , totalnr_(0)
    , nrdone_(0)
    , msg_(tr("Copying traces"))
{
    PtrMan<IOPar> lspar = par.subselect( sKey::Line() );
    if ( !lspar || lspar->isEmpty() )
	{ msg_ = toUiString("Internal: Required data missing"); return; }

    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar || linepar->isEmpty() )
	    break;

	Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	if ( !linepar->get(sKey::GeomID(),geomid) )
	    continue;

	selgeomids_ += geomid;
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	if ( !linepar->get(sKey::TrcRange(),trcrg) ||
		!linepar->get(sKey::ZRange(),zrg))
	    continue;

	trcrgs_ += trcrg;
	zrgs_ += zrg;
    }

    if ( trcrgs_.size() != selgeomids_.size() ) trcrgs_.erase();
    if ( zrgs_.size() != selgeomids_.size() ) zrgs_.erase();

    FixedString scalestr = par.find( sKey::Scale() );
    if ( !scalestr.isEmpty() )
	scaler_ = Scaler::get( scalestr );

    if ( trcrgs_.isEmpty() )
    {
	Seis2DDataSet dset( inobj );
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	for ( int idx=0; idx<selgeomids_.size(); idx++ )
	{
	    if ( dset.getRanges(selgeomids_[idx],trcrg,zrg) )
		totalnr_ += ( trcrg.nrSteps() + 1 );
	}
    }
    else
    {
	for ( int idx=0; idx<trcrgs_.size(); idx++ )
	    totalnr_ += ( trcrgs_[idx].nrSteps() + 1 );
    }

    if ( totalnr_ < 1 )
	msg_ = tr("No traces to copy");
}


Seis2DCopier::~Seis2DCopier()
{
    delete rdr_; delete wrr_;
    delete scaler_;
    delete &seldata_;
    delete (IOObj*)(&inioobj_);
    delete (IOObj*)(&outioobj_);
}


bool Seis2DCopier::initNextLine()
{
    delete rdr_; rdr_ = new SeisTrcReader( &inioobj_ );
    delete wrr_; wrr_ = new SeisTrcWriter( &outioobj_ );

    lineidx_++;
    if ( lineidx_ >= selgeomids_.size() )
	return false;

    if ( trcrgs_.isEmpty() )
	seldata_.setIsAll( true );
    else
    {
	seldata_.cubeSampling().hsamp_.setCrlRange( trcrgs_[lineidx_] );
	seldata_.cubeSampling().zsamp_ = zrgs_[lineidx_];
    }

    seldata_.setGeomID( selgeomids_[lineidx_] );
    rdr_->setSelData( seldata_.clone() );
    wrr_->setSelData( seldata_.clone() );
    if ( !rdr_->prepareWork() )
	{ msg_ = rdr_->errMsg(); return false; }

    return true;
}


uiString Seis2DCopier::uiNrDoneText() const
{
    return sNrTrcsCopied;
}


int Seis2DCopier::nextStep()
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
