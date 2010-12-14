/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiscbvsimpfromothersurv.cc,v 1.1 2010-12-14 08:52:02 cvsbruno Exp $";

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

#include "uidialog.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "uiseistransf.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"

#include <iostream>


uiSeisImpCBVSFromOtherSurveyDlg::uiSeisImpCBVSFromOtherSurveyDlg( uiParent* p, 
							    const IOObj& ioobj )
    : uiDialog(p,Setup("Import CBVS cube from other survey",
			"Specify import parameters",
			mTODOHelpID))
    , inctio_(*mMkCtxtIOObj(SeisTrc))
    , outctio_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,false))
{
    setCtrlStyle( DoAndStay );

    inctio_.ioobj = ioobj.clone();
    setCtrlStyle( DoAndStay );

    outctio_.ctxt.forread = false;
    outctio_.ctxt.toselect.allowtransls_ = "CBVS";
    uiSeisSel::Setup sssu( Seis::Vol );
    outfld_ = new uiSeisSel( this, outctio_, sssu );
    IOM().to( outctio_.ctxt.getSelKey() );
}


uiSeisImpCBVSFromOtherSurveyDlg::~uiSeisImpCBVSFromOtherSurveyDlg()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


bool uiSeisImpCBVSFromOtherSurveyDlg::acceptOK( CallBacker* )
{
    if ( !outfld_->commitInput() )
    {
	if ( outfld_->isEmpty() )
	uiMSG().error( "Please select a name for the output data" );
	return false;
    }
    SeisImpCBVSFromOtherSurvey imp( *inctio_.ioobj, *outctio_.ioobj );

    uiTaskRunner tr( this );
    return tr.execute( imp );
}


SeisImpCBVSFromOtherSurvey::SeisImpCBVSFromOtherSurvey( const IOObj& inp,
							IOObj& outp )
    : Executor("Importing CBVS")
    , inioobj_(inp)	
    , outioobj_(outp)
    , wrr_(0)
    , hsit_(0) 
    , nrdone_(0)							    
    , bidset_(2,false)
    , tr_(0)						    
{
    prepareRead();
}


SeisImpCBVSFromOtherSurvey::~SeisImpCBVSFromOtherSurvey()
{
    delete tr_;
    delete wrr_;
    delete hsit_;
}


void SeisImpCBVSFromOtherSurvey::prepareRead()
{
    createTranslators();

    getCubeSamplingInNewSurvey();
    const RCol2Coord& b2c = tr_->readMgr()->info().geom.b2c;
    oldinldist_ = getInlXlnDist( b2c, true ); 
    oldxlndist_ = getInlXlnDist( b2c, false ); 
    xzeropadfac_ = mNINT( oldinldist_/SI().inlDistance() );
    yzeropadfac_ = mNINT( oldxlndist_/SI().crlDistance() );

    HorSampling* hrg = new HorSampling();
    hrg->set( bidset_.inlRange(), bidset_.crlRange() );
    hrg->step.inl = SI().inlStep();
    hrg->step.crl = SI().crlStep();
    totnr_ = hrg->totalNr();
    hsit_ = new HorSamplingIterator( *hrg );
}


float SeisImpCBVSFromOtherSurvey::getInlXlnDist( const RCol2Coord& b2c, 
						 bool inldir ) const
{
    BinID orgbid = BinID( 0, 0 );
    BinID incrbid = BinID( inldir ? 1 : 0, inldir ? 0 : 1 );
    const Coord c00 = b2c.transform( orgbid );
    const Coord c10 = b2c.transform( incrbid );
    return c00.distTo(c10);
}


void SeisImpCBVSFromOtherSurvey::getCubeSamplingInNewSurvey() 
{
    getCubeInfo( oldcoords_, oldbids_ );
    for ( int idx=0; idx<oldcoords_.size(); idx++ )
    {
	const BinID bid = SI().transform( oldcoords_[idx] );
	if ( !SI().isInside( bid, true ) )
	    continue;
	bidset_.add( bid, oldbids_[idx].inl, oldbids_[idx].crl );
    }
}


void SeisImpCBVSFromOtherSurvey::getCubeInfo( TypeSet<Coord>& coords,
					    TypeSet<BinID>& bids ) const
{
    TypeSet<Coord> posns; TypeSet<BinID> posbids;
    if ( !tr_ && !tr_->readMgr() ) return;
    tr_->readMgr()->getPositions( posbids );
    bids.append( posbids );
    const RCol2Coord& b2c = tr_->readMgr()->info().geom.b2c;
    for ( int idx=0; idx<bids.size(); idx++ )
	coords += b2c.transform( bids[idx] ); 
}


bool SeisImpCBVSFromOtherSurvey::createTranslators()
{
    BufferString fname( inioobj_.name() );
    tr_= CBVSSeisTrcTranslator::make( fname, false, false, 0 ); 
    return tr_ ? true : false;
}


bool SeisImpCBVSFromOtherSurvey::createWriter()
{
    wrr_ = new SeisTrcWriter( &outioobj_ );
    return true;
}


int SeisImpCBVSFromOtherSurvey::nextStep()
{
    if ( !hsit_->next(curbid_) )
	return Executor::Finished();
    
    if ( !tr_ || !tr_->readMgr() ) 
	return Executor::ErrorOccurred();

    int nrtrcs = 3;
    int zpadfac = 12;

    ObjectSet<SeisTrc> trcs; 
    if ( !findSquareTracesAroundCurbid( nrtrcs, trcs ) )
	return Executor::MoreToDo();

    if ( zpadfac && nrtrcs > 0 )
	sincInterpol( 2*nrtrcs, trcs, zpadfac );
    const Coord coord = SI().transform( curbid_ );

    float mindist = mUdf( float );
    int coordidx = 0;
    SeisTrc* outtrc = 0;
    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	const Coord trccoord = trcs[idx]->info().coord;
	float dist = trccoord.sqDistTo( coord );
	if ( dist < mindist || mIsUdf( mindist ) )
	{
	    mindist = dist;
	    coordidx = idx;
	    outtrc = trcs[idx];
	}
    }
    outtrc->info().binid = curbid_; 

    if ( !wrr_ && !createWriter() )
	return Executor::ErrorOccurred();
    if ( !wrr_->put( *outtrc ) )
	{ errmsg_ = wrr_->errMsg(); return Executor::ErrorOccurred(); }

    deepErase( trcs );
    nrdone_ ++;
    return Executor::MoreToDo();
}


bool SeisImpCBVSFromOtherSurvey::findSquareTracesAroundCurbid(
						int nrlateraltrcs,
						ObjectSet<SeisTrc>& trcs) const
{
    const Coord curcoord = SI().transform( curbid_ );
    if ( oldcoords_.isEmpty() ) 
       return false;

    double mindist = mUdf( double );
    int coordidx = 0;
    //TODO improve this by using the bidset_ 
    for ( int idx=0; idx<oldcoords_.size(); idx++ )
    {
	const Coord oldcoord = oldcoords_[idx];
	float dist = oldcoord.sqDistTo( curcoord );
	if ( dist < mindist || mIsUdf( mindist ) )
	{
	    mindist = dist;
	    coordidx = idx;
	}
    }
    const BinID bid = oldbids_[ coordidx ];
    int inlstep = tr_->readMgr()->info().geom.step.inl;
    int crlstep = tr_->readMgr()->info().geom.step.crl;
    nrlateraltrcs *= inlstep; //TODO change because not square
    for ( int idinl=-nrlateraltrcs; idinl<nrlateraltrcs; idinl+=inlstep)
    {
	for ( int idcrl=-nrlateraltrcs; idcrl<nrlateraltrcs; idcrl+=crlstep)
	{
	    BinID oldbid( bid.inl+idinl, bid.crl+idcrl );
	    if ( tr_->goTo( oldbid )  )
	    {
		SeisTrc* trc = new SeisTrc();
		trc->info().binid = oldbid;
		tr_->readInfo( trc->info() ); 
		tr_->read( *trc );
		trcs += trc;
	    }
	    else
		{ deepErase( trcs ); return false; }
	}
    }
    return !trcs.isEmpty();
}


#define mDoFFT(isforward,inp,outp,dim1,dim2)\
{\
    PtrMan<Fourier::CC> fft = Fourier::CC::createDefault(); \
    fft->setInputInfo(Array3DInfoImpl(dim1,dim1,dim2));\
    fft->setDir(isforward);\
    fft->setNormalization(!isforward); \
    fft->setInput(inp.getData());\
    fft->setOutput(outp.getData());\
    fft->run(true); \
}

void SeisImpCBVSFromOtherSurvey::sincInterpol( int lateralsz,
						ObjectSet<SeisTrc>& trcs,
						int padfac ) const
{
    int szz = trcs[0]->size();
    int newsz = lateralsz*padfac;
    int sz = lateralsz;
    int newszz = szz;

    Array3DImpl<float_complex> arr( sz, sz, szz );
    int cpt =0;
    for ( int idx=0; idx<sz; idx ++ )
    {
	for ( int idy=0; idy<sz; idy++ )
	{
	    const SeisTrc& trc = *trcs[cpt];
	    for ( int idz=0; idz<szz; idz++ )
		arr.set( idx, idy, idz, trc.get( idz, 0 ) );
	    cpt ++;
	}
    }
    Array3DImpl<float_complex> fftarr( sz, sz, szz );
    mDoFFT( true, arr, fftarr, sz, szz )

    Array3DImpl<float_complex> padfftarr( newsz, newsz, newszz );
    int padsz = (int)( lateralsz/2 );
    for ( int idz=0; idz<newszz; idz++ ) 
    {
	for ( int idx=0; idx<padsz; idx++)
	{
	    for ( int idy=0; idy<padsz; idy++)
		padfftarr.set( idx, idy, idz, fftarr.get( idx, idy, idz ) );
	}
	for ( int idx=padsz; idx<sz; idx++)
	{
	    for ( int idy=padsz; idy<sz; idy++)
		padfftarr.set( newsz-sz+idx, newsz-sz+idy, idz, 
					      fftarr.get( idx, idy, idz ) );
	}
	for ( int idx=padsz; idx<sz; idx++)
	{
	    for ( int idy=0; idy<padsz; idy++ )
		padfftarr.set( newsz-sz+idx, idy, idz, 
					      fftarr.get( idx, idy, idz ) );
	}
	for ( int idx=0; idx<padsz; idx++)
	{
	    for ( int idy=padsz; idy<sz; idy++)
		padfftarr.set( idx, newsz-sz+idy, idz, 
					      fftarr.get( idx, idy, idz ) );
	}
    }

    Array3DImpl<float_complex> padarr( newsz, newsz, newszz );
    mDoFFT( false, padfftarr, padarr, newsz, newszz )

    const Coord startcrd = trcs[0]->info().coord;
    const Coord nextcrlcrd = trcs[1]->info().coord;
    const Coord nextinlcrd = trcs[lateralsz]->info().coord;
    float xcrldist = (nextcrlcrd.x-startcrd.x)/padfac;
    float ycrldist = (nextcrlcrd.y-startcrd.y)/padfac;
    float xinldist = (nextinlcrd.x-startcrd.x)/padfac;
    float yinldist = (nextinlcrd.y-startcrd.y)/padfac;

    deepErase( trcs );
    for ( int idx=0; idx<newsz; idx ++ )
    {
	for ( int idy=0; idy<newsz; idy++ )
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
