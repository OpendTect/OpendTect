/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2014
-*/


#include "seiscopy.h"
#include "scaler.h"
#include "seis2ddata.h"
#include "seistrc.h"
#include "seisprovider.h"
#include "seisrangeseldata.h"
#include "seisstorer.h"
#include "seissingtrcproc.h"
#include "seistrcprop.h"
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
    if ( typestr == VelocityDesc::TypeDef().getKey(VelocityDesc::RMS) )
	return mVelocityRMS;
    else if ( typestr == VelocityDesc::TypeDef().getKey(VelocityDesc::Avg) )
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

    const auto& strr = stp_->storer();
    if ( strr.ioObj() )
	veltype_ = getVelType( strr.ioObj()->pars() );

    if ( !stp_->provider() )
    {
	errmsg_ = stp_->message();
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


uiString SeisCubeCopier::nrDoneText() const
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


uiString SeisCubeCopier::message() const
{
    return stp_ ? stp_->message() : errmsg_;
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
				getDoubleSamplingData( intrc.info().sampling_ );
	mAllocVarLenArr( double, timevals, sizein )
	if ( !mIsVarLenArrOK(timevals) ) return;
	for ( int idx=0; idx<sizein; idx++ )
	    timevals[idx] = sdin.atIndex( idx );

	const int nrcomps = trc.nrComponents();
	const SamplingData<double> sdout =
				   getDoubleSamplingData( trc.info().sampling_);
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
    : Seis2DCopier(inobj,outobj)
{
    seldata_.usePar( par );
    const BufferString scalestr = par.find( sKey::Scale() );
    if ( !scalestr.isEmpty() )
	scaler_ = Scaler::get( scalestr );
}


Seis2DCopier::Seis2DCopier( const IOObj& inobj, const IOObj& outobj )
    : Executor("Copying 2D Seismic Data")
    , storer_(*new Seis::Storer(outobj))
    , seldata_(*new Seis::RangeSelData)
{
    prov_ = Seis::Provider::create( inobj, &uirv_ );
    if ( !storer_.isUsable() )
	uirv_.add( storer_.errNotUsable() );
}


Seis2DCopier::~Seis2DCopier()
{
    delete scaler_;
    delete prov_;
    delete &storer_;
    delete &seldata_;
}


bool Seis2DCopier::init()
{
    if ( !prov_ )
	return false;

    prov_->setSelData( seldata_.clone() );
    totalnr_ = seldata_.expectedNrTraces();

    inited_ = true;
    return true;
}


uiString Seis2DCopier::nrDoneText() const
{
    return sNrTrcsCopied;
}


uiString Seis2DCopier::message() const
{
    return uirv_.isOK() ? tr("Copying traces") : uiString( uirv_ );
}


int Seis2DCopier::nextStep()
{
    if ( !uirv_.isOK() )
	return ErrorOccurred();
    else if ( !inited_ )
	return init() ? MoreToDo() : ErrorOccurred();

    SeisTrc trc;
    uirv_ = prov_->getNext( trc );
    if ( !uirv_.isOK() )
	return isFinished(uirv_) ? Finished() : ErrorOccurred();

    if ( scaler_ )
    {
	SeisTrcPropChg stpc( trc );
	stpc.scale( *scaler_ );
    }

    uirv_ = storer_.put( trc );
    if ( !uirv_.isOK() )
	return ErrorOccurred();

    nrdone_++;
    return MoreToDo();
}
