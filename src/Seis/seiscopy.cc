/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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

    const StringView typestr = iop.find( VelocityDesc::sKeyVelocityType() );
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
    mAttachCB( stp_->proctobedone_, SeisCubeCopier::doProc );
}


SeisCubeCopier::SeisCubeCopier( SeisSingleTraceProc* tp, int compnr )
    : Executor("Copying 3D Cube")
    , stp_(tp)
    , compnr_(compnr)
{
    if ( !stp_ )
    {
	pErrMsg("Should not provide a null SeisSingleTraceProc");
	return;
    }

    mAttachCB( stp_->proctobedone_, SeisCubeCopier::doProc );
}


SeisCubeCopier::~SeisCubeCopier()
{
    detachAllNotifiers();
    delete stp_;
}


bool SeisCubeCopier::goImpl( od_ostream* strm, bool first, bool last,
			     int delay )
{
    if ( !stp_ )
	return false;

    inited_ = false;
    const bool res = stp_->goImpl( strm, first, last, delay );
    return res;
}


bool SeisCubeCopier::init()
{
    veltype_ = mNoVelocity;
    if ( !stp_ )
	return false;

    const SeisTrcWriter* wrr = stp_->writer();
    if ( !stp_->reader(0) || !wrr )
    {
	errmsg_ = stp_->uiMessage();
	if ( errmsg_.isEmpty() )
	    errmsg_ = uiStrings::phrCannotRead( uiStrings::sInput() );
	deleteAndZeroPtr( stp_ );
    }

    if ( wrr->ioObj() )
	veltype_ = getVelType( wrr->ioObj()->pars() );

    if ( compnr_<0 && veltype_<=0 )
    {
	mDetachCB( stp_->proctobedone_, SeisCubeCopier::doProc );
	return false;
    }

    return true;
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
    if ( !inited_ )
    {
	if ( !init() )
	    return;
    }

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
    , msg_(tr("Copying traces"))
{
    PtrMan<IOPar> outpars = par.subselect( sKey::Output() );
    PtrMan<IOPar> lspar = outpars ? outpars->subselect( sKey::Line() )
				  : nullptr;
    if ( !lspar || lspar->isEmpty() )
    {
	lspar = par.subselect( sKey::Line() );
	if ( !lspar || lspar->isEmpty() )
	    { msg_ = toUiString("Internal: Required data missing"); return; }
    }

    for ( int idx=0; ; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar || linepar->isEmpty() )
	    break;

	Pos::GeomID geomid = mUdfGeomID;
	if ( !linepar->get(sKey::GeomID(),geomid) || !Survey::is2DGeom(geomid) )
	    continue;

	auto* sd = new Seis::RangeSelData( false );
	sd->setGeomID( geomid );
	StepInterval<int> trcrg;
	if ( linepar->get(sKey::TrcRange(),trcrg) )
	    sd->setCrlRange( trcrg );

	ZSampling zrg;
	if ( linepar->get(sKey::ZRange(),zrg) )
	    sd->setZRange( zrg );

	totalnr_ += sd->cubeSampling().hsamp_.totalNr();
	seldatas_.add( sd );
    }

    StringView scalestr = par.find( sKey::Scale() );
    if ( !scalestr.isEmpty() )
	scaler_ = Scaler::get( scalestr );

    if ( totalnr_ < 1 )
	msg_ = tr("No traces to copy");
}


Seis2DCopier::~Seis2DCopier()
{
    delete rdr_;
    delete wrr_;
    delete scaler_;
    deepErase( seldatas_ );
}


bool Seis2DCopier::initNextLine()
{
    if ( !seldatas_.validIdx(++lineidx_) )
	return false;

    const Seis::GeomType gt = Seis::Line;
    const Seis::SelData* sd = seldatas_.get( lineidx_ );
    const Pos::GeomID geomid = sd->geomID();
    delete rdr_; rdr_ = new SeisTrcReader( inioobj_, geomid, &gt  );
    delete wrr_; wrr_ = new SeisTrcWriter( outioobj_, geomid, &gt );
    if ( !sd->isAll() )
    {
	rdr_->setSelData( sd->clone() );
	wrr_->setSelData( sd->clone() );
    }

    if ( !rdr_->prepareWork() )
	{ msg_ = rdr_->errMsg(); return false; }

    return true;
}


uiString Seis2DCopier::uiNrDoneText() const
{
    return sNrTrcsCopied;
}


bool Seis2DCopier::goImpl( od_ostream* strm, bool first, bool last, int delay )
{
    lineidx_ = -1;
    if ( !initNextLine() )
	return false;

    const bool res = Executor::goImpl( strm, first, last, delay );

    deleteAndZeroPtr( rdr_ );
    deleteAndZeroPtr( wrr_ );

    return res;
}


int Seis2DCopier::nextStep()
{
    if ( !rdr_ || !wrr_ )
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
