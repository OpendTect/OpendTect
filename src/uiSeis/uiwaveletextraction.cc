/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwaveletextraction.h"

#include "binidvalset.h"
#include "bufstring.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfaceposprov.h"
#include "iopar.h"
#include "od_helpids.h"
#include "posprovider.h"
#include "ptrman.h"
#include "seisioobjinfo.h"
#include "seisselectionimpl.h"
#include "survinfo.h"
#include "trckeyzsampling.h"

#include "uigeninput.h"
#include "uimsg.h"
#include "uiposprovgroup.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiseiswvltsel.h"
#include "uiselsurvranges.h"
#include "uitaskrunner.h"
#include "wavelet.h"
#include "waveletextractor.h"


uiWaveletExtraction::uiWaveletExtraction( uiParent* p, bool is2d )
    : uiDialog(p,Setup(tr("Extract Wavelet"),mNoDlgTitle,
			mODHelpKey(mWaveletExtractionHelpID) ).modal(false) )
    , extractionDone(this)
    , seisselfld_(0)
    , linesel2dfld_(0)
    , subselfld3d_(0)
    , zrangefld_(0)
    , seldata_(0)
    , datastep_(SI().zStep())
    , wvltsize_(0)
{
    setCtrlStyle( RunAndClose );
    const Seis::GeomType gt = Seis::geomTypeOf( is2d, false );
    seisselfld_ = new uiSeisSel( this, uiSeisSel::ioContext(gt,true),
				 uiSeisSel::Setup(gt) );
    mAttachCB( seisselfld_->selectionDone, uiWaveletExtraction::inputSelCB );
    if ( !is2d )
    {
	subselfld3d_ = new uiSeis3DSubSel( this, Seis::SelSetup(false,false)
					         .onlyrange(true)
					         .withstep(true)
					         .withoutz(true) );
	subselfld3d_->attach( alignedBelow, seisselfld_ );
    }
    else
    {
	linesel2dfld_ = new uiSeis2DMultiLineSel(this,uiStrings::sEmptyString(),
						 false,true);
	linesel2dfld_->attach( alignedBelow, seisselfld_ );
	mAttachCB( linesel2dfld_->selectionChanged,
		   uiWaveletExtraction::lineSelCB );
    }

    createCommonUIFlds();
    mAttachCB( postFinalize(), uiWaveletExtraction::choiceSelCB );
}


void uiWaveletExtraction::createCommonUIFlds()
{
    zextraction_ = new uiGenInput( this, tr("Vertical Extraction"),
			BoolInpSpec(linesel2dfld_,tr("Z range"),
				    uiStrings::sHorizon(mPlural)) );
    mAttachCB( zextraction_->valueChanged, uiWaveletExtraction::choiceSelCB );

    linesel2dfld_ ? zextraction_->attach( alignedBelow, linesel2dfld_ )
		  : zextraction_->attach( alignedBelow, subselfld3d_ );

    zrangefld_ = new uiSelZRange( this, false );
    zrangefld_->setLabel( uiStrings::sZRange() );
    zrangefld_->attach( alignedBelow, zextraction_ );

    surfacesel_ = uiPosProvGroup::factory().create( sKey::Surface(), this,
			 uiPosProvGroup::Setup(linesel2dfld_,false,true) );
    surfacesel_->attach( alignedBelow, zextraction_ );

    uiString lbl = tr("Wavelet length %1").arg(SI().getUiZUnitString());
    wtlengthfld_ = new uiGenInput( this, lbl, IntInpSpec(120) );
    wtlengthfld_->attach( alignedBelow, surfacesel_ );

    uiString taperlbl = tr("Taper length %1").arg(SI().getUiZUnitString());
    taperfld_ = new uiGenInput( this, taperlbl, IntInpSpec(20) );
    taperfld_->attach( alignedBelow, wtlengthfld_ );

    wvltphasefld_ = new uiGenInput( this, tr("Phase (Degrees)"),
				    IntInpSpec(0) );
    wvltphasefld_->attach( alignedBelow, taperfld_ );

    outputwvltfld_ = new uiWaveletSel( this, false );
    outputwvltfld_->attach( alignedBelow, wvltphasefld_ );

    mAttachCB( postFinalize(), uiWaveletExtraction::inputSelCB );
}


uiWaveletExtraction::~uiWaveletExtraction()
{
    detachAllNotifiers();
    delete seldata_;
}


void uiWaveletExtraction::choiceSelCB( CallBacker* )
{
    zrangefld_->display( zextraction_->getBoolValue() );
    surfacesel_->display( !zextraction_->getBoolValue() );
    if ( !zextraction_->getBoolValue() )
    {
	Interval<float> defextraz( -.2, .2 );
	IOPar extrazpar;
	extrazpar.set( "Surface.Extra Z", defextraz );
	surfacesel_->usePar( extrazpar );
    }
}


void uiWaveletExtraction::inputSelCB( CallBacker* )
{
    const IOObj* ioobj = seisselfld_->ioobj( true );
    if ( !ioobj )
	return;

    if ( linesel2dfld_ )
    {
	linesel2dfld_->setInput( ioobj->key() );
	return;
    }

    TrcKeyZSampling cs;
    if ( subselfld3d_ )
    {
	const SeisIOObjInfo si( ioobj );
	si.getRanges( cs );
	cs.hsamp_.step_.inl() = cs.hsamp_.step_.crl() = 10;
	subselfld3d_->uiSeisSubSel::setInput( cs );
	datastep_ = cs.zsamp_.step;
    }

    if ( zextraction_->getBoolValue() )
    {
	zrangefld_->setRangeLimits( cs.zsamp_ );
	zrangefld_->setRange( cs.zsamp_ );
    }
}


void uiWaveletExtraction::lineSelCB( CallBacker* )
{
    if ( !zextraction_->getBoolValue() )
	return;

    const SeisIOObjInfo si( seisselfld_->ioobj() );
    StepInterval<int> trcrg;
    StepInterval<float> commonzrg;
    TypeSet<Pos::GeomID> geomids;
    linesel2dfld_->getSelGeomIDs( geomids );
    for ( int idx=0; idx<geomids.size(); idx++ )
    {
	StepInterval<float> zrg( 0, 0, 1 );
	if ( !si.getRanges(geomids[idx],trcrg,zrg) )
	    return;

	if ( idx==0 )
	{
	    commonzrg = zrg;
	    datastep_ = zrg.step;
	}
	else
	{
	    if ( datastep_ != zrg.step )
	    {
		uiMSG().message( tr("Selected lines having different sample"
				    " intervals\n Please change selection") );
		return;
	    }

	    commonzrg.limitTo( zrg );
	}
    }

    if ( !geomids.isEmpty() && commonzrg.nrSteps() == 0 )
    {
	uiMSG().message( tr("No common Z Range in selected lines.\n"
			    "Please change selection") );
	return;
    }

    zrangefld_->setRangeLimits( commonzrg );
    zrangefld_->setRange( commonzrg );
}


bool uiWaveletExtraction::acceptOK( CallBacker* )
{
    const IOObj* seisioobj = seisselfld_->ioobj();
    if ( !seisioobj )
	return false;

    if ( outputwvltfld_->isEmpty() )
    {
	uiMSG().message( uiStrings::phrSpecifyOutput()
			 .append(uiStrings::sWavelet() ));
	return false;
    }

    const IOObj* wvltioobj = outputwvltfld_->ioobj();
    if ( !wvltioobj )
	return false;

    if ( linesel2dfld_ && !check2DFlds() )
	return false;

    IOPar inputpars, surfacepars;
    seisselfld_->fillPar( inputpars );

    outputwvltfld_->fillPar( inputpars );

    if ( subselfld3d_ )
	subselfld3d_->fillPar( inputpars );

    if ( !checkWaveletSize() )
	return false;

    if ( !zextraction_->getBoolValue() && !surfacesel_->fillPar(surfacepars) )
	    return false;

    const int taperlen = taperfld_->getIntValue();
    const int wvltlen = wtlengthfld_->getIntValue();
    if ( (2*taperlen > wvltlen) || taperlen < 0 )
    {
	uiMSG().error( tr("TaperLength should be in between\n"
			  "0 and half the Wavelet Length") );
	taperfld_->setValue( 5 );
	return false;
    }

    if ( wvltphasefld_->getIntValue()<-180 || wvltphasefld_->getIntValue()>180)
    {
	uiMSG().error( tr("Please enter Phase between -180 and 180") );
	wvltphasefld_->setValue( 0 );
	return false;
    }

    if ( outputwvltfld_->existingTyped() )
	outputwvltfld_->setConfirmOverwrite( true );

    doProcess( *seisioobj, *wvltioobj, inputpars, surfacepars );
    return false;
}


bool uiWaveletExtraction::checkWaveletSize()
{
    wvltsize_ = mNINT32( wtlengthfld_->getIntValue() /
		      (datastep_ * ((float) SI().zDomain().userFactor()))) + 1;
    if ( wvltsize_ < 3 )
    {
	uiMSG().error( tr("Minimum 3 samples are required to create Wavelet") );
	wtlengthfld_->setValue( 120 );
	return false;
    }

    if ( zextraction_->getBoolValue() )
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	const int range = 1 + mNINT32( (zrg.stop - zrg.start) / datastep_ );
	if ( range < wvltsize_ )
	{
	    uiMSG().message( tr("Selection window size should be more"
				" than Wavelet Size") );
	    return false;
	}
    }

    return true;
}


bool uiWaveletExtraction::check2DFlds()
{
    if ( !linesel2dfld_->nrSelected() )
    {
	uiMSG().error( tr("Select at least one line") );
	return false;
    }

    if ( linesel2dfld_ && !zextraction_->getBoolValue() )
    {
	uiMSG().message( tr("Extraction of wavelet on/between 2D-horizon(s)"
			    " is not implemented") );
	return false;
    }

    return true;
}


bool uiWaveletExtraction::doProcess( const IOObj& seisioobj,
	const IOObj& wvltioobj, const IOPar& rangepar, const IOPar& surfacepar )
{
    const int phase = wvltphasefld_->getIntValue();
    PtrMan<WaveletExtractor> extractor= new WaveletExtractor( seisioobj,
							      wvltsize_ );
    if ( !linesel2dfld_ )
    {
	if ( !getSelData(rangepar,surfacepar) || !seldata_ )
	    return false;

	extractor->setSelData( *seldata_ );
    }
    else
    {
	StepInterval<float> zrg = zrangefld_->getRange();
	Seis::RangeSelData range;
	range.setZRange( zrg );
	Interval<int> inlrg( 0, 0 );
	range.cubeSampling().hsamp_.setInlRange( inlrg );

	ObjectSet<Seis::SelData> sdset;
	TypeSet<Pos::GeomID> geomids;
	linesel2dfld_->getSelGeomIDs( geomids );
	for ( int lidx=0; lidx<geomids.size(); lidx++ )
	{
	    const Pos::GeomID geomid = geomids[lidx];
	    range.cubeSampling().hsamp_.setCrlRange(
					linesel2dfld_->getTrcRange(geomid) );
	    range.setGeomID( geomid );
	    seldata_ = range.clone();
	    sdset += seldata_;
	}

	extractor->setSelData( sdset );
    }

    const int taperlength = taperfld_->getIntValue();
    const float val =
	  1-(2*taperlength/( (wvltsize_-1)*datastep_*
		      ((float)SI().zDomain().userFactor())) );
    const float paramval = (float) ( val == 1 ? 1.0 - 1e-6 : val );
    extractor->setTaperParamVal( paramval );
    extractor->setPhase( phase );

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *extractor ) )
	return false;

    Wavelet storewvlt = extractor->getWavelet();
    storewvlt.put( &wvltioobj );
    extractionDone.trigger();

    return true;
}


bool uiWaveletExtraction::fillHorizonSelData( const IOPar& rangepar,
					      const IOPar& surfacepar,
					      Seis::TableSelData& tsd )
{
    const char* extrazkey = IOPar::compKey( sKey::Surface(),
					  Pos::EMSurfaceProvider::extraZKey());
    Interval<float> extz( 0, 0 );
    if ( surfacepar.get(extrazkey,extz) )
	tsd.extendZ( extz );

    Pos::Provider3D* prov = Pos::Provider3D::make( rangepar );
    BufferString surfkey = IOPar::compKey( sKey::Surface(),
					   Pos::EMSurfaceProvider::id1Key() );
    MultiID surf1mid, surf2mid;
    if ( !surfacepar.get( surfkey.buf(), surf1mid ) )
	return false;

    surfkey = IOPar::compKey( sKey::Surface(),
			      Pos::EMSurfaceProvider::id2Key() );
    const bool betweenhors = surfacepar.get( surfkey.buf(), surf2mid );

    if ( !betweenhors )
    {
	const int size = int ( 1+(extz.stop-extz.start)/datastep_ );
	if ( size < wvltsize_ )
	{
	    uiMSG().error( tr("Selection window size should be"
		              " more than Wavelet size") );
	    return false;
	}
    }

    uiTaskRunner dlg( this );
    EM::EMObject* emobjsinglehor =
	EM::EMM().loadIfNotFullyLoaded( surf1mid, &dlg );

    if ( !emobjsinglehor )
	return false;

    emobjsinglehor->ref();
    mDynamicCastGet(EM::Horizon3D*,horizon1,emobjsinglehor)
    if ( !horizon1 )
    {
	uiMSG().error( tr("Error loading horizon") );
	return false;
    }

    if ( betweenhors )
    {
	EM::EMObject* emobjdoublehor =
	    EM::EMM().loadIfNotFullyLoaded( surf2mid, &dlg );

	if ( !emobjdoublehor )
	    return false;

	emobjdoublehor->ref();
	mDynamicCastGet( EM::Horizon3D*, horizon2,emobjdoublehor )
	if ( !horizon2 )
	{
	    uiMSG().error( tr("Error loading second horizon") );
	    return false;
	}

	BinIDValueSet& bvs = tsd.binidValueSet();
	bvs.allowDuplicateBinIDs( true );
	horizon1->geometry().fillBinIDValueSet( bvs, prov );
	horizon2->geometry().fillBinIDValueSet( bvs, prov );
	emobjdoublehor->unRef();
    }
    else
    {
	horizon1->geometry().fillBinIDValueSet( tsd.binidValueSet(), prov );
    }

    emobjsinglehor->unRef();

    return true;
}


bool uiWaveletExtraction::getSelData( const IOPar& rangepar,
				      const IOPar& surfacepar )
{
    if ( zextraction_->getBoolValue() )
    {
	if ( !linesel2dfld_ )
	{
	    seldata_ = Seis::SelData::get( rangepar );
	    if ( !seldata_ ) return false;

	    StepInterval<float> zrg = zrangefld_->getRange();
	    seldata_->setZRange( zrg );
	}
    }
    else
    {
	Seis::TableSelData* tsd = new Seis::TableSelData;
	if ( !fillHorizonSelData( rangepar, surfacepar, *tsd ) )
	    return false;

	seldata_ = tsd;
    }

    return true;
}


MultiID uiWaveletExtraction::storeKey() const
{
    const IOObj* wvltioobj = outputwvltfld_->ioobj( true );
    return wvltioobj ? wvltioobj->key() : MultiID("");
}
