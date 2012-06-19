/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
________________________________________________________________________

-*/


static const char* rcsID mUnusedVar = "$Id: seis2dto3d.cc,v 1.10 2012-06-19 10:19:25 cvsbruno Exp $";

#include "seis2dto3d.h"

#include "arrayndutils.h"
#include "bufstring.h"
#include "fftfilter.h"
#include "scaler.h"
#include "dataclipper.h"
#include "ioobj.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seisread.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "seiswrite.h"


Seis2DTo3D::Seis2DTo3D()
    : Executor("Generating 3D cube from LineSet")
    , seisbuf_(*new SeisTrcBuf(false))
    , wrr_(0)
    , cs_(false)
    , nrdone_(0)
    , ls_(0)
    , sc_(0)
    , outioobj_(0)
    , tmpseisbuf_(true)
{}


Seis2DTo3D::~Seis2DTo3D()
{
    clear();
}


void Seis2DTo3D::setIsNearestTrace( bool yn )
{
    nearesttrace_ = yn;
    if ( yn )
    cs_.hrg.step = BinID( SI().inlStep(), SI().crlStep() );
}


void Seis2DTo3D::clear()
{
    read_ = false;
    errmsg_.setEmpty();
    totnr_ = -1; nrdone_ = 0;
    maxvel_ = 0;
    reusetrcs_ = false;

    delete sc_; sc_ = 0;
    delete wrr_; wrr_ = 0;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
void Seis2DTo3D::setInput( const IOObj& obj, const char* attrnm )
{
    clear();

    ls_ = new Seis2DLineSet( obj ); 
    attrnm_ = attrnm;
}


void Seis2DTo3D::setParams( int inlstep, int crlstep, float maxvel, bool reuse )
{
    inlstep_ = inlstep;
    crlstep_ = crlstep;
    reusetrcs_ = reuse;
    maxvel_ = maxvel;
}


void Seis2DTo3D::setOutput( IOObj& obj, const CubeSampling& outcs )
{
    outioobj_ = &obj;
    cs_ = outcs;
    cs_.hrg.step = BinID( inlstep_, crlstep_ );
}


bool Seis2DTo3D::read()
{
    if ( ls_->nrLines() < 1 )
	mErrRet( "Empty LineSet" )
    BufferStringSet lnms;
    ls_->getLineNamesWithAttrib( lnms, attrnm_.buf() );
    if ( lnms.isEmpty() )
	mErrRet( "No lines for this attribute" )

    SeisTrcBuf tmpbuf(false);
    for ( int idx=0; idx<lnms.size(); idx++)
    {
	const LineKey lk( lnms.get( idx ), attrnm_.buf() );
	const int lsidx = ls_->indexOf( lk );
	if ( lsidx < 0 )
	    continue;
	tmpbuf.erase();
	Executor* lf = ls_->lineFetcher( lsidx, tmpbuf );
	lf->execute();
	seisbuf_.add( tmpbuf );
    }
    if ( seisbuf_.isEmpty() )
	mErrRet("No trace could be read")

    CubeSampling linecs( false );
    Interval<float> inlrg( cs_.hrg.inlRange().start, cs_.hrg.inlRange().stop );
    Interval<float> crlrg( cs_.hrg.crlRange().start, cs_.hrg.crlRange().stop );
    for ( int idx=0; idx<seisbuf_.size(); idx++ )
    {
	const SeisTrc& trc = *seisbuf_.get( idx );
	const BinID& bid = trc.info().binid; 
	if ( !inlrg.includes( bid.inl,false ) || !crlrg.includes( bid.crl,false ) )
	    { seisbuf_.remove( idx ); continue; }

	linecs.hrg.include( bid );
    }
    cs_.hrg.limitTo( linecs.hrg );
    hsit_.setSampling( cs_.hrg );

    StepInterval<float> si;
    const SeisTrc& trc = *seisbuf_.get( 0 );
    si.start = trc.info().sampling.start;
    si.step = trc.info().sampling.step;
    si.stop = trc.size()*trc.info().sampling.step + si.start;
    cs_.zrg = si;

    if ( cs_.totalNr() < 2 )
	{ errmsg_ = "Not enough positions found in input lineset"; }

    delete ls_;

    sc_ = new SeisScaler( seisbuf_ );

    read_ = true;
    return true;
}


#define mInterpInlWin (int)(2*inlstep_)
#define mInterpCrlWin (int)(2*crlstep_)

int Seis2DTo3D::nextStep()
{
    if ( !read_ && !read() )
	return ErrorOccurred();

    if ( !hsit_.next(curbid_) )
	{ writeTmpTrcs(); return Finished(); }

    if ( !SI().includes( curbid_, 0, true ) )
	return MoreToDo();

    if ( nrdone_ == 0 )
	prevbid_ = curbid_;

    if ( curbid_.inl != prevbid_.inl )
    {
	if ( !writeTmpTrcs() )
	    { errmsg_ = "Can not write trace"; return ErrorOccurred(); }
	prevbid_ = curbid_;
    }

    float mindist = mUdf(float);
    if ( nearesttrace_ )
    {
	const SeisTrc* nearesttrc = 0;
	for( int idx=0; idx<seisbuf_.size(); idx++ )
	{
	    const SeisTrc* trc = seisbuf_.get( idx );
	    BinID b = trc->info().binid;

	    if ( b == curbid_ )
	    {
		nearesttrc = trc;
		break;
	    }

	    int xx0 = b.inl-curbid_.inl;     xx0 *= xx0;
	    int yy0 = b.crl-curbid_.crl;     yy0 *= yy0;

	    if ( (  xx0 + yy0  ) < mindist || mIsUdf(mindist) )
	    {
		nearesttrc = trc;
		mindist = xx0 + yy0;
	    }
	}

	SeisTrc* newtrc = new SeisTrc( *nearesttrc );
	newtrc->info().binid = curbid_;
	tmpseisbuf_.add( newtrc );
    }
    else
    {
	const int inl = curbid_.inl; const int crl = curbid_.crl;
	Interval<int> inlrg( inl-mInterpInlWin/2, inl+mInterpInlWin/2 );
	Interval<int> crlrg( crl-mInterpCrlWin/2, crl+mInterpCrlWin/2 );
	inlrg.limitTo( SI().inlRange(true) );
	crlrg.limitTo( SI().crlRange(true) );
	HorSampling hrg; hrg.set( inlrg, crlrg );
	hrg.step = BinID( SI().inlRange(true).step, SI().crlRange(true).step );
	HorSamplingIterator localhsit( hrg );
	BinID binid;
	ObjectSet<const SeisTrc> trcs;
	SeisTrcReader rdr( outioobj_ );
	SeisTrcBuf outtrcbuf(false);
	SeisBufReader sbrdr( rdr, outtrcbuf );
	sbrdr.execute();
	while ( localhsit.next(binid) )
	{
	    const int idtrc = seisbuf_.find( binid  );
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
		const int outidtrc = outtrcbuf.find( binid  );
		if ( outidtrc >= 0 )
		    trcs += outtrcbuf.get( outidtrc );
	    }
	}
	interpol_.setInput( trcs );
	interpol_.setParams( hrg, maxvel_);
	if ( !trcs.isEmpty() && !interpol_.execute() )
	    { errmsg_ = interpol_.errMsg(); return ErrorOccurred(); }

	Interval<int> wininlrg( inl-inlstep_/2, inl+inlstep_/2);
	Interval<int> wincrlrg( crl-crlstep_/2, crl+crlstep_/2);
	wininlrg.limitTo( SI().inlRange(true) );
	wincrlrg.limitTo( SI().crlRange(true) );
	HorSampling winhrg; winhrg.set( wininlrg, wincrlrg );
	winhrg.step = BinID(SI().inlRange(true).step,SI().crlRange(true).step);
	ObjectSet<SeisTrc> outtrcs;
	interpol_.getOutTrcs( outtrcs, winhrg );

	if ( outtrcs.isEmpty() )
	{
	    BinID bid;
	    HorSamplingIterator hsit( winhrg );
	    while ( hsit.next( bid ) )
	    {
		SeisTrc* trc = new SeisTrc( seisbuf_.get( 0 )->size() );
		trc->info().sampling = seisbuf_.get( 0 )->info().sampling;
		trc->info().binid = bid; 
	    }
	}

	for ( int idx=0; idx<outtrcs.size(); idx ++ )
	{
	    sc_->scaleTrace( *outtrcs[idx] );
	    tmpseisbuf_.add( outtrcs[idx] );
	}
    } 

    nrdone_ ++;
    return MoreToDo();
}


bool Seis2DTo3D::writeTmpTrcs()
{ 
    if ( !wrr_ )
    {
	wrr_ = new SeisTrcWriter( outioobj_ );
    }

    tmpseisbuf_.sort( true, SeisTrcInfo::BinIDInl );
    int curinl = tmpseisbuf_.get( 0 )->info().binid.inl;
    int previnl = curinl;
    SeisTrcBuf tmpbuf(true); 
    while ( !tmpseisbuf_.isEmpty() )
    {
	SeisTrc* trc = tmpseisbuf_.remove(0);
	curinl = trc->info().binid.inl;
	if ( previnl != curinl )
	{
	    tmpbuf.sort( true, SeisTrcInfo::BinIDCrl );
	    int curcrl = tmpbuf.get(0)->info().binid.crl;
	    int prevcrl = -1; 
	    while( !tmpbuf.isEmpty() )
	    {
		const SeisTrc* crltrc = tmpbuf.remove(0);
		curcrl = crltrc->info().binid.crl;
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
    return cs_.hrg.totalNr();
}


SeisInterpol::SeisInterpol()
    : Executor("Interpolating")
    , hs_(false)
    , fft_(0)		
    , nrdone_(0)
    , max_(0)
    , trcarr_(0)
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


void SeisInterpol::setParams( const HorSampling& hs, float maxvel )
{
    hs_ = hs;
    maxvel_ = maxvel;
}


void SeisInterpol::doPrepare()
{
    delete fft_;
    fft_ = Fourier::CC::createDefault();

    const StepInterval<int>& inlrg = hs_.inlRange();
    const StepInterval<int>& crlrg = hs_.crlRange();
   
    const int hsszx = inlrg.nrSteps();
    const int hsszy = crlrg.nrSteps();

    szx_ = fft_->getFastSize( inlrg.nrSteps() ); 
    szy_ = fft_->getFastSize( crlrg.nrSteps() );
    szz_ = fft_->getFastSize( (*inptrcs_)[0]->size() );

    const int diffszx = szx_ - hsszx; 
    const int diffszy = szy_ - hsszy;

    hs_.setInlRange(Interval<int>(inlrg.start-diffszx/2,inlrg.stop+diffszx/2));
    hs_.setCrlRange(Interval<int>(crlrg.start-diffszy/2,crlrg.stop+diffszy/2));

    setUpData();
}


#define mDefThreshold ( (float)(nriter_-nrdone_-1)/ (float)nriter_ )
#define mDoTransform(tf,isstraight,arr) \
{\
    tf->setInputInfo( arr->info() );\
    tf->setDir(isstraight);\
    tf->setNormalization(!isstraight);\
    tf->setInput(arr->getData());\
    tf->setOutput(arr->getData());\
    tf->run(true);\
}

int SeisInterpol::nextStep()
{
    if ( nrdone_ == 0 )
	doPrepare();
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
	{ return Executor::Finished(); }

    mDoTransform( fft_, true, trcarr_ );
    const float df = Fourier::CC::getDf( SI().zStep(), szz_ );
    const float mindist = mMIN(SI().inlDistance(),SI().crlDistance() );
    const float fmax = maxvel_ / ( 2*mindist*sin( M_PI/6 ) );
    const int poscutfreq = (int)(fmax/df);

#define mDoLoopWork( docomputemax )\
    for ( int idx=0; idx<szx_; idx++ )\
    {\
	for ( int idy=0; idy<szy_; idy++ )\
	{\
	    for ( int idz=0; idz<szz_; idz++ )\
	    {\
		float real = trcarr_->get(idx,idy,idz).real();\
		float imag = trcarr_->get(idx,idy,idz).imag();\
		float xfac; float yfac; float zfac;\
		xfac = yfac = zfac = 0;\
		if ( idz < poscutfreq || idz > szz_-poscutfreq )\
		    zfac = 1;\
		float dipangle; float revdipangle;\
		dipangle = revdipangle = 0;\
		if ( idx < szx_/2 )\
		{\
		    dipangle = atan( idy/(float)idx );\
		    revdipangle = atan( (szy_-idy-1)/(float)(idx) );\
		}\
		else\
		{\
		    dipangle = atan( idy/(float)(szx_-idx-1) );\
		    revdipangle = atan( (szy_-idy-1)/(float)(szx_-idx-1) );\
		}\
		if ( dipangle > M_PI/4 && revdipangle > M_PI/4 )\
		    { xfac = yfac = 1; }\
		real *= xfac*yfac*zfac; imag *= xfac*yfac*zfac;\
		float mod = real*real + imag*imag;\
		if ( docomputemax )\
		{\
		    mod = real*real + imag*imag;\
		    if ( mod > max_ )\
			max_ = mod;\
		}\
		else\
		{\
		    if ( mod < max_*mDefThreshold )\
			{ real = imag = 0; }\
		    trcarr_->set(idx,idy,idz,float_complex(real,imag));\
		}\
	    }\
	}\
    }

    if ( nrdone_ == 0 )
	{ mDoLoopWork( true ) }

    mDoLoopWork( false )
    mDoTransform( fft_, false, trcarr_ );
    
    nrdone_++;
    return Executor::MoreToDo();
}


const BinID SeisInterpol::convertToBID( int idx, int idy ) const
{
    return BinID( hs_.inlRange().atIndex(idx), hs_.crlRange().atIndex(idy) );
}


void SeisInterpol::convertToPos( const BinID& bid, int& idx, int& idy ) const
{
    idx = hs_.inlRange().getIndex( bid.inl );
    idy = hs_.crlRange().getIndex( bid.crl );
}


int SeisInterpol::getTrcInSet( const BinID& bin ) const
{
    for ( int idx=0; idx<inptrcs_->size(); idx++ )
    {
	const SeisTrc* trc = (*inptrcs_)[idx];
	if ( trc->info().binid == bin )
	   return idx; 
    }
    return -1;
}


void SeisInterpol::getOutTrcs( ObjectSet<SeisTrc>& trcs, 
				const HorSampling& hs) const
{
    if ( inptrcs_->isEmpty() )
	return;

    HorSamplingIterator hsit( hs_ );
    BinID bid;
    while ( hsit.next( bid ) )
    {
	if ( !hs.includes( bid ) ) 
	    continue;

	int idx = -1; int idy = -1;
	convertToPos( bid, idx, idy );
	if ( idx < 0 || idy < 0 || szx_ <= idx || szy_ <= idy) continue;
	
	SeisTrc* trc = new SeisTrc( szz_ );
	trc->info().sampling = (*inptrcs_)[0]->info().sampling;
	trc->info().binid = bid; 
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
	    const BinID& bid = convertToBID( idx, idy );
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
    max = rg.stop; min = rg.start;\

SeisScaler::SeisScaler( const SeisTrcBuf& trcs )
    : avgmaxval_(0)
    , avgminval_(0)
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


void SeisScaler::scaleTrace( SeisTrc& trc )
{
    const Coord& crd = trc.info().coord;
    float trcmaxval, trcminval;
    mGetTrcRMSVal( trc, trcmaxval, trcminval )
    LinScaler sc( trcminval, avgminval_, trcmaxval, avgmaxval_ );
    for ( int idz=0; idz<trc.size(); idz++ )
    {
	float val = trc.get( idz, 0 );
	val = sc.scale( val );
	trc.set( idz, val, 0 );
    }
}

