/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Feb 2011
________________________________________________________________________

-*/


static const char* rcsID = "$Id: seis2dto3d.cc,v 1.3 2011-05-05 15:19:18 cvsbruno Exp $";

#include "seis2dto3d.h"

#include "arrayndutils.h"
#include "bufstring.h"
#include "scaler.h"
#include "dataclipper.h"
#include "ioobj.h"
#include "seis2dline.h"
#include "seisbuf.h"
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
    , nriter_(100)	
    , nrdone_(0)
    , ls_(0)
    , outioobj_(0)
    , winsz_(50)
    , tmpseisbuf_(true)
{}


Seis2DTo3D::~Seis2DTo3D()
{
    clear();
}


void Seis2DTo3D::clear()
{
    read_ = false;
    errmsg_.setEmpty();
    delete wrr_; wrr_ = 0;
    nriter_ = 1;
    totnr_ = -1; nrdone_ = 0;
}


#define mErrRet(msg) { errmsg_ = msg; return false; }
void Seis2DTo3D::setInput( const IOObj& obj, const char* attrnm )
{
    clear();

    ls_ = new Seis2DLineSet( obj ); 
    attrnm_ = attrnm;
}


void Seis2DTo3D::setNrIter( int nriter )
{
    nriter_ = nriter;
}

void Seis2DTo3D::setOutput( IOObj& obj, const CubeSampling& outcs )
{
    outioobj_ = &obj;
    cs_ = outcs;
    cs_.hrg.step = BinID( winsz_, winsz_ );
    hsit_.setSampling( cs_.hrg );
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
	if ( !inlrg.includes( bid.inl ) || !crlrg.includes( bid.crl ) )
	    { seisbuf_.remove( idx ); continue; }

	linecs.hrg.include( bid );
    }
    cs_.hrg.limitTo( linecs.hrg );

    StepInterval<float> si;
    const SeisTrc& trc = *seisbuf_.get( 0 );
    si.start = trc.info().sampling.start;
    si.step = trc.info().sampling.step;
    si.stop = trc.size()*trc.info().sampling.step + si.start;
    cs_.zrg = si;

    if ( cs_.totalNr() < 2 )
	{ errmsg_ = "Not enough positions found in input lineset"; }

    delete ls_;

    read_ = true;
    return true;
}


#define mInterpWinsz (int)(1.5*winsz_)
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

    const int inl = curbid_.inl; const int crl = curbid_.crl;
    Interval<int> inlrg( inl-mInterpWinsz, inl+mInterpWinsz );
    Interval<int> crlrg( crl-mInterpWinsz, crl+mInterpWinsz );
    inlrg.limitTo( SI().inlRange(true) );
    crlrg.limitTo( SI().crlRange(true) );
    HorSampling hrg; hrg.set( inlrg, crlrg );
    hrg.step = BinID( SI().inlRange(true).step, SI().crlRange(true).step );
    HorSamplingIterator localhsit( hrg );
    BinID binid;
    ObjectSet<const SeisTrc> trcs;
    while ( localhsit.next(binid) )
    {
	const int idtrc = seisbuf_.find( binid  );
	if ( idtrc >= 0 )
	    trcs += seisbuf_.get( idtrc ); 
    }
    interpol_.setInput( trcs, hrg );
    interpol_.setNrIter( nriter_ );
    if ( !trcs.isEmpty() && !interpol_.execute() )
	{ errmsg_ = interpol_.errMsg(); return ErrorOccurred(); }

    Interval<int> wininlrg( inl-winsz_, inl+winsz_);
    Interval<int> wincrlrg( crl-winsz_, crl+winsz_);
    wininlrg.limitTo( SI().inlRange(true) );
    wincrlrg.limitTo( SI().crlRange(true) );
    HorSampling winhrg; winhrg.set( wininlrg, wincrlrg );
    winhrg.step = BinID( SI().inlRange(true).step, SI().crlRange(true).step );
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
	tmpseisbuf_.add( outtrcs[idx] );

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
    , nriter_(100)
    , sc_(0)
    , nrdone_(0)
    , max_(0)
    , trcarr_(0)
{}


SeisInterpol::~SeisInterpol()
{
    clear();
    delete trcarr_;
    delete sc_;
}


void SeisInterpol::clear()
{
    errmsg_.setEmpty();
    nriter_ = 1;
    totnr_ = -1; nrdone_ = 0;
}


void SeisInterpol::setInput( const ObjectSet<const SeisTrc>& trcs, 
			    const HorSampling& hs )
{
    clear();

    inptrcs_ = &trcs;
    hs_ = hs;
}


void SeisInterpol::setNrIter( int nriter )
{
    nriter_ = nriter;
}


void SeisInterpol::doPrepare()
{
    fft_ = Fourier::CC::createDefault();
    szx_ = fft_->getFastSize( hs_.inlRange().nrSteps() ); 
    szy_ = fft_->getFastSize( hs_.crlRange().nrSteps() );
    szz_ = fft_->getFastSize( (*inptrcs_)[0]->size() );

    setUpData();
}


#define mDefThreshold ( (float)(nriter_-nrdone_-10)/ (float)nriter_ )
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
    if ( nrdone_ == nriter_ ) 
	{ return Executor::Finished(); }

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

    mDoTransform( fft_, true, trcarr_ );

#define mDoLoopWork( docomputemax )\
    for ( int idx=0; idx<szx_; idx++ )\
    {\
	for ( int idy=0; idy<szy_; idy++ )\
	{\
	    for ( int idz=0; idz<szz_; idz++ )\
	    {\
		const float real = trcarr_->get(idx,idy,idz).real();\
		const float imag = trcarr_->get(idx,idy,idz).imag();\
		const float mod = real*real + imag*imag;\
		if ( docomputemax )\
		{\
		    if ( mod > max_ )\
			max_ = mod;\
		}\
		else\
		{\
		    if ( mod < max_*mDefThreshold )\
			trcarr_->set(idx,idy,idz,0);\
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

/*
		    if ( ( idx>(int)(szx_/(float)20) && idx<=szx_-(int)(szx_/(float)100) ) || ( idy>(int)(szy_/(float)100) && idy<=szy_-(int)(szy_/(float)100) ) )\
			trcarr_->set(idx,idy,idz,0);\
		}\
*/


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

    HorSamplingIterator hsit( hs );
    BinID bid;
    while ( hsit.next( bid ) )
    {
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
	bool found = false;
	for ( int idtrc=0; idtrc<posidxs_.size(); idtrc++ )
	{
	    const TrcPosTrl& trpos = posidxs_[idtrc];
	    if ( trpos.trcpos_ >= inptrcs_->size() )
		continue; 
	    const BinID& trcbid = (*inptrcs_)[trpos.trcpos_]->info().binid;
	    if ( trcbid == bid )
		{ found = true; break; }
	}
	if ( !found )
	    sc_->scaleTrace( *trc );

	trcs += trc;
    }
}


void SeisInterpol::setUpData()
{
    int trcpos = -1;
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
    sc_ = new SeisScaler( *inptrcs_ );
}


#define mGetTrcRMSVal(tr,max,min)\
    Interval<float> rg;\
    DataClipper cl; cl.setApproxNrValues( tr.size() );\
    TypeSet<float> vals; \
    for ( int idx=0; idx<tr.size(); idx ++ )\
        vals += tr.get( idx, 0 );\
    cl.putData( vals.arr(), tr.size() );\
    cl.calculateRange( 0.05, rg );\
    max = rg.stop; min = rg.start;\

SeisScaler::SeisScaler( const ObjectSet<const SeisTrc>& trcs )
    : avgmaxval_(0)
    , avgminval_(0)
{
    for ( int idtrc=0; idtrc<trcs.size(); idtrc++ )
    {
	float maxval, minval;
	const SeisTrc& curtrc = *trcs[idtrc];
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

