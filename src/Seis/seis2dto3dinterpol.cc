/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seis2dto3dinterpol.h"

#include "arrayndalgo.h"
#include "bufstring.h"
#include "dataclipper.h"
#include "fftfilter.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "paralleltask.h"
#include "ptrman.h"
#include "scaler.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisjobexecprov.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "uistrings.h"

#include "statruncalc.h"
#include "statparallelcalc.h"

mImplFactory(Seis2DTo3DInterPol, Seis2DTo3DInterPol::factory)

const char* Seis2DTo3DInterPol::sKeyInput()	{ return "Input ID"; }
const char* Seis2DTo3DInterPol::sKeyType()	{ return "Interpolation type"; }
const char* Seis2DTo3DInterPol::sKeyPow()	{ return "Operator decay"; }
const char* Seis2DTo3DInterPol::sKeyTaper()	{ return "Operator taper"; }
const char* Seis2DTo3DInterPol::sKeySmrtScale() { return "Smart scaling"; }
const char* Seis2DTo3DInterPol::sKeyCreaterType()   { return "Creater Type"; }

Seis2DTo3DInterPol::Seis2DTo3DInterPol()
    : Executor("Generating a 3D cube from a 2DDataSet")
    , inioobj_(0)
    , outioobj_(0)
    , tkzs_(true)
    , read_(false)
    , seisbuf_(*new SeisTrcBuf(true))
    , nrdone_(0)
    , wrr_(0)
    , rdr_(0)
    , tmpseisbuf_(true)
    , trcarr_(0)
    , butterfly_(0)
    , fft_(0)
    , geom_(0)
    , taperangle_(0)
    , pow_(2)
	, strm_(0)
	, taskrun_(0)
{
}

void Seis2DTo3DInterPol::setTaskRunner(TaskRunner* taskr)
{
    taskrun_ = taskr;
}

void Seis2DTo3DInterPol::setStream(od_ostream& strm)
{
    strm_ = &strm;
}

Seis2DTo3DInterPol::~Seis2DTo3DInterPol()
{
    seisbuf_.erase();
    delete wrr_;
    delete rdr_;
    delete inioobj_;
    delete outioobj_;
    delete trcarr_;
    delete butterfly_;
    delete geom_;
}


bool Seis2DTo3DInterPol::init( const IOPar& pars )
{ return usePar(pars); }


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool Seis2DTo3DInterPol::usePar( const IOPar& pars )
{
    if ( !setIO(pars) )
	return false;
    PtrMan<IOPar> parampars = pars.subselect( sKey::Pars() );
    if ( !parampars)
	mErrRet( " No processing parameters found")

    parampars->get( sKeyPow() , pow_);
    parampars->get( sKeyTaper() , taperangle_);
    parampars->getYN( sKeySmrtScale() , smartscaling_);

    return checkParameters();
}


bool Seis2DTo3DInterPol::setIO( const IOPar& pars )
{
    MultiID key;
    pars.get( sKeyInput(), key );
    inioobj_ = IOM().get( key );
    if ( !inioobj_ )
	mErrRet( "2DDataSet not found" )

    pars.get( SeisJobExecProv::sKeySeisOutIDKey(), key );
    outioobj_ = IOM().get( key );
    if ( !outioobj_ )
	mErrRet( "Output cube entry not found" )

    PtrMan<IOPar> subsel = pars.subselect( sKey::Output() );
    if ( !subsel ) return false;

    PtrMan<IOPar> sampling = subsel->subselect( sKey::Subsel() );
    if ( !sampling )
	mErrRet( "No volume processing area found" )
    tkzs_.usePar( *sampling );
    return true;
}


bool Seis2DTo3DInterPol::checkParameters()
{
    if (taperangle_ < 0 || taperangle_ > 90)
	mErrRet( " Taper angle should be between 0 and 90 degrees" )
    return readData();
}


bool Seis2DTo3DInterPol::readData()
{
    if (!read() )
	mErrRet( "Cannot Read Data" )

    const SeisTrc* trc = seisbuf_.get( 0 );
    totnr_ = trc->nrComponents();
    return true;
}


bool Seis2DTo3DInterPol::read()
{
    const SeisIOObjInfo seisinfo( inioobj_ );
    if ( !seisinfo.isOK() || !seisinfo.is2D() )
	return false;

    TypeSet<Pos::GeomID> gids;
    seisinfo.getGeomIDs( gids );
    if ( gids.isEmpty() )
	mErrRet( "Input dataset has no lines" )

    const TrcKeySampling& tks = tkzs_.hsamp_;
    const int ns = tkzs_.zsamp_.nrSteps() + 1;
    const Survey::Geometry* geom = Survey::GM().getGeometry( tks.getGeomID() );
    const Survey::Geometry3D* geom3d = geom ? geom->as3D() : nullptr;
    if ( !geom3d )
	return false;

    seisbuf_.erase();
    seisbuftks_.init( false );
    const Seis::GeomType gt = seisinfo.geomType();
    for ( const auto& geomid : gids )
    {
	SeisTrcReader rdr( *inioobj_, geomid, &gt );
	if ( !rdr.prepareWork() )
	    continue;

	int readres;
	do
	{
	    SeisTrc inptrc;
	    readres = rdr.get( inptrc.info() );
	    if ( readres == -1 )
		return false;
	    else if ( readres == 0 )
		break;
	    else if ( readres == 2 )
		continue;

	    const BinID bid = geom3d->transform( inptrc.info().coord );
	    if ( !tks.includes(bid) || !rdr.get(inptrc) )
		continue;

	    auto* trc = new SeisTrc( inptrc );
	    trc->reSize( ns, false );
	    trc->info().setPos( bid );
	    trc->info().calcCoord();
	    trc->info().sampling.start = tkzs_.zsamp_.start;
	    trc->info().sampling.step = tkzs_.zsamp_.step;
	    for ( int isamp=0; isamp<ns; isamp++ )
	    {
		const float z = tkzs_.zsamp_.atIndex( isamp );
		for ( int icomp=0; icomp<inptrc.nrComponents(); icomp++ )
		    trc->set( isamp, inptrc.getValue(z,icomp), icomp );
	    }
	    seisbuf_.add( trc );
	    seisbuftks_.include( bid );
	} while( readres > 0 );
    }

    if ( seisbuf_.isEmpty() )
	return false;

    read_ = true;
    return true;
}


bool Seis2DTo3DInterPol::readInputCube(const int szfastx,
			       const int szfasty, const int szfastz )
{
    if(nrdone_ == 0)
    trcarr_ = new Array3DImpl<float_complex>(szfastx,szfasty,szfastz);
	if (!trcarr_->isOK())
		return false;
    trcarr_->setAll(float_complex(0.f,0.f));

    if (smartscaling_ && nrdone_ == 0)
    {
	geom_ = new Array3DImpl<float_complex>( *trcarr_);
    }

    rmsmax_ = 0;
    Stats::CalcSetup rcsetup;
    rcsetup.require( Stats::RMS );
    Stats::RunCalc<float> runcalc(rcsetup);

    for( int trcidx=0; trcidx<seisbuf_.size(); trcidx++ )
    {
	const SeisTrc* trc = seisbuf_.get( trcidx );
	const BinID bid = trc->info().binID();
	const int idx = tkzs_.hsamp_.lineIdx(bid.inl());
	const int idy = tkzs_.hsamp_.trcIdx(bid.crl());
	for ( int idz = 0; idz < trc->size(); idz++ )
	{
	    float val = trc->get(idz, nrdone_);
	    trcarr_->set(idx,idy,idz,float_complex(val,0.f));
	    if (!smartscaling_)
	    {
		runcalc.addValue(val);
	    }
	    else if (nrdone_ == 0 )
	    {
		geom_->set(idx,idy,idz,float_complex(1.f,0.f));
	    }
	}
	if (!smartscaling_)
	{
	    if (runcalc.size(false) == 0)   //if
		continue;
	    float rms = (float) runcalc.rms();
	    runcalc.clear();
	    if (rms>rmsmax_)
		rmsmax_ = rms;
	}
    }
	return true;
}



class OperatorComputerExecutor : public ParallelTask
{ mODTextTranslationClass(OperatorComputerExecutor);
public:
    OperatorComputerExecutor( const TrcKeyZSampling& tkzs,
			      const Array3DInfo& info,
			      float_complex* butterfly,
			      const float& taperangle, const float& pow )
	: tkzs_(tkzs)
	, info_(info)
	, taperangle_(taperangle)
	, pow_(pow)
	, butterfly_(butterfly)
	, zscale_(SI().zDomain().isTime() ? 2500.f : 1.f)
    {
	n1_ = info.getSize( 0 );
	n2_ = info.getSize( 1 );
	n3_ = info.getSize( 2 );
    }

    od_int64	nrIterations() const override	{ return n1_*n2_;}

    uiString uiMessage() const override
    { return tr("Constructing Operator"); }
    uiString uiNrDoneText() const override
    { return sTrcFinished(); }

    bool doPrepare( int ) override
    {
	//corner points of reference binids
	TypeSet<BinID> refbid;
	TrcKeySampling hrg( tkzs_.hsamp_);
	hrg.stop_ += BinID( n1_ - tkzs_.hsamp_.nrInl(),
			    n2_ - tkzs_.hsamp_.nrCrl() );

	refbid += BinID( hrg.start_);
	refbid += BinID( hrg.start_.inl(), hrg.stop_.crl() +
			 hrg.step_.crl() );
	refbid += BinID( hrg.stop_.inl() + hrg.step_.inl(),
			 hrg.start_.crl() );
	refbid += BinID( hrg.stop_.inl() + hrg.step_.inl(),
			 hrg.stop_.crl() + hrg.step_.crl() );

	int zdiff = n3_ - tkzs_.nrZ();
	refz_.set( tkzs_.zsamp_.start,
		   tkzs_.zsamp_.stop + (zdiff + 1) * tkzs_.zsamp_.step );
	refz_.scale( zscale_ );

	TypeSet<Coord> refpos2d;
	for ( int i=0; i<refbid.size(); i++)
	    refpos2d += tkzs_.hsamp_.toCoord( refbid[i] );

	refpos3d_.setEmpty();
	for ( int i=0; i<refbid.size(); i++)
	{
	    refpos3d_ += Coord3(refpos2d[i] ,refz_.start);
	    refpos3d_ += Coord3(refpos2d[i] ,refz_.stop);
	}

	return true;
    }

    bool doWork( od_int64 start, od_int64 stop, int ) override
    {
	const float taperangle =  taperangle_ / 180.0f * M_PIf;
	const float mindist = SI().crlDistance();

	for ( int it=(int)start; it<=stop && shouldContinue(); it++)
	{
	    const int idx = it/n2_;
	    const int idy = it%n2_;
	    BinID bid( tkzs_.hsamp_.atIndex(idx,idy) );
	    Coord pos2d( tkzs_.hsamp_.toCoord(bid) );
	    const od_int64 offset = it * n3_ ;
	    for ( int idz=0; idz<n3_; idz++)
	    {
		const Coord3 pos3d(pos2d,
			    tkzs_.zsamp_.step*zscale_*idz + refz_.start);

		int nearestidx = 0;
		float nearest = (float)pos3d.distTo(refpos3d_[0]);
		for ( int i=1; i<refpos3d_.size(); i++)
		{
		    float dist = (float)pos3d.distTo(refpos3d_[i]);
		    if (dist < nearest)
		    {
			nearest = dist;
			nearestidx = i;
		    }
		}
		const float dist2d = (float)pos2d.distTo(
						refpos3d_[nearestidx].coord());
		double topz = pos3d.z -refz_.start;
		double bottomz = refz_.stop-pos3d.z;
		if ( topz > tan( taperangle )*dist2d
			&& bottomz > tan( taperangle )*dist2d )
		    continue;

		const float dist3d =
				    (float)pos3d.distTo(refpos3d_[nearestidx]);
		const float valreal = mIsZero(dist3d/1000,mDefEps) ?
				    10 / (Math::PowerOf(mindist/1000 , pow_)):
				    (1 / (Math::PowerOf(dist3d/1000, pow_)));
		butterfly_[offset+idz] = (float) valreal;
	    }

	    addToNrDone(1);
	}
	return true;
    }

    TypeSet<Coord3>  refpos3d_;
    int n1_, n2_ , n3_;
    Interval<double>	refz_;
    float   zscale_;

    const float pow_;
    const float taperangle_;
    const TrcKeyZSampling&  tkzs_;
    const Array3DInfo&	info_;
    float_complex* butterfly_;
};

#define mDoTransform(tf,isstraight,arr, taskrun) \
{\
tf->setInputInfo( arr->info() );\
tf->setDir(isstraight);\
tf->setNormalization(!isstraight);\
tf->setInput(arr->getData());\
tf->setOutput(arr->getData());\
TaskRunner::execute(taskrun, *fft_);\
}

mDefParallelCalc2Pars( ArrayMultiplierExec,
		       ArrayMultiplierExec::tr("Array multiplication"),
			const float_complex*, a, float_complex*, b )
mDefParallelCalcBody(,b_[idx] *= a_[idx];,)

mDefParallelCalc2Pars( ArrayDivideExec,
			ArrayDivideExec::tr("Array division"),
			const float_complex*, a, float_complex*, b )
mDefParallelCalcBody(,(a_[idx].real()  > 1e-40f) ? b_[idx] /= a_[idx]:
		     b_[idx] = float_complex(mUdf(float),mUdf(float));,)

int Seis2DTo3DInterPol::nextStep()
{
    if ( !read_ )
	return ErrorOccurred();

    delete fft_;
    fft_ = Fourier::CC::createDefault();

    const int szfastx = fft_->getFastSize( tkzs_.nrLines() );
    const int szfasty = fft_->getFastSize( tkzs_.nrTrcs() );
    const int szfastz = fft_->getFastSize( tkzs_.nrZ() );

    *strm_ << " \nConstructing 3D cube ... \n";
	if (!readInputCube(szfastx, szfasty, szfastz))
		return ErrorOccurred();
	*strm_ << " \nFinished constructing 3D cube \n";
	if (!preProcessArray())
		return ErrorOccurred();

	if (nrdone_ == 0)
		if (!butterflyOperator())
			return ErrorOccurred();

    mDoTransform( fft_, true, trcarr_ ,taskrun_);
    if ( nrdone_ == 0 )
    mDoTransform( fft_, true, butterfly_ ,taskrun_ );

    if (smartscaling_ && nrdone_ == 0)
    {
	mDoTransform( fft_, true, geom_ , taskrun_);
	ArrayMultiplierExec meexec( butterfly_->info().getTotalSz(),
				   butterfly_->getData(), geom_->getData() );
	taskrun_->execute( meexec );
	mDoTransform( fft_, false, geom_ , taskrun_);
    }
    ArrayMultiplierExec meexec( butterfly_->info().getTotalSz(),
			       butterfly_->getData(), trcarr_->getData() );
    taskrun_->execute( meexec );
    mDoTransform( fft_, false, trcarr_ , taskrun_);

    *strm_ << " \nScaling results ... \n";
    if (smartscaling_)
    {
    smartScale();
    }
    else
    {
    scaleArray();
    }

	if (!unProcessArray())
		return ErrorOccurred();
    *strm_ << " \nWriting component to disk ... \n";
	if (!writeOutput())
		return ErrorOccurred();
    nrdone_++;

    if (nrdone_ == totalNr() )
	return Finished();

	return MoreToDo();
}


bool Seis2DTo3DInterPol::butterflyOperator()
{
    butterfly_ = new Array3DImpl<float_complex>( *trcarr_ );
    if ( !butterfly_ || !butterfly_->isOK() )
	return false;
    butterfly_->setAll( float_complex(0.f,0.f) );

    OperatorComputerExecutor opcompexec(tkzs_, butterfly_->info(),
				butterfly_->getData(), taperangle_, pow_);
    taskrun_->execute(opcompexec);
	return true;
}


bool Seis2DTo3DInterPol::scaleArray()
{
    Stats::CalcSetup rcsetup;
    rcsetup.require( Stats::RMS );
    Stats::RunCalc<float> runcalc( rcsetup);
    const TrcKeySampling& hrg = tkzs_.hsamp_;
    TrcKeySamplingIterator iter( hrg );
    float_complex* data = trcarr_->getData();
    BinID bid;
    const int nz = tkzs_.nrZ();
    do
    {
	const TrcKey trk( iter.curTrcKey() );
	const int inlpos = hrg.lineIdx( trk.lineNr() );
	const int crlpos = hrg.trcIdx( trk.trcNr() );
	od_uint64 ipos = trcarr_->info().getOffset( inlpos, crlpos, 0 );
	for ( int idz=0; idz<nz; idz++ )
	{
	    const float_complex val = data[ipos+idz];
	    runcalc.addValue( val.real() );
	}

	const float rms = (float) runcalc.rms();
	runcalc.clear();
	for( int idz=0; idz<nz; idz++ )
	{
	    const float val = data[ipos+idz].real() * rmsmax_ / rms;
	    data[ipos+idz] = float_complex(val,0.f);
	}
    } while ( iter.next(bid) );

    return true;
}


void Seis2DTo3DInterPol::smartScale()
{
    ArrayDivideExec adexec( trcarr_->info().getTotalSz(), geom_->getData(),
			       trcarr_->getData() );
    taskrun_->execute( adexec );
}


bool Seis2DTo3DInterPol::writeOutput()
{
    delete wrr_;
    const Seis::GeomType gt = Seis::Vol;
    wrr_ = new SeisTrcWriter( *outioobj_, &gt );

    if ( nrdone_ != 0 )
    {
	delete rdr_;
	rdr_ = new SeisTrcReader( *outioobj_, &gt );
    }

    const TrcKeySampling& hrg = tkzs_.hsamp_;
    TrcKeySamplingIterator iter( hrg );
    SeisTrc& trc( *seisbuf_.get(0) );
    trc.info().setPos( hrg.start_ );
    trc.info().sampling = tkzs_.zsamp_;
    const int nrz = tkzs_.nrZ();
    BinID bid;
    do
    {
	const TrcKey trk( iter.curTrcKey() );
	const int inlpos = hrg.lineIdx( trk.lineNr() );
	const int crlpos = hrg.trcIdx( trk.trcNr() );

	if ( nrdone_ != 0 )
	    rdr_->get(trc);

	trc.info().setPos( trk.position() );
	for ( int idz=0; idz<nrz; idz++ )
	{
	    const float val = trcarr_->get(inlpos,crlpos,idz).real();
	    trc.set( idz, val, nrdone_ );
	}
	if ( !wrr_->put(trc) )
	    mErrRet( "Cannot write traces" );

    } while ( iter.next(bid) );

    return true;
}


od_int64 Seis2DTo3DInterPol::totalNr() const
{
    return totnr_;
}
