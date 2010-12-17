/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiscbvsimpfromothersurv.cc,v 1.3 2010-12-17 10:15:31 cvsbruno Exp $";

#include "uiseiscbvsimpfromothersurv.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "cbvsreader.h"
#include "cbvsreadmgr.h"
#include "ctxtioobj.h"
#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "fourier.h"
#include "ioman.h"
#include "keystrs.h"
#include "seisinfo.h"
#include "seis2dline.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "seiscbvs.h"
#include "seistrctr.h"
#include "seisread.h"
#include "seiswrite.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "uiseistransf.h"
#include "uiselsimple.h"
#include "uiselobjothersurv.h"
#include "uiseparator.h"
#include "uitaskrunner.h"

#include <iostream>


static const char* interpols[] = { "Sinc interpolation", "Nearest trace", 0 };

uiSeisImpCBVSFromOtherSurveyDlg::uiSeisImpCBVSFromOtherSurveyDlg( uiParent* p )
    : uiDialog(p,Setup("Import CBVS cube from other survey",
			"Specify import parameters", mTODOHelpID))
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

    cellsizefld_ = new uiLabeledSpinBox( this, "Grid cell size" );
    cellsizefld_->attach( alignedBelow, interpfld_ );
    cellsizefld_->box()->setInterval( 2, 10, 2 );
    cellsizefld_->box()->setValue( 4 );

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
    const SeisImpCBVSFromOtherSurvey::Interpol inter = 
			(SeisImpCBVSFromOtherSurvey::Interpol)(curitm);
    bool issinc = inter == SeisImpCBVSFromOtherSurvey::Sinc;
    if ( import_ )
    {
	import_->setInterpol( inter );
	import_->setCellSize( issinc ? cellsizefld_->box()->getValue() : 0 );
    }
    cellsizefld_->display( issinc );
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
	    subselfld_->setInput( import_->horSampling() ); 
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
    interpSelDone( 0 );
    import_->setOutput( *outctio_.ioobj );
    //TODO replace with batch
    uiTaskRunner tr( this );
    return tr.execute( *import_ );
}



SeisImpCBVSFromOtherSurvey::SeisImpCBVSFromOtherSurvey( const IOObj& inp )
    : Executor("Importing CBVS")
    , inioobj_(inp)	
    , outioobj_(0)
    , wrr_(0)
    , hrg_(false) 
    , hsit_(0)
    , nrdone_(0)
    , tr_(0)
    , cellsize_(0)
    , fullusrexp_(0)
{}


SeisImpCBVSFromOtherSurvey::~SeisImpCBVSFromOtherSurvey()
{
    deepErase( trcsset_ );
    delete tr_;
    delete wrr_;
    delete hsit_;
}


bool SeisImpCBVSFromOtherSurvey::prepareRead( const char* fulluserexp )
{
    if ( !createTranslators( fulluserexp ) )
	mErrRet( "Can not read cube" )

    const RCol2Coord& b2c = tr_->getTransform();
    const CBVSInfo::SurvGeom& geom = tr_->readMgr()->info().geom;
    oldhrg_.start = BinID( geom.start.inl, geom.start.crl ); 
    oldhrg_.stop  = BinID( geom.stop.inl, geom.stop.crl ); 
    oldhrg_.step  = BinID( geom.step.inl, geom.step.crl ); 
    hsit_ = new HorSamplingIterator( oldhrg_ );

    BinID bid;
    while ( hsit_->next( bid ) )
	hrg_.include( SI().transform( b2c.transform( bid ) ) );

    if ( !SI().isInside(hrg_.start,true) && !SI().isInside(hrg_.stop,true) )
	mErrRet("The selected cube has no coordinate inside the current survey")

    hsit_->setSampling( hrg_ ); 
    totnr_ = hrg_.totalNr();

    int step = oldhrg_.step.inl;
    xzeropadfac_ = mNINT( getInlXlnDist(b2c,true,step)/SI().inlDistance() );
    step = oldhrg_.step.crl;
    yzeropadfac_ = mNINT( getInlXlnDist(b2c,false,step)/SI().crlDistance() );

    return true;
}


float SeisImpCBVSFromOtherSurvey::getInlXlnDist( const RCol2Coord& b2c, 
						 bool inldir, int step ) const
{
    BinID orgbid = BinID( 0, 0 );
    BinID nextbid = BinID( inldir ? step : 0, inldir ? 0 : step );
    const Coord c00 = b2c.transform( orgbid );
    const Coord c10 = b2c.transform( nextbid );
    return c00.distTo(c10);
}


bool SeisImpCBVSFromOtherSurvey::createTranslators( const char* fulluserexp )
{
    BufferString fnm( fulluserexp ? fulluserexp : inioobj_.fullUserExpr(true) );
    tr_= CBVSSeisTrcTranslator::make( fnm, false, false, 0, true ); 
    return tr_ ? true : false;
}


bool SeisImpCBVSFromOtherSurvey::createWriter()
{
    wrr_ = new SeisTrcWriter( outioobj_ );
    return true;
}


int SeisImpCBVSFromOtherSurvey::nextStep()
{
    if ( !hsit_->next(curbid_) )
	return Executor::Finished();
    
    if ( !tr_ || !tr_->readMgr() ) 
	return Executor::ErrorOccurred();

    SeisTrc* outtrc = 0; 
    const Coord& curcoord = SI().transform( curbid_ );
    const RCol2Coord& b2c = tr_->getTransform();
    const StepInterval<int> rowrg( oldhrg_.inlRange() );
    const StepInterval<int> colrg( oldhrg_.crlRange() );
    const BinID& oldbid = b2c.transformBack( curcoord, &rowrg, &colrg );
    if ( interpol_ == Nearest )
    {
	outtrc = readTrc( oldbid ); 
	if ( !outtrc )
	    { nrdone_++; return Executor::MoreToDo(); }
    }
    else
    {
	bool needgathertraces = curoldbid_ != oldbid; 
	curoldbid_ = oldbid;
	if ( needgathertraces || trcsset_.isEmpty() )
	{
	    if ( !findSquareTracesAroundCurbid( trcsset_ ) )
		{ nrdone_++; return Executor::MoreToDo(); }
	    if ( cellsize_ > 1 && yzeropadfac_ > 1 && xzeropadfac_ > 1 )
		sincInterpol( trcsset_ );
	}
	float mindist = mUdf( float );
	for ( int idx=0; idx<trcsset_.size(); idx++ )
	{
	    const Coord trccoord = trcsset_[idx]->info().coord;
	    float dist = trccoord.sqDistTo( curcoord );
	    if ( dist < mindist || mIsUdf( mindist ) )
	    {
		mindist = dist;
		outtrc = trcsset_[idx];
	    }
	}
    }
    outtrc->info().binid = curbid_; 

    if ( !wrr_ && !createWriter() )
	return Executor::ErrorOccurred();
    if ( !wrr_->put( *outtrc ) )
	{ errmsg_ = wrr_->errMsg(); return Executor::ErrorOccurred(); }

    nrdone_ ++;
    return Executor::MoreToDo();
}


SeisTrc* SeisImpCBVSFromOtherSurvey::readTrc( const BinID& bid ) const
{
    SeisTrc* trc = 0;
    if ( tr_->goTo( bid )  )
    {
	trc = new SeisTrc();
	trc->info().binid = bid;
	tr_->readInfo( trc->info() ); 
	tr_->read( *trc );
    }
    return trc;
}


bool SeisImpCBVSFromOtherSurvey::findSquareTracesAroundCurbid(
						ObjectSet<SeisTrc>& trcs) const
{
    deepErase( trcs );
    const int inlstep = oldhrg_.step.inl;
    const int crlstep = oldhrg_.step.crl;
    const int nrinltrcs = cellsize_*inlstep/2;
    const int nrcrltrcs = cellsize_*crlstep/2;
    for ( int idinl=-nrinltrcs; idinl<nrinltrcs; idinl+=inlstep)
    {
	for ( int idcrl=-nrcrltrcs; idcrl<nrcrltrcs; idcrl+=crlstep)
	{
	    BinID oldbid( curoldbid_.inl + idinl, curoldbid_.crl + idcrl );
	    SeisTrc* trc = readTrc( oldbid );
	    if ( !trc )
		{ deepErase( trcs ); return false; }
	    trcs += trc;
	}
    }
    return !trcs.isEmpty();
}


#define mDoFFT(isforward,inp,outp,dim1,dim2,dim3)\
{\
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault(); \
    fft->setInputInfo(Array3DInfoImpl(dim1,dim2,dim3));\
    fft->setDir(isforward);\
    fft->setNormalization(!isforward); \
    fft->setInput(inp.getData());\
    fft->setOutput(outp.getData());\
    fft->run(true); \
}

void SeisImpCBVSFromOtherSurvey::sincInterpol( ObjectSet<SeisTrc>& trcs ) const
{
    if ( trcs.size() < 2 ) 
	return;

    int szx = cellsize_;
    int szy = cellsize_;
    int szz = trcs[0]->size();
    int newszx = szx*xzeropadfac_;
    int newszy = szy*yzeropadfac_;
    int newszz = szz;

    Array3DImpl<float_complex> arr( szx, szy, szz );
    int cpt =0;
    for ( int idx=0; idx<szx; idx ++ )
    {
	for ( int idy=0; idy<szy; idy++ )
	{
	    const SeisTrc& trc = *trcs[cpt];
	    for ( int idz=0; idz<szz; idz++ )
		arr.set( idx, idy, idz, trc.get( idz, 0 ) );
	    cpt ++;
	}
    }
    Array3DImpl<float_complex> fftarr( szx, szy, szz );
    mDoFFT( true, arr, fftarr, szx, szy, szz )

    Array3DImpl<float_complex> padfftarr( newszx, newszy, newszz );
    int xpadsz = (int)( szx/2 );
    int ypadsz = (int)( szy/2 );
#define mSetVal(ix,iy,iz) padfftarr.set( ix, iy, iz, fftarr.get(idx,idy,idz) );
    for ( int idz=0; idz<newszz; idz++ ) 
    {
	for ( int idx=0; idx<xpadsz; idx++)
	{
	    for ( int idy=0; idy<ypadsz; idy++)
		mSetVal( idx, idy, idz )
	}
	for ( int idx=xpadsz; idx<szx; idx++)
	{
	    for ( int idy=ypadsz; idy<szy; idy++)
		mSetVal( newszx-szx+idx, newszy-szy+idy, idz )
	}
	for ( int idx=xpadsz; idx<szx; idx++)
	{
	    for ( int idy=0; idy<ypadsz; idy++ )
		mSetVal( newszx-szx+idx, idy, idz )
	}
	for ( int idx=0; idx<xpadsz; idx++)
	{
	    for ( int idy=ypadsz; idy<szy; idy++)
		mSetVal( idx, newszy-szy+idy, idz ) 
	}
    }

    Array3DImpl<float_complex> padarr( newszx, newszy, newszz );
    mDoFFT( false, padfftarr, padarr, newszx, newszy, newszz )

    const Coord startcrd = trcs[0]->info().coord;
    const Coord nextcrlcrd = trcs[1]->info().coord;
    const Coord nextinlcrd = trcs[cellsize_]->info().coord;
    float xcrldist = (nextcrlcrd.x-startcrd.x)/xzeropadfac_;
    float ycrldist = (nextcrlcrd.y-startcrd.y)/yzeropadfac_;
    float xinldist = (nextinlcrd.x-startcrd.x)/xzeropadfac_;
    float yinldist = (nextinlcrd.y-startcrd.y)/yzeropadfac_;

    deepErase( trcs );
    for ( int idx=0; idx<newszx; idx ++ )
    {
	for ( int idy=0; idy<newszy; idy++ )
	{
	    SeisTrc* trc = new SeisTrc( szz );
	    trc->info().coord.x = startcrd.x + idy*xcrldist + idx*xinldist;
	    trc->info().coord.y = startcrd.y + idy*ycrldist + idx*yinldist;

	    for ( int idz=0; idz<newszz; idz++ )
		trc->set( idz, padarr.get( idx, idy, idz ).real(), 0 );
	    trcs += trc;
	}
    }
}


od_int64 SeisImpCBVSFromOtherSurvey::totalNr() const
{
    return totnr_;
}
