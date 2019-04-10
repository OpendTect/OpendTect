/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2015
________________________________________________________________________

-*/

#include "seiscubeimpfromothersurv.h"

#include "arrayndalgo.h"
#include "cbvsreadmgr.h"
#include "seisblocksreader.h"
#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "seisstorer.h"
#include "survinfo.h"
#include "survgeom3d.h"
#include "odcomplex.h"


SeisCubeImpFromOtherSurvey::SeisCubeImpFromOtherSurvey( const IOObj& inp )
    : Executor("Importing Seismic Data")
    , inioobj_(inp)
    , storer_(0)
    , outioobj_(0)
    , nrdone_(0)
    , rdr_(0)
    , cbvstr_(0)
    , fullusrexp_(0)
    , fft_(0)
    , arr_(0)
    , fftarr_(0)
    , taper_(0)
{
}


SeisCubeImpFromOtherSurvey::~SeisCubeImpFromOtherSurvey()
{
    deepErase( trcsset_ );
    delete rdr_;
    delete cbvstr_;
    delete storer_;
    delete data_.hsit_;
    delete fft_;
    delete arr_;
    delete fftarr_;
    delete taper_;
    delete outioobj_;
}


static bool isCBVS( const char* fnm )
{
    const BufferString ext( File::Path(fnm).extension() );
    return ext == "cbvs";
}


void SeisCubeImpFromOtherSurvey::setOutput( const IOObj& ioobj )
{
    delete outioobj_;
    outioobj_ = ioobj.clone();
}


#define mErrRet( str ) \
    { uirv_ = str; return false; }

bool SeisCubeImpFromOtherSurvey::prepareRead( const char* mainfilenm )
{
    if ( !createReader( mainfilenm ) )
	return false;

    Pos::IdxPair2Coord b2c;
    if ( cbvstr_ )
    {
	const CBVSInfo& info = cbvstr_->readMgr()->info();
	b2c = cbvstr_->getTransform();
	const CBVSInfo::SurvGeom& geom = info.geom_;
	olddata_.tkzs_.hsamp_.start_ = BinID(geom.start.inl(),geom.start.crl());
	olddata_.tkzs_.hsamp_.stop_  = BinID(geom.stop.inl(),geom.stop.crl());
	olddata_.tkzs_.hsamp_.step_  = BinID(geom.step.inl(),geom.step.crl());
	olddata_.tkzs_.zsamp_ = info.sd_.interval( info.nrsamples_ );
    }
    else
    {
	b2c = rdr_->hGeom().binID2Coord();
	Interval<int> inlrg, crlrg;
	rdr_->positions().getRanges( inlrg, crlrg );
	olddata_.tkzs_.hsamp_.start_ = BinID( inlrg.start, crlrg.start );
	olddata_.tkzs_.hsamp_.stop_ = BinID( inlrg.stop, crlrg.stop );
	olddata_.tkzs_.hsamp_.step_ = BinID( rdr_->hGeom().inlRange().step,
					     rdr_->hGeom().crlRange().step );
	olddata_.tkzs_.zsamp_ = rdr_->zGeom();
    }

    data_.hsit_ = new TrcKeySamplingIterator( olddata_.tkzs_.hsamp_ );
    data_.tkzs_.zsamp_ = olddata_.tkzs_.zsamp_;
    data_.tkzs_.zsamp_.step = SI().zStep();

    do
    {
	const BinID bid( data_.hsit_->curBinID() );
	data_.tkzs_.hsamp_.include( SI().transform( b2c.transform( bid ) ) );
    } while ( data_.hsit_->next() );

    if ( !SI().includes(data_.tkzs_.hsamp_.start_)
	&& !SI().includes(data_.tkzs_.hsamp_.stop_) )
	mErrRet(tr("The selected cube has no coordinates "
		   "matching the current survey.") )

    int step = olddata_.tkzs_.hsamp_.step_.inl();
    int padx = (int)( getInlXlnDist(b2c,true,step ) /SI().inlDistance() )+1;
    step = olddata_.tkzs_.hsamp_.step_.crl();
    int pady = (int)( getInlXlnDist(b2c,false,step) /SI().crlDistance() )+1;
    padfac_ = mMAX( padx, pady );

    return true;
}


void SeisCubeImpFromOtherSurvey::setPars( Interpol& interp, int cellsz,
					const TrcKeyZSampling& cs )
{
    interpol_ = interp;
    data_.tkzs_ = cs;
    data_.tkzs_.limitTo( TrcKeyZSampling(true) );
    data_.tkzs_.hsamp_.snapToSurvey();
    data_.hsit_->setSampling( data_.tkzs_.hsamp_ );
    totnr_ = mCast( int, data_.tkzs_.hsamp_.totalNr() );
    if ( cellsz<1 )
	return;

    fft_ = Fourier::CC::createDefault();
    sz_ = fft_->getFastSize( cellsz );
    StepInterval<float> zsi( data_.tkzs_.zsamp_ );
    zsi.step = olddata_.tkzs_.zsamp_.step;
    szz_ = fft_->getFastSize( zsi.nrSteps() );
    arr_ = new CplxArr3D( sz_, sz_, szz_ );
    fftarr_ = new CplxArr3D( sz_, sz_, szz_ );
    newsz_ = fft_->getFastSize( sz_*padfac_ );
    taper_ = new ArrayNDWindow(Array1DInfoImpl(szz_),false,"CosTaper",0.95);
}


float SeisCubeImpFromOtherSurvey::getInlXlnDist( const Pos::IdxPair2Coord& b2c,
						 bool inldir, int step ) const
{
    BinID orgbid = BinID( 0, 0 );
    BinID nextbid = BinID( inldir ? step : 0, inldir ? 0 : step );
    const Coord c00 = b2c.transform( orgbid );
    const Coord c10 = b2c.transform( nextbid );
    return c00.distTo<float>(c10);
}


bool SeisCubeImpFromOtherSurvey::createReader( const char* mainfilenm )
{
    BufferString fnm( mainfilenm ? mainfilenm : inioobj_.mainFileName().str() );
    if ( isCBVS(fnm) )
    {
	uiString errmsg;
	cbvstr_ = CBVSSeisTrcTranslator::make( fnm, false, false, &errmsg,
						true );
	uirv_.add( errmsg );
	uirv_.add( cbvstr_->errMsg() );
	if ( !uirv_.isOK() )
	    { deleteAndZeroPtr( cbvstr_ ); return false; }
    }
    else
    {
	rdr_ = new Seis::Blocks::Reader( fnm );
	uirv_ = rdr_->state();
	if ( !uirv_.isOK() )
	    { deleteAndZeroPtr( rdr_ ); return false; }
    }

    return true;
}



int SeisCubeImpFromOtherSurvey::nextStep()
{
    if ( !cbvstr_ && !rdr_ )
	return Executor::ErrorOccurred();

    if ( !data_.hsit_->next() )
    {
	deleteAndZeroPtr( storer_ ); // close stuff asap
	return Executor::Finished();
    }

    data_.curbid_ = data_.hsit_->curBinID();
    const Coord curcoord = SI().transform( data_.curbid_ );
    const Pos::IdxPair2Coord& b2c = rdr_ ? rdr_->hGeom().binID2Coord()
				         : cbvstr_->getTransform();
    const BinID oldbid = BinID( b2c.transformBack( curcoord,
	olddata_.tkzs_.hsamp_.start_, olddata_.tkzs_.hsamp_.step_ ) );
    SeisTrc* outtrc = 0;
    if ( interpol_==Nearest || padfac_<=1 )
    {
	outtrc = readTrc( oldbid );
	if ( !outtrc )
	{
	    outtrc = new SeisTrc( data_.tkzs_.zsamp_.nrSteps() );
	    outtrc->zero();
	}

	outtrc->info().sampling_ = olddata_.tkzs_.zsamp_;
    }
    else
    {
	bool needgathertrcs = olddata_.curbid_ != oldbid || trcsset_.isEmpty();
	olddata_.curbid_ = oldbid;
	if ( needgathertrcs )
	{
	    if ( !findSquareTracesAroundCurbid( trcsset_ ) )
		{ nrdone_++; return Executor::MoreToDo(); }
	    sincInterpol( trcsset_ );
	}
	float mindist = mUdf( float );
	int outtrcidx = 0;
	for ( int idx=0; idx<trcsset_.size(); idx++ )
	{
	    const Coord trccoord = trcsset_[idx]->info().coord_;
	    float dist = (float) trccoord.sqDistTo( curcoord );
	    if ( dist < mindist || mIsUdf( mindist ) )
	    {
		mindist = dist; outtrcidx = idx;
	    }
	}
	outtrc = new SeisTrc( *trcsset_[outtrcidx] );
    }
    outtrc->info().setPos( data_.curbid_ );

    if ( !storer_ )
	storer_ = new Seis::Storer( *outioobj_ );
    uirv_ = storer_->put( *outtrc );
    delete outtrc;
    if ( !uirv_.isOK() )
	return Executor::ErrorOccurred();

    nrdone_ ++;
    return Executor::MoreToDo();
}


SeisTrc* SeisCubeImpFromOtherSurvey::readTrc( const BinID& bid ) const
{
    SeisTrc* trc = 0;
    if ( rdr_ )
    {
	trc = new SeisTrc;
	if ( rdr_->get(bid,*trc).isError() )
	    { delete trc; return 0; }
    }
    else if ( cbvstr_ && cbvstr_->goTo( bid )  )
    {
	trc = new SeisTrc;
	if ( !cbvstr_->read(*trc) )
	    { delete trc; return 0; }
    }
    return trc;
}


bool SeisCubeImpFromOtherSurvey::findSquareTracesAroundCurbid(
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

void SeisCubeImpFromOtherSurvey::sincInterpol( ObjectSet<SeisTrc>& trcs ) const
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
    CplxArr3D padfftarr( newszx, newszy, szz_ );

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

    CplxArr3D padarr( newszx, newszy, szz_ );
    mDoFFT( false, padfftarr, padarr, newszx, newszy, szz_ )

    const Coord startcrd = trcs[0]->info().coord_;
    const Coord nextcrlcrd = trcs[1]->info().coord_;
    const Coord nextinlcrd = trcs[sz_]->info().coord_;
    const double xcrldist = (nextcrlcrd.x_-startcrd.x_)/padfac_;
    const double ycrldist = (nextcrlcrd.y_-startcrd.y_)/padfac_;
    const double xinldist = (nextinlcrd.x_-startcrd.x_)/padfac_;
    const double yinldist = (nextinlcrd.y_-startcrd.y_)/padfac_;
    const float amplfac = float(padfac_*padfac_);

    deepErase( trcs );
    for ( int idx=0; idx<newszx; idx ++ )
    {
	for ( int idy=0; idy<newszy; idy++ )
	{
	    SeisTrc* trc = new SeisTrc( szz_ );
	    trc->info().sampling_ = olddata_.tkzs_.zsamp_;
	    trc->info().coord_.x_ = startcrd.x_ + idy*xcrldist + idx*xinldist;
	    trc->info().coord_.y_ = startcrd.y_ + idy*ycrldist + idx*yinldist;
	    trcs += trc;
	    for ( int idz=0; idz<szz_; idz++ )
	    {
		if ( idz < trc->size() )
		    trc->set( idz, padarr.get(idx,idy,idz).real()*amplfac, 0 );
	    }
	}
    }
}
