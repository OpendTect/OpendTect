/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seiscopy.h"

#include "ioobj.h"
#include "keystrs.h"
#include "seis2ddata.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seissingtrcproc.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seiswrite.h"
#include "survgeom.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "veldesc.h"
#include "zvalseriesimpl.h"


static uiString sNrTrcsCopied = toUiString("Number of traces copied");


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
    delete worker_;
}


bool SeisCubeCopier::goImpl( od_ostream* strm, bool first, bool last,
			     int delay )
{
    if ( !stp_ || !init() )
	return false;

    const bool res = stp_->goImpl( strm, first, last, delay );
    deleteAndNullPtr( worker_ );
    return res;
}


bool SeisCubeCopier::init()
{
    if ( !stp_ )
	return false;

    const SeisTrcReader* rdr = stp_->reader();
    const SeisTrcWriter* wrr = stp_->writer();
    if ( !rdr || !wrr )
    {
	errmsg_ = stp_->uiMessage();
	if ( errmsg_.isEmpty() )
	    errmsg_ = uiStrings::phrCannotRead( uiStrings::sInput() );
	deleteAndNullPtr( stp_ );
    }

    VelocityDesc veldesc;
    if ( wrr->ioObj() )
	veldesc.usePar( rdr->ioObj()->pars() );

    if ( !veldesc.isUdf() )
    {
	delete worker_;
	worker_ = new Vel::Worker( veldesc, SI().seismicReferenceDatum(),
				   UnitOfMeasure::surveyDefSRDStorageUnit() );
    }

    if ( compnr_<0 && !worker_ )
	mDetachCB( stp_->proctobedone_, SeisCubeCopier::doProc );

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
    SeisTrc& trc = stp_->getTrace();
    if ( worker_ )
	resampleVels( stp_->getInputTrace(), trc );

    if ( compnr_ >= 0 )
	cropComponents( trc );
}


bool SeisCubeCopier::resampleVels( const SeisTrc& inptrc, SeisTrc& trc ) const
{
    const SeisTrcReader* rdr = stp_->reader();
    const SeisTrcWriter* wrr = stp_->writer();
    if ( !rdr || !wrr )
	return false;

    const RegularZValues zvals_in( inptrc.info().sampling,
				   inptrc.size(), rdr->zDomain() );
    const RegularZValues zvals_out( trc.info().sampling,
				    trc.size(), wrr->zDomain() );
    if ( zvals_in == zvals_out )
	return true;

    const Scaler* scaler = stp_->scaler();
    SeisTrcValueSeries inptrcvs( inptrc, 0 );
    SeisTrcValueSeries outptrcvs( trc, 0 );
    PtrMan<ValueSeries<double> > inpvels, outpvels;
    inpvels = ScaledValueSeries<double,float>::getFrom( inptrcvs );
    outpvels = ScaledValueSeries<double,float>::getFrom( outptrcvs );
    for ( int icomp=0; icomp<inptrc.nrComponents(); icomp++ )
    {
	if ( icomp == compnr_ || compnr_ < 0 )
	{
	    inptrcvs.setComponent( icomp );
	    outptrcvs.setComponent( icomp );
	    worker_->sampleVelocities( *inpvels, zvals_in,
				       zvals_out, *outpvels );
	    if ( scaler )
		trc.data().scale( *scaler, icomp );
	}
    }

    return true;
}


void SeisCubeCopier::cropComponents( SeisTrc& trc ) const
{
    const TraceData& td = trc.data();
    const DataCharacteristics dc( td.getInterpreter(compnr_)->dataChar() );
    SeisTrc tmp( trc.size(), dc );
    tmp.data().copyFrom( td, compnr_, compnr_ );
    trc.data() = tmp.data();
}


// Seis2DCopier

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

	Pos::GeomID geomid;
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

    const BufferString scalestr = par.find( sKey::Scale() );
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

    deleteAndNullPtr( rdr_ );
    deleteAndNullPtr( wrr_ );

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
