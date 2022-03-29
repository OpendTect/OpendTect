/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2015
________________________________________________________________________

-*/

#include "seiscbvsimpfromothersurv.h"

#include "arrayndalgo.h"
#include "cbvsreadmgr.h"
#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "odcomplex.h"


SeisImpCBVSFromOtherSurvey::SeisImpCBVSFromOtherSurvey( const IOObj& inp )
    : Executor("Importing CBVS")
    , inioobj_(inp)
    , wrr_(0)
    , outioobj_(0)
    , nrdone_(0)
    , tr_(0)
    , fullusrexp_(0)
    , fft_(0)
    , arr_(0)
    , fftarr_(0)
    , taper_(0)
{
}


SeisImpCBVSFromOtherSurvey::~SeisImpCBVSFromOtherSurvey()
{
    deepErase( trcsset_ );
    delete tr_;
    delete wrr_;
    delete data_.hsit_;
    delete fft_;
    delete arr_;
    delete fftarr_;
    delete taper_;
}


#define mErrRet( str ) \
    { errmsg_ = str; return false; }

bool SeisImpCBVSFromOtherSurvey::prepareRead( const char* fulluserexp )
{
    if ( !createTranslators( fulluserexp ) )
	mErrRet( tr("Can not read cube") )

    const CBVSInfo& info = tr_->readMgr()->info();
    const Pos::IdxPair2Coord& b2c = tr_->getTransform();
    const CBVSInfo::SurvGeom& geom = info.geom_;
    olddata_.tkzs_.hsamp_.start_ = BinID( geom.start.inl(), geom.start.crl() );
    olddata_.tkzs_.hsamp_.stop_  = BinID( geom.stop.inl(), geom.stop.crl() );
    olddata_.tkzs_.hsamp_.step_  = BinID( geom.step.inl(), geom.step.crl() );
    data_.hsit_ = new TrcKeySamplingIterator( olddata_.tkzs_.hsamp_ );
    olddata_.tkzs_.zsamp_ = info.sd_.interval( info.nrsamples_ );
    data_.tkzs_.zsamp_ = olddata_.tkzs_.zsamp_;
    data_.tkzs_.zsamp_.step = SI().zStep();
    if ( hasSameGridAsThisSurvey() )
    {
	data_.tkzs_.hsamp_ = olddata_.tkzs_.hsamp_;
	data_.tkzs_.hsamp_.limitTo( SI().sampling(false).hsamp_ );
    }
    else
    {
	BinID bid;
	while ( data_.hsit_->next( bid ) )
	    data_.tkzs_.hsamp_.include( SI().transform(b2c.transform(bid)) );
    }

    if ( !SI().isInside(data_.tkzs_.hsamp_.start_,true)
	&& !SI().isInside(data_.tkzs_.hsamp_.stop_,true) )
	mErrRet(tr("The selected cube has no coordinates "
		   "matching the current survey.") )

    int step = olddata_.tkzs_.hsamp_.step_.inl();
    int padx = (int)( getInlXlnDist(b2c,true,step ) /SI().inlDistance() )+1;
    step = olddata_.tkzs_.hsamp_.step_.crl();
    int pady = (int)( getInlXlnDist(b2c,false,step) /SI().crlDistance() )+1;
    padfac_ = mMAX( padx, pady );

    return true;
}


bool SeisImpCBVSFromOtherSurvey::hasSameGridAsThisSurvey() const
{
    return tr && (tr_->getTransform() == SI().binID2Coord());
}


void SeisImpCBVSFromOtherSurvey::setPars( Interpol& interp, int cellsz,
					const TrcKeyZSampling& cs )
{
    interpol_ = interp;
    data_.tkzs_ = cs;
    data_.tkzs_.limitTo( SI().sampling(false) );
    data_.tkzs_.hsamp_.snapToSurvey();
    data_.hsit_->setSampling( data_.tkzs_.hsamp_ );
    totnr_ = mCast( int, data_.tkzs_.hsamp_.totalNr() );
    if ( !cellsz ) return;
    fft_ = Fourier::CC::createDefault();
    sz_ = fft_->getFastSize( cellsz );
    StepInterval<float> zsi( data_.tkzs_.zsamp_ );
    zsi.step = olddata_.tkzs_.zsamp_.step;
    szz_ = fft_->getFastSize( zsi.nrSteps() );
    arr_ = new Array3DImpl<float_complex>( sz_, sz_, szz_ );
    fftarr_ = new Array3DImpl<float_complex>( sz_, sz_, szz_ );
    newsz_ = fft_->getFastSize( sz_*padfac_ );
    taper_ = new ArrayNDWindow(Array1DInfoImpl(szz_),false,"CosTaper",0.95);
}


float SeisImpCBVSFromOtherSurvey::getInlXlnDist( const Pos::IdxPair2Coord& b2c,
						 bool inldir, int step ) const
{
    BinID orgbid = BinID( 0, 0 );
    BinID nextbid = BinID( inldir ? step : 0, inldir ? 0 : step );
    const Coord c00 = b2c.transform( orgbid );
    const Coord c10 = b2c.transform( nextbid );
    return (float) c00.distTo(c10);
}


bool SeisImpCBVSFromOtherSurvey::createTranslators( const char* fulluserexp )
{
    BufferString fnm( fulluserexp ? fulluserexp : inioobj_.fullUserExpr(true) );
    tr_ = CBVSSeisTrcTranslator::make( fnm, false, false, 0, true );
    return tr_ ? true : false;
}



int SeisImpCBVSFromOtherSurvey::nextStep()
{
    if ( !data_.hsit_->next(data_.curbid_) )
	return Finished();

    if ( !tr_ || !tr_->readMgr() )
	return ErrorOccurred();

    const Coord curcoord = SI().transform( data_.curbid_ );
    const Pos::IdxPair2Coord& b2c = tr_->getTransform();
    const BinID oldbid = b2c.transformBack( curcoord,
	olddata_.tkzs_.hsamp_.start_, olddata_.tkzs_.hsamp_.step_ );
    SeisTrc* outtrc = 0;
    if ( interpol_==Nearest || padfac_<=1 )
    {
	outtrc = readTrc( oldbid );
	if ( !outtrc )
	{
	    outtrc = new SeisTrc( data_.tkzs_.zsamp_.nrSteps() );
	    outtrc->zero();
	}

	outtrc->info().sampling = olddata_.tkzs_.zsamp_;
    }
    else
    {
	bool needgathertrcs = olddata_.curbid_ != oldbid || trcsset_.isEmpty();
	olddata_.curbid_ = oldbid;
	if ( needgathertrcs )
	{
	    if ( !findSquareTracesAroundCurbid( trcsset_ ) )
		{ nrdone_++; return MoreToDo(); }
	    sincInterpol( trcsset_ );
	}
	float mindist = mUdf( float );
	int outtrcidx = 0;
	for ( int idx=0; idx<trcsset_.size(); idx++ )
	{
	    const Coord trccoord = trcsset_[idx]->info().coord;
	    float dist = (float) trccoord.sqDistTo( curcoord );
	    if ( dist < mindist || mIsUdf( mindist ) )
	    {
		mindist = dist; outtrcidx = idx;
	    }
	}
	outtrc = new SeisTrc( *trcsset_[outtrcidx] );
    }
    outtrc->info().setPos( data_.curbid_ );

    if ( !wrr_ )
    {
	if ( !outioobj_ )
	{
	    errmsg_ = uiStrings::phrCannotOpenOutpFile();
	    return ErrorOccurred();
	}

	const Seis::GeomType gt = Seis::Vol;
	wrr_ = new SeisTrcWriter(*outioobj_,&gt);
    }

    if ( !wrr_->put(*outtrc) )
    {
	errmsg_ = wrr_->errMsg();
	delete outtrc;
	return ErrorOccurred();
    }

    delete outtrc;

    nrdone_ ++;
    return MoreToDo();
}


SeisTrc* SeisImpCBVSFromOtherSurvey::readTrc( const BinID& bid ) const
{
    SeisTrc* trc = 0;
    if ( tr_->goTo( bid )  )
    {
	trc = new SeisTrc;
	trc->info().setPos( bid );
	tr_->readInfo( trc->info() );
	tr_->read( *trc );
    }
    return trc;
}


bool SeisImpCBVSFromOtherSurvey::findSquareTracesAroundCurbid(
					    ObjectSet<SeisTrc>& trcs ) const
{
    deepErase( trcs );
    const int inlstep = olddata_.tkzs_.hsamp_.step_.inl();
    const int crlstep = olddata_.tkzs_.hsamp_.step_.crl();
    const int nrinltrcs = sz_*inlstep/2;
    const int nrcrltrcs = sz_*crlstep/2;
    for ( int idinl=-nrinltrcs; idinl<nrinltrcs; idinl+=inlstep)
    {
	for ( int idcrl=-nrcrltrcs; idcrl<nrcrltrcs; idcrl+=crlstep)
	{
	    BinID oldbid( olddata_.curbid_.inl() + idinl,
			  olddata_.curbid_.crl() + idcrl );
	    SeisTrc* trc = readTrc( oldbid );
	    if ( !trc || trc->isEmpty() )
		{ deepErase( trcs ); return false; }
	    trcs += trc;
	}
    }
    return !trcs.isEmpty();
}


/*!Sinc interpol( x ):
    x -> FFT(x) -> Zero Padd FFT -> iFFT -> y

Zero Padding in FFT domain ( 2D example )
			xx00xx
	xxxx	->	000000
	xxxx		000000
			xx00xx
!*/

#define mDoFFT(isforward,inp,outp,dim1,dim2,dim3)\
{\
    fft_->setInputInfo(Array3DInfoImpl(dim1,dim2,dim3));\
    fft_->setDir(isforward);\
    fft_->setNormalization(!isforward); \
    fft_->setInput(inp.getData());\
    fft_->setOutput(outp.getData());\
    fft_->run(true);\
}

void SeisImpCBVSFromOtherSurvey::sincInterpol( ObjectSet<SeisTrc>& trcs ) const
{
    if ( trcs.size() < 2 )
	return;

    int szx = sz_;	int newszx = newsz_;	 int xpadsz = (int)(szx/2);
    int szy = sz_;	int newszy = newsz_;	 int ypadsz = (int)(szy/2);

    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	for ( int idz=szz_; idz<trcs[idx]->size(); idz++ )
	    trcs[idx]->set( idz, 0, 0 );
    }

    int cpt =0;
    for ( int idx=0; idx<szx; idx ++ )
    {
	for ( int idy=0; idy<szy; idy++ )
	{
	    const SeisTrc& trc = *trcs[cpt]; cpt ++;
	    for ( int idz=0; idz<szz_; idz++ )
	    {
		const float val = idz < trc.size() ? trc.get(idz,0) : 0;
		arr_->set( idx, idy, idz, val );
	    }
	}
    }
    taper_->apply( arr_ );
    mDoFFT( true, (*arr_), (*fftarr_), szx, szy, szz_ )
    Array3DImpl<float_complex> padfftarr( newszx, newszy, szz_ );

#define mSetArrVal(xstart,ystart,xstop,ystop,xshift,yshift,z)\
    for ( int idx=xstart; idx<xstop; idx++)\
    {\
	for ( int idy=ystart; idy<ystop; idy++)\
	    padfftarr.set(idx+xshift,idy+yshift,z,fftarr_->get(idx,idy,idz));\
    }
#define mSetVals(zstart,zstop,zshift)\
    for ( int idz=zstart; idz<zstop; idz++ )\
    {\
	int newidz = zshift + idz;\
	mSetArrVal( 0, 0, xpadsz, ypadsz, 0, 0, newidz )\
	mSetArrVal( xpadsz, ypadsz, szx, szy, newszx-szx, newszy-szy, newidz )\
	mSetArrVal( xpadsz, 0, szx, ypadsz, newszx-szx, 0, newidz )\
	mSetArrVal( 0, ypadsz, xpadsz, szy, 0, newszy-szy, newidz )\
    }
    mSetVals( 0, szz_, 0 )
    mSetVals( szz_, szz_, 0 )

    Array3DImpl<float_complex> padarr( newszx, newszy, szz_ );
    mDoFFT( false, padfftarr, padarr, newszx, newszy, szz_ )

    const Coord startcrd = trcs[0]->info().coord;
    const Coord nextcrlcrd = trcs[1]->info().coord;
    const Coord nextinlcrd = trcs[sz_]->info().coord;
    const double xcrldist = (nextcrlcrd.x-startcrd.x)/padfac_;
    const double ycrldist = (nextcrlcrd.y-startcrd.y)/padfac_;
    const double xinldist = (nextinlcrd.x-startcrd.x)/padfac_;
    const double yinldist = (nextinlcrd.y-startcrd.y)/padfac_;
    const float amplfac = float(padfac_*padfac_);

    deepErase( trcs );
    for ( int idx=0; idx<newszx; idx ++ )
    {
	for ( int idy=0; idy<newszy; idy++ )
	{
	    SeisTrc* trc = new SeisTrc( szz_ );
	    trc->info().sampling = olddata_.tkzs_.zsamp_;
	    trc->info().coord.x = startcrd.x + idy*xcrldist + idx*xinldist;
	    trc->info().coord.y = startcrd.y + idy*ycrldist + idx*yinldist;
	    trcs += trc;
	    for ( int idz=0; idz<szz_; idz++ )
	    {
		if ( idz < trc->size() )
		    trc->set( idz, padarr.get(idx,idy,idz).real()*amplfac, 0 );
	    }
	}
    }
}
