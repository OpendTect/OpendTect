/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seis2dto3d.h"

#include "dataclipper.h"
#include "fftfilter.h"
#include "ioman.h"
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
#include "survgeom3d.h"
#include "survinfo.h"



const char* Seis2DTo3D::sKeyInput()	{ return "Input ID"; }
const char* Seis2DTo3D::sKeyIsNearest() { return "Take Nearest"; }
const char* Seis2DTo3D::sKeyStepout()	{ return "Inl-Crl Stepout"; }
const char* Seis2DTo3D::sKeyReUse()	{ return "Re-use existing"; }
const char* Seis2DTo3D::sKeyMaxVel()	{ return "Maximum Velocity"; }
const char* Seis2DTo3D::sKeyCreaterType() { return "Creater Type"; }



Seis2DTo3D::Seis2DTo3D()
    : Executor("Generating a 3D cube from a 2DDataSet")
    , tkzs_(true)
    , seisbuf_(*new SeisTrcBuf(true))
    , tmpseisbuf_(true)
{}


Seis2DTo3D::~Seis2DTo3D()
{
    seisbuf_.erase();
    delete wrr_;
    delete sc_;
    delete inioobj_;
    delete outioobj_;
}


bool Seis2DTo3D::init( const IOPar& pars )
{
    return usePar(pars);
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
bool Seis2DTo3D::usePar( const IOPar& pars )
{
    if ( !setIO(pars) )
	return false;

    PtrMan<IOPar> parampars = pars.subselect( sKey::Pars() );
    if ( !parampars )
	mErrRet( tr("No processing parameters found") )

    parampars->getYN( sKeyIsNearest(), nearesttrace_ );
    if ( !nearesttrace_ )
    {
	Interval<int> step;
	parampars->get( sKeyStepout(), step );
        inlstep_ = step.start_;
        crlstep_ = step.stop_;
	parampars->getYN( sKeyReUse(), reusetrcs_ );
	parampars->get( sKeyMaxVel(), maxvel_ );
	tkzs_.hsamp_.step_ = BinID( inlstep_, crlstep_ );
    }

    return true;
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
    if ( !subsel )
	return false;

    PtrMan<IOPar> sampling = subsel->subselect( sKey::Subsel() );
    if ( !sampling )
	mErrRet( tr("No volume processing area found") )

    tkzs_.usePar( *sampling );

    return true;
}


bool Seis2DTo3D::checkParameters()
{
    if ( nearesttrace_ )
	return true;

    if ( inlstep_ < 1 && crlstep_ < 1 )
    {
	uiString msg = tr("Internal: %1 step is not set")
		     .arg(inlstep_ < 1 ? sKey::Inline() : sKey::Crossline());
	mErrRet(msg)
    }

    if ( mIsUdf(maxvel_) )
	mErrRet( tr("Internal: Maximum velocity is not set") )

    return true;
}


bool Seis2DTo3D::read()
{
    const SeisIOObjInfo seisinfo( inioobj_ );
    if ( !seisinfo.isOK() || !seisinfo.is2D() )
	return false;

    TypeSet<Pos::GeomID> gids;
    seisinfo.getGeomIDs( gids );
    if ( gids.isEmpty() )
	mErrRet( tr("Input dataset has no lines") )

    const TrcKeySampling& tks = tkzs_.hsamp_;
    const Interval<int> inlrg( tks.inlRange().start_ - inlstep_,
                               tks.inlRange().stop_ + inlstep_ );
    const Interval<int> crlrg( tks.crlRange().start_ - crlstep_,
                               tks.crlRange().stop_ + crlstep_ );
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

	    const BinID bid = geom3d->transform( inptrc.info().coord_ );
	    if ( !inlrg.includes(bid.inl(),false) ||
		 !crlrg.includes(bid.crl(),false) ||
		 !rdr.get(inptrc) )
		continue;

	    auto* trc = new SeisTrc( inptrc );
	    trc->reSize( ns, false );
	    trc->info().setPos( bid );
	    trc->info().calcCoord();
	    trc->info().sampling_.start_ = tkzs_.zsamp_.start_;
	    trc->info().sampling_.step_ = tkzs_.zsamp_.step_;
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

    hsit_.setSampling( tks );

    if ( !nearesttrace_ )
	sc_ = new SeisScaler( seisbuf_ );

    read_ = true;
    return true;
}


int Seis2DTo3D::nextStep()
{
    if ( !read_ && !read() )
	return ErrorOccurred();

    if ( !hsit_.next(curbid_) )
    {
	writeTmpTrcs();
	return Finished();
    }

    if ( !SI().sampling(false).hsamp_.includes(curbid_) )
	return MoreToDo();

    if ( nrdone_ == 0 )
	prevbid_ = curbid_;

    if ( curbid_.inl() != prevbid_.inl() )
    {
	if ( !writeTmpTrcs() )
	{
	    errmsg_ = tr( "Can not write trace" );
	    return ErrorOccurred();
	}

	prevbid_ = curbid_;
    }

    if ( nearesttrace_ )
	doWorkNearest();
    else
    {
	if ( !doWorkFFT() )
	    return ErrorOccurred();
    }

    nrdone_++;
    return MoreToDo();
}


void Seis2DTo3D::doWorkNearest()
{
    if ( seisbuf_.isEmpty() ) return;
    od_int64 mindist = mUdf(od_int64);
    const SeisTrc* nearesttrc = 0;
    for( int idx=0; idx<seisbuf_.size(); idx++ )
    {
	const SeisTrc* trc = seisbuf_.get( idx );
	const BinID bid = trc->info().binID();

	if ( bid == curbid_ )
	{
	    nearesttrc = trc;
	    break;
	}

	int xx0 = bid.inl()-curbid_.inl();
	xx0 *= xx0;
	int yy0 = bid.crl()-curbid_.crl();
	yy0 *= yy0;

	if ( (xx0 + yy0)<mindist || mIsUdf(mindist) )
	{
	    nearesttrc = trc;
	    mindist = xx0 + yy0;
	}
    }

    if ( !nearesttrc )
	return;

    auto* newtrc = new SeisTrc( *nearesttrc );
    newtrc->info().setPos( curbid_ );
    tmpseisbuf_.add( newtrc );
}


bool Seis2DTo3D::doWorkFFT()
{
    const int inl = curbid_.inl(); const int crl = curbid_.crl();
    Interval<int> inlrg( inl-inlstep_, inl+inlstep_ );
    Interval<int> crlrg( crl-crlstep_, crl+crlstep_ );
    inlrg.limitTo( SI().inlRange(true) );
    crlrg.limitTo( SI().crlRange(true) );
    TrcKeySampling hrg; hrg.set( inlrg, crlrg );
    hrg.step_ = BinID( SI().inlRange(true).step_, SI().crlRange(true).step_ );
    TrcKeySamplingIterator localhsit( hrg );
    BinID binid;
    ObjectSet<const SeisTrc> trcs;
    if ( !outioobj_ )
	mErrRet(toUiString("Internal: No output is set"))

    SeisTrcBuf outtrcbuf(false);
    if ( reusetrcs_ )
    {
	const Seis::GeomType gt = Seis::Vol;
	SeisTrcReader rdr( *outioobj_, &gt );
	SeisBufReader sbrdr( rdr, outtrcbuf );
	sbrdr.execute();
    }

    while ( localhsit.next(binid) )
    {
	const bool hasbid = seisbuftks_.includes( binid );
	const int idtrc = hasbid ? seisbuf_.find(binid) : -1;
	if ( idtrc >= 0 )
	    trcs += seisbuf_.get( idtrc );
	else if ( reusetrcs_ && !tmpseisbuf_.isEmpty() )
	{
	    const int idinterptrc = tmpseisbuf_.find( binid  );
	    if ( idinterptrc >= 0 )
		trcs += tmpseisbuf_.get( idinterptrc );
	}
	else if ( reusetrcs_ && !outtrcbuf.isEmpty() )
	{
	    const int outidtrc = outtrcbuf.find( binid	);
	    if ( outidtrc >= 0 )
		trcs += outtrcbuf.get( outidtrc );
	}
    }
    interpol_.setInput( trcs );
    interpol_.setParams( hrg, maxvel_);
    if ( !trcs.isEmpty() && !interpol_.execute() )
    { errmsg_ = interpol_.uiMessage(); return false; }

    Interval<int> wininlrg( inl-inlstep_/2, inl+inlstep_/2);
    Interval<int> wincrlrg( crl-crlstep_/2, crl+crlstep_/2);
    wininlrg.limitTo( SI().inlRange(true) );
    wincrlrg.limitTo( SI().crlRange(true) );
    TrcKeySampling winhrg;
    winhrg.set( wininlrg, wincrlrg );
    winhrg.step_ = BinID(SI().inlRange(true).step_,SI().crlRange(true).step_);
    ObjectSet<SeisTrc> outtrcs;
    interpol_.getOutTrcs( outtrcs, winhrg );

    for ( int idx=0; sc_ && idx<outtrcs.size(); idx ++ )
	sc_->scaleTrace( *outtrcs[idx] );

    if ( reusetrcs_ && outtrcs.isEmpty() )
    {
	BinID bid;
	TrcKeySamplingIterator hsit( winhrg );
	while ( hsit.next(bid) && seisbuf_.get(0) )
	{
	    auto* trc = new SeisTrc( seisbuf_.get(0)->size() );
	    trc->info().sampling_ = seisbuf_.get(0)->info().sampling_;
	    trc->info().setPos( bid );
	    outtrcs += trc;
	}
    }

    for ( int idx=0; idx<outtrcs.size(); idx ++ )
	tmpseisbuf_.add( outtrcs[idx] );

    return true;
}


bool Seis2DTo3D::writeTmpTrcs()
{
    if ( tmpseisbuf_.isEmpty() )
	return true;

    if ( !wrr_ )
    {
	const Seis::GeomType gt = Seis::Vol;
	wrr_ = new SeisTrcWriter( *outioobj_, &gt );
    }

    tmpseisbuf_.sort( true, SeisTrcInfo::BinIDInl );
    int curinl = tmpseisbuf_.get( 0 )->info().inl();
    int previnl = curinl;
    SeisTrcBuf tmpbuf(true);
    bool isbufempty = false;
    while ( !tmpseisbuf_.isEmpty() )
    {
	SeisTrc* trc = tmpseisbuf_.remove(0);
	curinl = trc->info().inl();
	isbufempty = tmpseisbuf_.isEmpty();
	if ( previnl != curinl || isbufempty )
	{
	    tmpbuf.sort( true, SeisTrcInfo::BinIDCrl );
	    int prevcrl = -1;
	    while( !tmpbuf.isEmpty() )
	    {
		const SeisTrc* crltrc = tmpbuf.remove(0);
		const int curcrl = crltrc->info().crl();
		if ( curcrl != prevcrl )
		{
		    if ( !wrr_->put( *crltrc ) )
			return false;
		}
		prevcrl = curcrl;
		delete crltrc;
	    }
	    previnl = curinl;
	}
	tmpbuf.add( trc );
    }
    return true;
}


od_int64 Seis2DTo3D::totalNr() const
{
    return tkzs_.hsamp_.totalNr();
}



// SeisInterpol
SeisInterpol::SeisInterpol()
    : Executor("Interpolating")
    , hs_(false)
{}


SeisInterpol::~SeisInterpol()
{
    clear();
    delete trcarr_;
}


void SeisInterpol::clear()
{
    errmsg_.setEmpty();
    nriter_ = 10;
    totnr_ = -1;
    nrdone_ = 0;
    szx_ = szy_ = szz_ = 0;
    max_ = 0;
    maxvel_ = 0;
    posidxs_.erase();
}


void SeisInterpol::setInput( const ObjectSet<const SeisTrc>& trcs )
{
    clear();
    inptrcs_ = &trcs;
}


void SeisInterpol::setParams( const TrcKeySampling& hs, float maxvel )
{
    hs_ = hs;
    maxvel_ = maxvel;
}


bool SeisInterpol::doPrepare( od_ostream* )
{
    delete fft_;
    fft_ = Fourier::CC::createDefault();

    const StepInterval<int>& inlrg = hs_.inlRange();
    const StepInterval<int>& crlrg = hs_.crlRange();

    const int hsszx = inlrg.nrSteps() + 1;
    const int hsszy = crlrg.nrSteps() + 1;

    szx_ = fft_->getFastSize( hsszx );
    szy_ = fft_->getFastSize( hsszy );
    szz_ = fft_->getFastSize( (*inptrcs_)[0]->size() );

    const int diffszx = szx_ - hsszx;
    const int diffszy = szy_ - hsszy;

    hs_.setInlRange(
		Interval<int>(inlrg.start_-diffszx/2,inlrg.stop_+diffszx/2) );
    hs_.setCrlRange(
		Interval<int>(crlrg.start_-diffszy/2,crlrg.stop_+diffszy/2) );

    setUpData();
    return true;
}


void SeisInterpol::doWork( bool docomputemax, int poscutfreq )
{
    const float threshold = (float)(nriter_-nrdone_-1)/ (float)nriter_;
    for ( int idx=0; idx<szx_; idx++ )
    {
	for ( int idy=0; idy<szy_; idy++ )
	{
	    for ( int idz=0; idz<szz_; idz++ )
	    {
		float real = trcarr_->get(idx,idy,idz).real();
		float imag = trcarr_->get(idx,idy,idz).imag();
		float xfac; float yfac; float zfac;
		xfac = yfac = zfac = 0;
		if ( idz < poscutfreq || idz > szz_-poscutfreq )
		    zfac = 1;

		float dipangle; float revdipangle;
		dipangle = revdipangle = 0;
		if ( idx < szx_/2 )
		{
		    dipangle = atan( idy/(float)idx );
		    revdipangle = atan( (szy_-idy-1)/(float)(idx) );
		}
		else
		{
		    dipangle = atan( idy/(float)(szx_-idx-1) );
		    revdipangle = atan( (szy_-idy-1)/(float)(szx_-idx-1) );
		}

		if ( dipangle > M_PI_4f && revdipangle > M_PI_4f )
		    { xfac = yfac = 1; }

		real *= xfac*yfac*zfac; imag *= xfac*yfac*zfac;
		float mod = real*real + imag*imag;
		if ( docomputemax )
		{
		    mod = real*real + imag*imag;
		    if ( mod > max_ )
			max_ = mod;
		}
		else
		{
		    if ( mod < max_*threshold )
			{ real = imag = 0; }
		    trcarr_->set(idx,idy,idz,float_complex(real,imag));
		}
	    }
	}
    }
}


int SeisInterpol::nextStep()
{
    for ( int idtrc=0; idtrc<posidxs_.size(); idtrc++ )
    {
	TrcPosTrl& trpos = posidxs_[idtrc];
	if ( trpos.trcpos_ >= inptrcs_->size() )
	    continue;

	for ( int idz=0; idz<szz_; idz++ )
	{
	    float val = 0;
	    if ( idz < (*inptrcs_)[0]->size() )
		val = (*inptrcs_)[trpos.trcpos_]->get(idz,0);
	    trcarr_->set( trpos.idx_, trpos.idy_, idz, val );
	}
    }

    if ( nrdone_ == nriter_ )
	return Executor::Finished();

    fft_->setInputInfo( trcarr_->info() );
    fft_->setDir( true );
    fft_->setNormalization( false );
    fft_->setInput( trcarr_->getData() );
    fft_->setOutput( trcarr_->getData() );
    fft_->run( true );

    const float df = Fourier::CC::getDf( SI().zStep(), szz_ );
    const float mindist = mMIN(SI().inlDistance(),SI().crlDistance() );
    const float fmax = mCast(float, maxvel_ / ( 2.f*mindist*sin( M_PIf/6.f ) ));
    const int poscutfreq = mCast(int, fmax/df );

    if ( nrdone_ == 0 )
	doWork( true, poscutfreq );

    doWork( false, poscutfreq );

    fft_->setInputInfo( trcarr_->info() );
    fft_->setDir( false );
    fft_->setNormalization( true );
    fft_->setInput( trcarr_->getData() );
    fft_->setOutput( trcarr_->getData() );
    fft_->run( true );

    nrdone_++;
    return Executor::MoreToDo();
}


const BinID SeisInterpol::convertToBID( int idx, int idy ) const
{
    return BinID( hs_.inlRange().atIndex(idx), hs_.crlRange().atIndex(idy) );
}


void SeisInterpol::convertToPos( const BinID& bid, int& idx, int& idy ) const
{
    idx = hs_.inlRange().getIndex( bid.inl() );
    idy = hs_.crlRange().getIndex( bid.crl() );
}


int SeisInterpol::getTrcInSet( const BinID& bin ) const
{
    for ( int idx=0; idx<inptrcs_->size(); idx++ )
    {
	const SeisTrc* trc = (*inptrcs_)[idx];
	if ( trc->info().binID() == bin )
	   return idx;
    }
    return -1;
}


void SeisInterpol::getOutTrcs( ObjectSet<SeisTrc>& trcs,
				const TrcKeySampling& hs) const
{
    if ( inptrcs_->isEmpty() )
	return;

    TrcKeySamplingIterator hsit( hs_ );
    BinID bid;
    while ( hsit.next( bid ) )
    {
	if ( !hs.includes(bid) )
	    continue;

	int idx = -1; int idy = -1;
	convertToPos( bid, idx, idy );
	if ( idx < 0 || idy < 0 || szx_ <= idx || szy_ <= idy) continue;

	auto* trc = new SeisTrc( szz_ );
	trc->info().sampling_ = (*inptrcs_)[0]->info().sampling_;
	trc->info().setPos( bid );
	for ( int idz=0; idz<szz_; idz++ )
	{
	    float val = trcarr_->get( idx, idy, idz ).real();
	    if ( mIsUdf(val) ) val = 0;
	    trc->set( idz, val, 0 );
	}
	trcs += trc;
    }
}


void SeisInterpol::setUpData()
{
    if ( !trcarr_)
	trcarr_ = new Array3DImpl<float_complex>( szx_, szy_, szz_ );
    else
	trcarr_->setSize( szx_, szy_, szz_ );
    trcarr_->setAll( 0 );

    for ( int idx=0; idx<szx_; idx++ )
    {
	for ( int idy=0; idy<szy_; idy++ )
	{
	    const BinID bid = convertToBID( idx, idy );
	    const int trcidx = getTrcInSet( bid );
	    if ( trcidx >= 0 )
		posidxs_ += TrcPosTrl( idx, idy, trcidx );
	}
    }
}


#define mGetTrcRMSVal(tr,max,min)\
    Interval<float> rg;\
    DataClipper cl; cl.setApproxNrValues( tr.size() );\
    TypeSet<float> vals; \
    for ( int idx=0; idx<tr.size(); idx ++ )\
        vals += tr.get( idx, 0 );\
    cl.putData( vals.arr(), tr.size() );\
    cl.calculateRange( 0.1, rg );\
    max = rg.stop_; min = rg.start_;\


SeisScaler::SeisScaler()
{}


SeisScaler::SeisScaler( const SeisTrcBuf& trcs )
{
    for ( int idtrc=0; idtrc<trcs.size(); idtrc++ )
    {
	float maxval, minval;
	const SeisTrc& curtrc = *trcs.get( idtrc );
	mGetTrcRMSVal( curtrc, maxval, minval )
	avgmaxval_ += maxval / trcs.size();
	avgminval_ += minval / trcs.size();
    }
}


SeisScaler::~SeisScaler()
{}


void SeisScaler::scaleTrace( SeisTrc& trc )
{
    float trcmaxval, trcminval;
    mGetTrcRMSVal( trc, trcmaxval, trcminval )
    LinScaler sc( trcminval, avgminval_, trcmaxval, avgmaxval_ );
    for ( int idz=0; idz<trc.size(); idz++ )
    {
	float val = (float) trc.get( idz, 0 );
	val = (float) sc.scale( val );
	trc.set( idz, val, 0 );
    }
}
