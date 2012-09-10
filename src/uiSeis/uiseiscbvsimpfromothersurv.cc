/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiseiscbvsimpfromothersurv.cc,v 1.21 2012-09-10 07:42:45 cvsranojay Exp $";

#include "uiseiscbvsimpfromothersurv.h"

#include "arrayndutils.h"
#include "cbvsreadmgr.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiselobjothersurv.h"
#include "uiseparator.h"
#include "uitaskrunner.h"


static const char* interpols[] = { "Sinc interpolation", "Nearest trace", 0 };

uiSeisImpCBVSFromOtherSurveyDlg::uiSeisImpCBVSFromOtherSurveyDlg( uiParent* p )
    : uiDialog(p,Setup("Import CBVS cube from other survey",
			"Specify import parameters", "103.0.22"))
    , inctio_(*mMkCtxtIOObj(SeisTrc))
    , outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
    , import_(0)							
{
    setCtrlStyle( DoAndStay );

    finpfld_ = new uiGenInput( this, "CBVS file name" );
    finpfld_->setReadOnly();
    CallBack cb = mCB(this,uiSeisImpCBVSFromOtherSurveyDlg,cubeSel);
    uiPushButton* selbut = new uiPushButton( this, "&Select ...", cb, true );
    selbut->attach( rightOf, finpfld_ );

    subselfld_ = new uiSeis3DSubSel( this, Seis::SelSetup( false ) );
    subselfld_->attach( alignedBelow, finpfld_ );

    uiSeparator* sep1 = new uiSeparator( this, "sep" );
    sep1->attach( stretchedBelow, subselfld_ );

    interpfld_ = new uiGenInput( this, "Interpolation", 
			    BoolInpSpec( true, interpols[0], interpols[1] ) );
    interpfld_->valuechanged.notify( 
	    	mCB(this,uiSeisImpCBVSFromOtherSurveyDlg,interpSelDone) );
    interpfld_->attach( ensureBelow, sep1 ); 
    interpfld_->attach( alignedBelow, subselfld_ ); 

    cellsizefld_ = new uiLabeledSpinBox( this, "Lateral stepout (Inl/Crl)" );
    cellsizefld_->attach( alignedBelow, interpfld_ );
    cellsizefld_->box()->setInterval( 2, 12, 2 );
    cellsizefld_->box()->setValue( 8 );

    uiSeparator* sep2 = new uiSeparator( this, "sep" );
    sep2->attach( stretchedBelow, cellsizefld_ );

    outctio_.ctxt.forread = false;
    outctio_.ctxt.toselect.allowtransls_ = "CBVS";
    uiSeisSel::Setup sssu( Seis::Vol );
    outfld_ = new uiSeisSel( this, outctio_, sssu );
    outfld_->attach( alignedBelow, cellsizefld_ );
    outfld_->attach( ensureBelow, sep2 );
    IOM().to( outctio_.ctxt.getSelKey() );

    interpSelDone(0);
}


uiSeisImpCBVSFromOtherSurveyDlg::~uiSeisImpCBVSFromOtherSurveyDlg()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


void uiSeisImpCBVSFromOtherSurveyDlg::interpSelDone( CallBacker* )
{
    const bool curitm = interpfld_->getBoolValue() ? 0 : 1; 
    interpol_ = (SeisImpCBVSFromOtherSurvey::Interpol)(curitm);
    issinc_ = interpol_ == SeisImpCBVSFromOtherSurvey::Sinc;
    cellsizefld_->display( issinc_ );
}


void uiSeisImpCBVSFromOtherSurveyDlg::cubeSel( CallBacker* )
{
    uiSelObjFromOtherSurvey objdlg( this, inctio_ );
    if ( objdlg.go() && inctio_.ioobj )
    {
	if ( import_ ) delete import_;
	import_ = new SeisImpCBVSFromOtherSurvey( *inctio_.ioobj ); 
	BufferString fusrexp; objdlg.getIOObjFullUserExpression( fusrexp );
	if ( import_->prepareRead( fusrexp ) )
	{
	    finpfld_->setText( fusrexp );
	    subselfld_->setInput( import_->cubeSampling() ); 
	}
	else
	    { delete import_; import_ = 0; }
    }
}


#define mErrRet(msg ) { uiMSG().error( msg ); return false; }
bool uiSeisImpCBVSFromOtherSurveyDlg::acceptOK( CallBacker* )
{
    if ( !import_ )
	mErrRet( "No valid input, please select a new input file" ) 

    if ( !outfld_->commitInput() )
    {
	if ( outfld_->isEmpty() )
	    mErrRet( "Please select a name for the output data" )
	else
	    mErrRet( "Can not process output" ) 
    }
    int cellsz = issinc_ ? cellsizefld_->box()->getValue() : 0;
    CubeSampling cs; subselfld_->getSampling( cs );
    import_->setPars( interpol_, cellsz, cs );
    import_->setOutput( *outctio_.ioobj );
    uiTaskRunner tr( this );
    return tr.execute( *import_ );
}



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


bool SeisImpCBVSFromOtherSurvey::prepareRead( const char* fulluserexp )
{
    if ( !createTranslators( fulluserexp ) )
	mErrRet( "Can not read cube" )

    const CBVSInfo& info = tr_->readMgr()->info();
    const RCol2Coord& b2c = tr_->getTransform();
    const CBVSInfo::SurvGeom& geom = info.geom_;
    olddata_.cs_.hrg.start = BinID( geom.start.inl, geom.start.crl );
    olddata_.cs_.hrg.stop  = BinID( geom.stop.inl, geom.stop.crl );
    olddata_.cs_.hrg.step  = BinID( geom.step.inl, geom.step.crl ); 
    data_.hsit_ = new HorSamplingIterator( olddata_.cs_.hrg );
    olddata_.cs_.zrg = info.sd_.interval( info.nrsamples_ );
    data_.cs_.zrg = olddata_.cs_.zrg; data_.cs_.zrg.step = SI().zStep();

    BinID bid;
    while ( data_.hsit_->next( bid ) )
	data_.cs_.hrg.include( SI().transform( b2c.transform( bid ) ) );

    if ( !SI().isInside(data_.cs_.hrg.start,true) 
	&& !SI().isInside(data_.cs_.hrg.stop,true) )
	mErrRet("The selected cube has no coordinate matching the current survey")

    int step = olddata_.cs_.hrg.step.inl;
    int padx = (int)( getInlXlnDist(b2c,true,step ) /SI().inlDistance() )+1;
    step = olddata_.cs_.hrg.step.crl;
    int pady = (int)( getInlXlnDist(b2c,false,step) /SI().crlDistance() )+1;
    padfac_ = mMAX( padx, pady );

    return true;
}


void SeisImpCBVSFromOtherSurvey::setPars( Interpol& interp, int cellsz, 
					const CubeSampling& cs )
{
    interpol_ = interp; 
    data_.cs_ = cs;
    data_.cs_.limitTo( SI().sampling(false) );
    data_.cs_.hrg.snapToSurvey();
    data_.hsit_->setSampling( data_.cs_.hrg ); 
    totnr_ = data_.cs_.hrg.totalNr();
    if ( !cellsz ) return; 
    fft_ = Fourier::CC::createDefault(); 
    sz_ = fft_->getFastSize( cellsz );
    StepInterval<float> zsi( data_.cs_.zrg ); 
    zsi.step = olddata_.cs_.zrg.step;
    szz_ = fft_->getFastSize( zsi.nrSteps() );
    arr_ = new Array3DImpl<float_complex>( sz_, sz_, szz_ );
    fftarr_ = new Array3DImpl<float_complex>( sz_, sz_, szz_ );
    newsz_ = fft_->getFastSize( sz_*padfac_ );
    taper_ = new ArrayNDWindow(Array1DInfoImpl(szz_),false,"CosTaper",0.95);
}


float SeisImpCBVSFromOtherSurvey::getInlXlnDist( const RCol2Coord& b2c, 
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
    tr_= CBVSSeisTrcTranslator::make( fnm, false, false, 0, true ); 
    return tr_ ? true : false;
}



int SeisImpCBVSFromOtherSurvey::nextStep()
{
    if ( !data_.hsit_->next(data_.curbid_) )
	return Executor::Finished();
    
    if ( !tr_ || !tr_->readMgr() ) 
	return Executor::ErrorOccurred();

    const Coord& curcoord = SI().transform( data_.curbid_ );
    const StepInterval<int>& rowrg( olddata_.cs_.hrg.inlRange() );
    const StepInterval<int>& colrg( olddata_.cs_.hrg.crlRange() );
    const RCol2Coord& b2c = tr_->getTransform();
    const BinID& oldbid = b2c.transformBack( curcoord, &rowrg, &colrg );
    SeisTrc* outtrc = 0; 
    if ( interpol_ == Nearest || padfac_ <= 1 )
    {
	outtrc = readTrc( oldbid ); 
	if ( !outtrc )
	{
	    outtrc = new SeisTrc( data_.cs_.zrg.nrSteps() );
	    outtrc->zero();
	}

	outtrc->info().sampling = olddata_.cs_.zrg;
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
	    const Coord trccoord = trcsset_[idx]->info().coord;
	    float dist = (float) trccoord.sqDistTo( curcoord );
	    if ( dist < mindist || mIsUdf( mindist ) )
	    {
		mindist = dist; outtrcidx = idx;
	    }
	}
	outtrc = new SeisTrc( *trcsset_[outtrcidx] );
    }
    outtrc->info().binid = data_.curbid_; 

    if ( !wrr_ ) 
	wrr_ = new SeisTrcWriter(outioobj_);
    if ( !wrr_->put( *outtrc ) )
    { 
	errmsg_ = wrr_->errMsg(); 
	delete outtrc;
	return Executor::ErrorOccurred(); 
    }

    delete outtrc;

    nrdone_ ++;
    return Executor::MoreToDo();
}


SeisTrc* SeisImpCBVSFromOtherSurvey::readTrc( const BinID& bid ) const
{
    SeisTrc* trc = 0; 
    if ( tr_->goTo( bid )  )
    {
	trc = new SeisTrc;
	trc->info().binid = bid;
	tr_->readInfo( trc->info() ); 
	tr_->read( *trc );
    }
    return trc;
}


bool SeisImpCBVSFromOtherSurvey::findSquareTracesAroundCurbid(
					    ObjectSet<SeisTrc>& trcs ) const
{
    deepErase( trcs );
    const int inlstep = olddata_.cs_.hrg.step.inl;
    const int crlstep = olddata_.cs_.hrg.step.crl;
    const int nrinltrcs = sz_*inlstep/2;
    const int nrcrltrcs = sz_*crlstep/2;
    for ( int idinl=-nrinltrcs; idinl<nrinltrcs; idinl+=inlstep)
    {
	for ( int idcrl=-nrcrltrcs; idcrl<nrcrltrcs; idcrl+=crlstep)
	{
	    BinID oldbid( olddata_.curbid_.inl + idinl, 
		    	  olddata_.curbid_.crl + idcrl );
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
	xxxx    -> 	000000
	xxxx    	000000
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

    int szx = sz_; 	int newszx = newsz_;	 int xpadsz = (int)(szx/2);
    int szy = sz_; 	int newszy = newsz_;	 int ypadsz = (int)(szy/2);

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
    const float xcrldist = (float) ( (nextcrlcrd.x-startcrd.x)/padfac_ );
    const float ycrldist = (float) ( (nextcrlcrd.y-startcrd.y)/padfac_ );
    const float xinldist = (float) ( (nextinlcrd.x-startcrd.x)/padfac_ );
    const float yinldist = (float) ( (nextinlcrd.y-startcrd.y)/padfac_ );

    deepErase( trcs );
    for ( int idx=0; idx<newszx; idx ++ )
    {
	for ( int idy=0; idy<newszy; idy++ )
	{
	    SeisTrc* trc = new SeisTrc( szz_ );
	    trc->info().sampling = olddata_.cs_.zrg;
	    trc->info().coord.x = startcrd.x + idy*xcrldist + idx*xinldist;
	    trc->info().coord.y = startcrd.y + idy*ycrldist + idx*yinldist;
	    trcs += trc;
	    for ( int idz=0; idz<szz_; idz++ )
	    {
		float amplfac = padfac_*padfac_;
		if ( idz < trc->size() ) 
		    trc->set( idz, padarr.get(idx,idy,idz).real()*amplfac, 0 );
	    }
	}
    }
}
