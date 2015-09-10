/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "seis2dto3d.h"

#include "arrayndalgo.h"
#include "bufstring.h"
#include "dataclipper.h"
#include "fftfilter.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
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

const char* Seis2DTo3D::sKeyInput()	{ return "Input ID"; }
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
    , wrr_(0)
    , rdr_(0)
    , tmpseisbuf_(true)
    , trcarr_(0)
    , butterfly_(0)
    , fft_(0)
    , geom_(0)
    , taperangle_(0)
    , pow_(2)
{}

Seis2DTo3D::~Seis2DTo3D()
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
    MultiID key;
    pars.get( sKeyInput(), key );
    inioobj_ = IOM().get( key );
    if ( !inioobj_ )
	mErrRet( tr("2DDataSet not found") )

    pars.get( SeisJobExecProv::sKeySeisOutIDKey(), key );
    outioobj_ = IOM().get( key );
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
	PtrMan<Executor> lf = ds.lineFetcher( ds.geomID(iline), tmpbuf );
	if ( !lf || !lf->execute() )
	    continue;

	for ( int idx=tmpbuf.size()-1; idx>=0; idx-- )
	{
	    const SeisTrc& intrc = *tmpbuf.get( idx );
	    if ( !tkzs_.hsamp_.includes(intrc.info().binid) )
		continue;

	    const BinID bid = intrc.info().binid;
	    SeisTrc* trc = new SeisTrc( intrc );
	    const int ns = tkzs_.zsamp_.nrSteps() + 1;
	    trc->reSize( ns, false );
	    trc->info().sampling.start = tkzs_.zsamp_.start;
	    trc->info().sampling.step = tkzs_.zsamp_.step;
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


void Seis2DTo3D::readInputCube(const int szfastx,
			       const int szfasty, const int szfastz )
{
    if(nrdone_ == 0)
    trcarr_ = new Array3DImpl<float_complex>(szfastx,szfasty,szfastz);
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
	const BinID bid = trc->info().binid;
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
}


#define mDoTransform(tf,isstraight,arr) \
{\
tf->setInputInfo( arr->info() );\
tf->setDir(isstraight);\
tf->setNormalization(!isstraight);\
tf->setInput(arr->getData());\
tf->setOutput(arr->getData());\
tf->run(true);\
}


int Seis2DTo3D::nextStep()
{
    if ( !read_ )
	return ErrorOccurred();

    delete fft_;
    fft_ = Fourier::CC::createDefault();

    const int szfastx = fft_->getFastSize( tkzs_.nrLines() );
    const int szfasty = fft_->getFastSize( tkzs_.nrTrcs() );
    const int szfastz = fft_->getFastSize( tkzs_.nrZ() );
    //careful here since in some cases the input might have a
    //different sampling rate than the selected output
    readInputCube( szfastx, szfasty, szfastz );

    if( nrdone_ == 0 )
    butterflyOperator();

    mDoTransform( fft_, true, trcarr_ );
    if ( nrdone_ == 0 )
    mDoTransform( fft_, true, butterfly_ );

    if (smartscaling_ && nrdone_ == 0)
    {
    mDoTransform( fft_, true, geom_ );
    multiplyArray( *butterfly_, *geom_ );
    mDoTransform( fft_, false, geom_ );
    }

    multiplyArray( *butterfly_, *trcarr_ );
    mDoTransform( fft_, false, trcarr_ );

    if (smartscaling_)
    {
    smartScale();
    }
    else
    {
    scaleArray();
    }

    writeOutput();
    nrdone_++;

    if (nrdone_ == totalNr() )
	return Finished();

    return MoreToDo();
}


void Seis2DTo3D::butterflyOperator()
{
    butterfly_ = new Array3DImpl<float_complex>( *trcarr_ );
    butterfly_->setAll( float_complex(0.f,0.f) );

    const double taperangle =  (double) taperangle_/180*M_PIf;
    const double pow = (double) pow_;
    const double mindist = SI().crlDistance();
    TrcKeySampling hrg( tkzs_.hsamp_ );
    const int nx = trcarr_->info().getSize(0);
    const int ny = trcarr_->info().getSize(1);
    const int nz = trcarr_->info().getSize(2);
    hrg.stop_ += BinID( nx - tkzs_.hsamp_.nrInl(), ny - tkzs_.hsamp_.nrCrl() );
    TrcKeySamplingIterator iter( hrg );
    BinID bid;

    //corner points of reference binids
    TypeSet<BinID> refbid;
    refbid += BinID( hrg.start_);
    refbid += BinID( hrg.start_.inl(), hrg.stop_.crl() + hrg.step_.crl());
    refbid += BinID( hrg.stop_.inl() + hrg.step_.inl(), hrg.start_.crl());
    refbid += BinID( hrg.stop_.inl() + hrg.step_.inl(),
		     hrg.stop_.crl() + hrg.step_.crl());

    int zdiff = nz - tkzs_.nrZ();
    Interval<double> refz( tkzs_.zsamp_.start,
			tkzs_.zsamp_.stop + (zdiff + 1) * tkzs_.zsamp_.step);
    const float zscale = SI().zDomain().isTime() ? 2500.f : 1.f;
    refz.scale( zscale );

    TypeSet<Coord> refpos2d;

    for ( int i=0; i<refbid.size(); i++)
    {
	refpos2d += hrg.toCoord( refbid[i] );
    }

    TypeSet<Coord3> refpos3d;
    for ( int i=0; i<refbid.size(); i++)
    {
	refpos3d += Coord3(refpos2d[i] ,refz.start);
	refpos3d += Coord3(refpos2d[i] ,refz.stop);
    }

    while (iter.next(bid))
    {
	Coord pos2d = tkzs_.hsamp_.toCoord( bid );
	const int idx = tkzs_.hsamp_.inlIdx( bid.inl() );
	const int idy = tkzs_.hsamp_.crlIdx( bid.crl() );

	for ( int idz=0; idz<nz; idz++)
	{
	    const Coord3 pos3d(pos2d, tkzs_.zsamp_.step*zscale*idz);

	    int nearestidx = 0;
	    double nearest = pos3d.distTo(refpos3d[0]);
	    for ( int i=1; i<refpos3d.size(); i++)
	    {
		double dist = pos3d.distTo(refpos3d[i]);
		if (dist < nearest)
		{
		    nearest = dist;
		    nearestidx = i;
		}
	    }
	    const double dist2d =
		pos2d.distTo(refpos3d[nearestidx].coord());

	    const double dist3d = pos3d.distTo(refpos3d[nearestidx]);
	    double topz = pos3d.z;
	    double bottomz = refz.stop-pos3d.z;

	    if (topz <= tan( taperangle )*dist2d ||
		bottomz <= tan( taperangle )*dist2d)
	    {
		const double valreal = mIsZero(dist3d,mDefEps) ?
			10 / (Math::PowerOf(mindist , pow)):
			(1 / (Math::PowerOf(dist3d, pow)));
		butterfly_->set(idx,idy,idz, (float) valreal);
	    }
	}
    }
}


void Seis2DTo3D::multiplyArray( const Array3DImpl<float_complex>& a,
				Array3DImpl<float_complex>& b )
{
    ArrayNDIter iter(b.info() );
    const int* itposidx = iter.getPos();

    do
    {
	float_complex val = a.getND(itposidx)*b.getND(itposidx);
	b.setND(itposidx,val);
    } while ( iter.next() );

}


bool Seis2DTo3D::scaleArray()
{
    Stats::CalcSetup rcsetup;
    rcsetup.require( Stats::RMS );
    Stats::RunCalc<float> runcalc( rcsetup);
    TrcKeySamplingIterator iter( tkzs_.hsamp_ );
    BinID bid;
    float_complex* data = trcarr_->getData();

    const int nz = tkzs_.nrZ();

    while ( iter.next(bid) )
    {
	const int idx = tkzs_.hsamp_.inlIdx( bid.inl() );
	const int idy = tkzs_.hsamp_.crlIdx( bid.crl() );
	od_uint64 ipos = trcarr_->info().getOffset( idx, idy, 0 );
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
    }
    return true;
}


void Seis2DTo3D::smartScale()
{
    TrcKeySamplingIterator iter( tkzs_.hsamp_ );
    BinID bid;
    float_complex* data = trcarr_->getData();
    float_complex* scaledata = geom_->getData();
    const int nz = tkzs_.nrZ();

    while ( iter.next(bid) )
    {
	const int idx = tkzs_.hsamp_.inlIdx( bid.inl() );
	const int idy = tkzs_.hsamp_.crlIdx( bid.crl() );
	od_uint64 ipos = trcarr_->info().getOffset( idx, idy, 0 );
	 for( int idz=0; idz<nz; idz++ )
	{
	    const float val = data[ipos+idz].real()/ scaledata[ipos+idz].real();
	    data[ipos+idz] = float_complex(val,0.f);
	}
    }
}


bool Seis2DTo3D::writeOutput()
{
    delete wrr_;
    wrr_ = new SeisTrcWriter( outioobj_ );

    if (nrdone_ != 0)
    {	delete rdr_;
	rdr_ = new SeisTrcReader( outioobj_ );
    }

    const TrcKeySampling& hrg = tkzs_.hsamp_;
    TrcKeySamplingIterator iter( hrg );
    BinID binid;
    SeisTrc& trc( *seisbuf_.get(0) );

    trc.info().sampling = tkzs_.zsamp_;
    trc.info().binid = binid;

    while(iter.next(binid))
    {
	const int idx = hrg.inlIdx(binid.inl());
	const int idy = hrg.crlIdx(binid.crl());

	if (nrdone_ != 0)
	{
	rdr_->get(trc);
	}

	for ( int idz=0; idz<tkzs_.nrZ(); idz++ )
	{
	    trc.info().binid = binid;
	    const float val = trcarr_->get(idx,idy,idz).real();
	    trc.set(idz,val,nrdone_);
	}
	if ( !wrr_->put(trc) )
	    mErrRet( uiStrings::phrCannotWrite( uiStrings::sTrace(mPlural)));
    }
    return true;
}


od_int64 Seis2DTo3D::totalNr() const
{
    return totnr_;
}
