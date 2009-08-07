/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          April 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwaveletextraction.cc,v 1.9 2009-08-07 12:34:55 cvsnageswara Exp $";

#include "uiwaveletextraction.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiioobj.h"
#include "uimsg.h"
#include "uiposprovgroup.h"
#include "uiposprovgroupstd.h"
#include "uiseissel.h"
#include "uiselsurvranges.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "bufstring.h"
#include "ctxtioobj.h"
#include "cubesampling.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceposprov.h"
#include "ioman.h"
#include "iopar.h"
#include "multiid.h"
#include "posprovider.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seisselectionimpl.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "wavelet.h"
#include "waveletextract.h"


uiWaveletExtraction::uiWaveletExtraction( uiParent* p )
    : uiDialog(this,Setup("Wavelet Extraction","Specify parameters","104.1.0")
	         .modal(true) )
    , seisctio_(*mMkCtxtIOObj(SeisTrc))
    , wvltctio_(*mMkCtxtIOObj(Wavelet))
    , wvltsize_(0)
    , zrangefld_(0)
    , betweenhors_(false)
    , extractionDone(this)
    , sd_(0)
{
    setCtrlStyle( DoAndStay );

    seisctio_.ctxt.forread = true;
    seisctio_.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    seisctio_.ctxt.includeconstraints = false;
    seisctio_.ctxt.allowcnstrsabsent = true;

    seisselfld_ = new uiSeisSel( this, seisctio_
	    			     , uiSeisSel::Setup(false,false) );
    seisselfld_->selectiondone.notify( mCB(this,uiWaveletExtraction
					       ,inputSel) );

    subselfld_ = new uiSeis3DSubSel( this, Seis::SelSetup( false,false)
	    				   .onlyrange(false)
					   .withstep(true)
	   				   .withoutz(true) );
    subselfld_->attach( alignedBelow, seisselfld_ );

    zextraction_ = new uiGenInput( this, "Vertical Extraction",
	    			   BoolInpSpec(true,"Z range","Horizons") );
    zextraction_->valuechanged.notify(
				mCB(this,uiWaveletExtraction,choiceSel) );
    zextraction_->attach( alignedBelow, subselfld_ );

    BufferString rangelbl = "Z Range ";
    rangelbl += SI().getZUnitString();
    zrangefld_ = new uiSelZRange( this, false, false, rangelbl );
    zrangefld_->attach( alignedBelow, zextraction_ );

    surfacesel_ = uiPosProvGroup::factory().create( sKey::Surface, this,
	   		 uiPosProvGroup::Setup(false,false,true) );
    surfacesel_->attach( alignedBelow, zextraction_ );

    wvltphasefld_ = new uiGenInput( this, "Phase (Degrees)", IntInpSpec(0) );
    wvltphasefld_->attach( alignedBelow, surfacesel_ );

    BufferString lbl = "Wavelet Length ";
    lbl += SI().getZUnitString();
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(120) );
    wtlengthfld_->attach( alignedBelow, wvltphasefld_ );

    wvltctio_.ctxt.forread = false;
    outputwvltfld_ = new uiIOObjSel( this, wvltctio_, "Output wavelet" );
    outputwvltfld_->attach( alignedBelow, wtlengthfld_ );

    finaliseDone.notify( mCB(this,uiWaveletExtraction,choiceSel) );
}     


uiWaveletExtraction::~uiWaveletExtraction()
{
    delete sd_;
    delete &seisctio_; delete &wvltctio_;
}


void uiWaveletExtraction::choiceSel( CallBacker* )
{
    zrangefld_->display( zextraction_->getBoolValue() );
    surfacesel_->display( !zextraction_->getBoolValue() );
}


void uiWaveletExtraction::inputSel( CallBacker* )
{
    CubeSampling cs;
    SeisIOObjInfo si( seisselfld_->ioobj() );
    si.getRanges( cs );
    subselfld_->setInput( *seisselfld_->ioobj() );
    if ( zextraction_->getBoolValue() )
    {
	zrangefld_->setRangeLimits( cs.zrg );
    	zrangefld_->setRange( cs.zrg );
    }
} 


bool uiWaveletExtraction::acceptOK( CallBacker* )
{
    if ( !seisselfld_->ioobj() || !outputwvltfld_->ioobj() )
	return false;

    wvltsize_ = mNINT( wtlengthfld_->getfValue() /
	    		      (SI().zStep() * SI().zFactor()) ) + 1 ;

    IOPar inputpars, surfacepars;
    seisselfld_->fillPar( inputpars );
    outputwvltfld_->fillPar( inputpars );
    subselfld_->fillPar( inputpars );

    if ( zextraction_->getBoolValue() )
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	int range = 1 + mNINT( (zrg.stop - zrg.start) / SI().zStep() );
	if ( range < wvltsize_ )
	{
	    uiMSG().message( "Time window size should be more",
		   	     " than Wavelet Size" );
	    return false;
	}
        inputpars.set( sKey::ZRange, zrg );
    }

    else if ( !surfacesel_->fillPar(surfacepars) )
	return false;

    if ( wvltsize_ < 3 )
    {
	uiMSG().error( "Minimum 3 samples are required to create Wavelet" );
	wtlengthfld_->setValue( 120 );
	return false;
    }

    if ( wvltphasefld_->getfValue()<0 || wvltphasefld_->getfValue() >360 )
    {
	uiMSG().error( "Please enter Phase between 0-360" );
	wvltphasefld_->setValue( 0 );
	return false;
    }

    if ( outputwvltfld_->existingTyped() )
	outputwvltfld_->setConfirmOverwrite( true );

    doProcess( inputpars, surfacepars );
    return false;
}


bool uiWaveletExtraction::doProcess( const IOPar& rangepar,
				     const IOPar& surfacepar )
{
    int phase = mNINT(wvltphasefld_->getfValue());

    if ( !readInputData( rangepar, surfacepar ) || !sd_ )
	return false;

    WaveletExtract extract( seisctio_.ioobj, sd_, wvltsize_ );
    extract.setOutputPhase( phase );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute( extract ) )
	return false;

    const float* wvlt = extract.getWavelet();
    if ( !wvlt ) return false;

    return storeWavelet( wvlt );
}


bool uiWaveletExtraction::fillHorizonSelData( const IOPar& rangepar,
					      const IOPar& surfacepar,
					      Seis::TableSelData& tsd )
{
    const char* extrazkey = IOPar::compKey( sKey::Surface,
	    			  	  Pos::EMSurfaceProvider::extraZKey() );
    Interval<float> extz( 0, 0 );
    if ( surfacepar.get(extrazkey,extz) )
	tsd.extendZ( extz );

    Pos::Provider3D* prov = Pos::Provider3D::make( rangepar );
    BufferString surfkey = IOPar::compKey( sKey::Surface,
	    				   Pos::EMSurfaceProvider::id1Key() );
    MultiID surf1mid, surf2mid;
    if ( !surfacepar.get(surfkey.buf(),surf1mid) )
	return false;

    surfkey = IOPar::compKey( sKey::Surface,
	    			  Pos::EMSurfaceProvider::id2Key() );
    const bool betweenhors = surfacepar.get( surfkey.buf(), surf2mid );
    betweenhors_ = betweenhors;

    if ( !betweenhors )
    {
	int size = int (1+(extz.stop-extz.start)/SI().zStep());
	if ( size < wvltsize_ )
	{
	    uiMSG().error("Time window size should be more than Wavelet size");
	    return false;
	}
    }

    uiTaskRunner dlg( this );
    EM::EMObject* emobjsingle = EM::EMM().loadIfNotFullyLoaded( surf1mid,
	    							&dlg );

    if ( emobjsingle )
	emobjsingle->ref();
    else 
	return false;

    mDynamicCastGet(EM::Horizon3D*,horizon1,emobjsingle)
    if ( !horizon1 )
    {
	uiMSG().error( "Error loading horizon" );
	return false;
    }

    if ( betweenhors )
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	EM::EMObject* emobjdouble = EM::EMM().loadIfNotFullyLoaded( surf2mid,
	       							    &dlg );

	if ( emobjdouble )
	    emobjdouble->ref();
	else 
	    return false;

	mDynamicCastGet( EM::Horizon3D*,horizon2,emobjdouble )

	if ( !horizon2 )
	{
	    uiMSG().error( "Error loading second horizon" );
	    return false;
	}

	EM::SectionID sid2 = horizon2->sectionID( 0 );

	BinIDValueSet& bvs = tsd.binidValueSet();
	bvs.allowDuplicateBids( true );
	horizon1->geometry().fillBinIDValueSet( sid, bvs, prov );
	horizon2->geometry().fillBinIDValueSet( sid2, bvs, prov );

	emobjdouble->unRef();
    }

    else
    {
	EM::SectionID sid = horizon1->sectionID( 0 );
	horizon1->geometry().fillBinIDValueSet( sid,tsd.binidValueSet(),prov );
    }

    emobjsingle->unRef();
    
    return true;
}


bool uiWaveletExtraction::readInputData( const IOPar& rangepar,
					 const IOPar& surfacepar )
{
    if ( zextraction_->getBoolValue() )
    {
	sd_ = Seis::SelData::get( rangepar );
	if ( !sd_ ) return false;

	StepInterval<float> zrg = zrangefld_->getRange();
    	sd_->setZRange( zrg );
    }

    else
    {
	Seis::TableSelData* tsd = new Seis::TableSelData;
	if ( !fillHorizonSelData( rangepar, surfacepar, *tsd ) )
	    return false;
	sd_ = tsd;
    }

    return true;
}


bool uiWaveletExtraction::storeWavelet( const float* vals )
{
    Wavelet wvlt( outputwvltfld_->getInput(), -wvltsize_/2, SI().zStep() );
    wvlt.reSize( wvltsize_ );
    for( int idx=0; idx<wvltsize_; idx++ )
	wvlt.samples()[idx] = vals[idx];
    
    wvlt.put( wvltctio_.ioobj );
    extractionDone.trigger();
    return true;
}


MultiID uiWaveletExtraction::storeKey() const
{
    return wvltctio_.ioobj ? wvltctio_.ioobj->key() : MultiID("");
}
