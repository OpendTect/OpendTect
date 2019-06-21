/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
________________________________________________________________________

-*/


#include "seis2dto3d.h"

#include "arrayndalgo.h"
#include "bufstring.h"
#include "dataclipper.h"
#include "fftfilter.h"
#include "ioobj.h"
#include "keystrs.h"
#include "paralleltask.h"
#include "ptrman.h"
#include "scaler.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seisjobexecprov.h"
#include "seisprovider.h"
#include "seisstorer.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "uistrings.h"

#include "statruncalc.h"
#include "statparallelcalc.h"

mImplClassFactory( Seis2DTo3D, factory )

const char* Seis2DTo3D::sKeyInput()	{ return "Input ID"; }
const char* Seis2DTo3D::sKeyType()	{ return "Interpolation type"; }
const char* Seis2DTo3D::sKeyPow()	{ return "Operator decay"; }
const char* Seis2DTo3D::sKeyTaper()	{ return "Operator taper"; }
const char* Seis2DTo3D::sKeySmrtScale() { return "Smart scaling"; }

Seis2DTo3D::Seis2DTo3D()
    : Executor("Generating a 3D cube from a 2DDataSet")
    , inioobj_(0)
    , outioobj_(0)
    , tkzs_(true)
    , read_(false)
    , seisbuf_(*new SeisTrcBuf(true))
    , nrdone_(0)
    , prov_(0)
    , storer_(0)
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

void Seis2DTo3D::setTaskRunner(TaskRunner* taskr)
{
	taskrun_ = taskr;
}

void Seis2DTo3D::setStream(od_ostream& strm)
{
	strm_ = &strm;
}

Seis2DTo3D::~Seis2DTo3D()
{
    seisbuf_.erase();
    delete storer_;
    delete prov_;
    delete inioobj_;
    delete outioobj_;
    delete trcarr_;
    delete butterfly_;
    delete geom_;
}


bool Seis2DTo3D::init( const IOPar& pars )
{ return usePar(pars); }


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool Seis2DTo3D::usePar( const IOPar& pars )
{
    if ( !setIO(pars) )
	return false;
    PtrMan<IOPar> parampars = pars.subselect( sKey::Pars() );
    if ( !parampars)
	mErrRet( tr(" No processing parameters found"))

    parampars->get( sKeyPow() , pow_);
    parampars->get( sKeyTaper() , taperangle_);
    parampars->getYN( sKeySmrtScale() , smartscaling_);

    return checkParameters();
}


bool Seis2DTo3D::setIO( const IOPar& pars )
{
    DBKey key;
    pars.get( sKeyInput(), key );
    inioobj_ = key.getIOObj();
    if ( !inioobj_ )
	mErrRet( tr("2DDataSet not found") )

    pars.get( SeisJobExecProv::sKeySeisOutIDKey(), key );
    outioobj_ = key.getIOObj();
    if ( !outioobj_ )
	mErrRet( tr("Output cube entry not found") )

    PtrMan<IOPar> subsel = pars.subselect( sKey::Output() );
    if ( !subsel ) return false;

    PtrMan<IOPar> sampling = subsel->subselect( sKey::Subsel() );
    if ( !sampling )
	mErrRet( tr("No volume processing area found") )
    tkzs_.usePar( *sampling );
    return true;
}


bool Seis2DTo3D::checkParameters()
{
    if (taperangle_ < 0 || taperangle_ > 90)
	mErrRet( tr(" Taper angle should be between 0 and 90 degrees" ) )
    return readData();
}


bool Seis2DTo3D::readData()
{
    if (!read() )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sData()) )

    const SeisTrc* trc = seisbuf_.get( 0 );
    totnr_ = trc->nrComponents();
    return true;
}


bool Seis2DTo3D::read()
{
    if ( !inioobj_ ) return false;

    Seis2DDataSet ds( *inioobj_ );
    if ( ds.isEmpty() )
	mErrRet( tr("Input dataset has no lines") )

    SeisTrcBuf tmpbuf(false);
    seisbuf_.erase();
    seisbuftks_.init( false );
    for ( int iline=0; iline<ds.nrLines(); iline++)
    {
	uiRetVal uirv;
	PtrMan<Seis2DTraceGetter> getter =
		ds.traceGetter( ds.geomID(iline), 0, uirv );
	if ( !getter )
	    continue;

	while ( true )
	{
	    SeisTrc* trc = new SeisTrc;
	    uirv = getter->getNext( *trc );
	    if ( !uirv.isOK() )
	    {
		delete trc;
		if ( isFinished(uirv) )
		    break;

		mErrRet( uirv );
	    }

	    tmpbuf.add( trc );
	}

	for ( int idx=tmpbuf.size()-1; idx>=0; idx-- )
	{
	    const SeisTrc& intrc = *tmpbuf.get( idx );
	    if ( !tkzs_.hsamp_.includes(intrc.info().binID()) )
		continue;

	    const BinID bid = intrc.info().binID();
	    SeisTrc* trc = new SeisTrc( intrc );
	    const int ns = tkzs_.zsamp_.nrSteps() + 1;
	    trc->reSize( ns, false );
	    trc->info().sampling_.start = tkzs_.zsamp_.start;
	    trc->info().sampling_.step = tkzs_.zsamp_.step;
	    for ( int isamp=0; isamp<ns; isamp++ )
	    {
		const float z = tkzs_.zsamp_.atIndex( isamp );
		for ( int icomp=0; icomp<intrc.nrComponents(); icomp++ )
		    trc->set( isamp, intrc.getValue(z,icomp), icomp );
	    }
	    seisbuf_.add( trc );
	    seisbuftks_.include( bid );
	}
	tmpbuf.erase();
    }
    if ( seisbuf_.isEmpty() )
	return false;

    read_ = true;
    return true;
}


bool Seis2DTo3D::readInputCube(const int szfastx,
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

    od_int64	nrIterations() const	{return n1_*n2_;}
    uiString	message() const   {return tr("Constructing Operator"); }
    uiString	nrDoneText() const	{return tr("Position finished"); }

    bool doPrepare( int )
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

    bool doWork ( od_int64 start, od_int64 stop, int )
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
		float nearest = pos3d.distTo<float>(refpos3d_[0]);
		for ( int i=1; i<refpos3d_.size(); i++)
		{
		    float dist = pos3d.distTo<float>(refpos3d_[i]);
		    if (dist < nearest)
		    {
			nearest = dist;
			nearestidx = i;
		    }
		}
		const float dist2d =
			    pos2d.distTo<float>(refpos3d_[nearestidx].getXY());
		double topz = pos3d.z_ -refz_.start;
		double bottomz = refz_.stop-pos3d.z_;
		if ( topz > tan( taperangle )*dist2d
			&& bottomz > tan( taperangle )*dist2d )
		    continue;

		const float dist3d = pos3d.distTo<float>(refpos3d_[nearestidx]);
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

int Seis2DTo3D::nextStep()
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
	ArrayMultiplierExec meexec( butterfly_->totalSize(),
				    butterfly_->getData(), geom_->getData() );
	taskrun_->execute( meexec );
	mDoTransform( fft_, false, geom_ , taskrun_);
    }
    ArrayMultiplierExec meexec( butterfly_->totalSize(),
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


bool Seis2DTo3D::butterflyOperator()
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


bool Seis2DTo3D::scaleArray()
{
    Stats::CalcSetup rcsetup;
    rcsetup.require( Stats::RMS );
    Stats::RunCalc<float> runcalc( rcsetup);
    const TrcKeySampling& hrg = tkzs_.hsamp_;
    TrcKeySamplingIterator iter( hrg );
    float_complex* data = trcarr_->getData();

    const int nz = tkzs_.nrZ();
    do
    {
	const TrcKey trk( iter.curTrcKey() );
	const int inlpos = hrg.lineIdx( trk.lineNr() );
	const int crlpos = hrg.trcIdx( trk.trcNr() );
	od_int64 ipos = trcarr_->info().getOffset( inlpos, crlpos, 0 );
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
    } while ( iter.next() );

    return true;
}


void Seis2DTo3D::smartScale()
{
    ArrayDivideExec adexec( trcarr_->totalSize(), geom_->getData(),
			       trcarr_->getData() );
    taskrun_->execute( adexec );
}


bool Seis2DTo3D::writeOutput()
{
    if ( !outioobj_ )
	mErrRet( mINTERNAL("outioobj_ is null") )

    delete storer_;
    storer_ = new Seis::Storer( *outioobj_ );
    if ( !storer_->isUsable() )
	mErrRet( storer_->errNotUsable() )

    if ( nrdone_ != 0 )
    {
	delete prov_;
	uiRetVal uirv;
	prov_ = Seis::Provider::create( *outioobj_, &uirv );
	if ( !prov_ )
	    mErrRet( uirv );
    }

    const TrcKeySampling& hrg = tkzs_.hsamp_;
    TrcKeySamplingIterator iter( hrg );
    SeisTrc& trc( *seisbuf_.get(0) );
    trc.info().setPos( hrg.start_ );
    trc.info().sampling_ = tkzs_.zsamp_;
    const int nrz = tkzs_.nrZ();

    do
    {
	const TrcKey trk( iter.curTrcKey() );
	const int inlpos = hrg.lineIdx( trk.lineNr() );
	const int crlpos = hrg.trcIdx( trk.trcNr() );

	uiRetVal uirv;
	if ( nrdone_ != 0 )
	{
	    uirv = prov_->getNext( trc );
	    if ( !uirv.isOK() )
	    {
		if ( isFinished(uirv) )
		    return true;

		mErrRet( uirv );
	    }
	}

	trc.info().setPos( trk.position() );
	for ( int idz=0; idz<nrz; idz++ )
	{
	    const float val = trcarr_->get(inlpos,crlpos,idz).real();
	    trc.set( idz, val, nrdone_ );
	}
	uirv = storer_->put( trc );
	if ( !uirv.isOK() )
	    mErrRet( uirv )

    } while ( iter.next() );

    return true;
}


od_int64 Seis2DTo3D::totalNr() const
{
    return totnr_;
}
