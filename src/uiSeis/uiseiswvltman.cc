/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseiswvltman.cc,v 1.33 2009-03-19 09:01:55 cvsbert Exp $";


#include "uiseiswvltman.h"
#include "uiseiswvltimp.h"
#include "wavelet.h"
#include "ioobj.h"
#include "ioman.h"
#include "iostrm.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "color.h"
#include "oddirs.h"
#include "dirlist.h"
#include "survinfo.h"
#include "statruncalc.h"
#include "arrayndimpl.h"
#include "flatposdata.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uitextedit.h"
#include "uiflatviewer.h"
#include "uilistbox.h"
#include "uiselsimple.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSeisWvltMan::uiSeisWvltMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Wavelet management",
                                     "Manage wavelets",
                                     "103.3.0").nrstatusflds(1),
	    	   WaveletTranslatorGroup::ioContext(), sKey::DefWavelet )
    , curid_(DataPack::cNoID())
{
    createDefaultUI();

    selgrp->getManipGroup()->addButton( "wvltfromothsurv.png",
	mCB(this,uiSeisWvltMan,getFromOtherSurvey), "Get from other survey" );

    uiGroup* butgrp = new uiGroup( this, "Imp/Create buttons" );
    uiPushButton* impbut = new uiPushButton( butgrp, "&Import", false );
    impbut->activated.notify( mCB(this,uiSeisWvltMan,impPush) );
    impbut->setPrefWidthInChar( 15 );
    uiPushButton* crbut = new uiPushButton( butgrp, "&Generate", false );
    crbut->activated.notify( mCB(this,uiSeisWvltMan,crPush) );
    crbut->attach( rightOf, impbut );
    crbut->setPrefWidthInChar( 15 );
    butgrp->attach( centeredBelow, selgrp );

    wvltfld = new uiFlatViewer( this );
    FlatView::Appearance& app = wvltfld->appearance();
    app.annot_.x1_.name_ = "Amplitude";
    app.annot_.x2_.name_ = SI().zIsTime() ? "Time" : "Depth";
    app.annot_.setAxesAnnot( false );
    app.setGeoDefaults( true );
    app.ddpars_.show( true, false );
    app.ddpars_.wva_.overlap_ = 0;
    app.ddpars_.wva_.clipperc_.start = app.ddpars_.wva_.clipperc_.stop = 0;
    app.ddpars_.wva_.left_ = Color( 250, 250, 0 );
    app.ddpars_.wva_.mid_ = Color( 150, 150, 150 );
    app.ddpars_.wva_.symmidvalue_ = mUdf(float);
    app.setDarkBG( false );

    wvltfld->setPrefWidth( 60 );
    wvltfld->attach( ensureRightOf, selgrp );
    wvltfld->setStretch( 1, 2 );
    wvltfld->setExtraBorders( uiRect(2,5,2,5) );

    infofld->attach( ensureBelow, butgrp );
    infofld->attach( ensureBelow, wvltfld );
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    selChg(0);
}


uiSeisWvltMan::~uiSeisWvltMan()
{
}


void uiSeisWvltMan::impPush( CallBacker* )
{
    uiSeisWvltImp dlg( this );
    if ( dlg.go() )
	selgrp->fullUpdate( dlg.selKey() );
}


class uiSeisWvltManCrWvlt : public uiDialog
{
public:

uiSeisWvltManCrWvlt( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Create Wavelet",
				 "Specify wavelet creation parameters",
				 "103.3.2"))
    , ctio_(*mMkCtxtIOObj(Wavelet))
{
    isrickfld = new uiGenInput( this, "Wavelet type",
				BoolInpSpec(true,"Ricker","Sinc") );

    const float sisr = SI().zStep();
    float deffrq = 0.1 / sisr; int ideffr = mNINT(deffrq);
    if ( ideffr > 0 && mIsZero(deffrq-ideffr,1e-4) )
	deffrq = ideffr; // avoid awkward 99.999 display
    BufferString txt( "Central " );
    txt += SI().zIsTime() ? "Frequency" : "Wavenumber";
    freqfld = new uiGenInput( this, txt, FloatInpSpec(deffrq) );
    freqfld->attach( alignedBelow, isrickfld );

    const float usrsr = sisr * SI().zFactor();
    txt = "Sample interval "; txt += SI().getZUnitString();
    srfld = new uiGenInput( this, txt, FloatInpSpec(usrsr) );
    srfld->attach( alignedBelow, freqfld );

    peakamplfld = new uiGenInput( this, "Peak amplitude", FloatInpSpec(1) );
    peakamplfld->attach( alignedBelow, srfld );

    ctio_.ctxt.forread = false;
    wvltfld = new uiIOObjSel( this, ctio_ );
    wvltfld->attach( alignedBelow, peakamplfld );
}

~uiSeisWvltManCrWvlt()
{
    delete ctio_.ioobj; delete &ctio_;
}

#define mErrRet(s) { uiMSG().error(s); return false; }

bool acceptOK( CallBacker* )
{
    if ( !wvltfld->commitInput(true) )
	mErrRet( "Please enter a name for the new Wavelet" );

    const float sr = srfld->getfValue();
    const float peakampl = peakamplfld->getfValue();
    const float freq = freqfld->getfValue();

    if ( mIsUdf(sr) || sr <= 0 )
	mErrRet( "The sample interval is not valid" )
    else if ( peakampl == 0 )
	mErrRet( "The peak amplitude must be non-zero" )
    else if ( mIsUdf(freq) || freq <= 0 )
	mErrRet( "The frequency must be positive" )

    const float realsr = sr / SI().zFactor();
    Wavelet wvlt( isrickfld->getBoolValue(), freq, realsr, peakampl );
    if ( !wvlt.put(ctio_.ioobj) )
    {
	mErrRet( "Cannot write wavelet" )
	return false;
    }

    return true;
}

    CtxtIOObj&		ctio_;
    uiGenInput*		isrickfld;
    uiGenInput*		freqfld;
    uiGenInput*		srfld;
    uiGenInput*		peakamplfld;
    uiIOObjSel*		wvltfld;

};


void uiSeisWvltMan::crPush( CallBacker* )
{
    uiSeisWvltManCrWvlt dlg( this );
    if ( !dlg.go() ) return;

    selgrp->fullUpdate( dlg.ctio_.ioobj->key() );
}


void uiSeisWvltMan::mkFileInfo()
{
    BufferString txt;
    Wavelet* wvlt = Wavelet::get( curioobj_ );

    wvltfld->removePack( curid_ );
    curid_ = DataPack::cNoID();
    if ( wvlt )
    {
	const int wvltsz = wvlt->size();
	const float zfac = SI().zFactor();

	Array2DImpl<float>* fva2d = new Array2DImpl<float>( 1, wvltsz );
	FlatDataPack* dp = new FlatDataPack( "Wavelet", fva2d );
	memcpy( fva2d->getData(), wvlt->samples(), wvltsz * sizeof(float) );
	dp->setName( wvlt->name() );
	DPM( DataPackMgr::FlatID() ).add( dp );
	curid_ = dp->id();
	StepInterval<double> posns; posns.setFrom( wvlt->samplePositions() );
	if ( SI().zIsTime() ) posns.scale( zfac );
	dp->posData().setRange( false, posns );

	Stats::RunCalc<float> rc( Stats::RunCalcSetup().require(Stats::Max) );
	rc.addValues( wvltsz, wvlt->samples() );

	BufferString tmp;
	tmp += "Number of samples: "; tmp += wvlt->size(); tmp += "\n";
	tmp += "Sample interval "; tmp += SI().getZUnitString(true);tmp += ": ";
	tmp += wvlt->sampleRate() * zfac; tmp += "\n";
	tmp += "Min/Max amplitude: ";
	tmp += rc.min(); tmp += "/"; tmp += rc.max(); tmp += "\n";
	txt += tmp;

	delete wvlt;
    }

    wvltfld->setPack( true, curid_, false );
    wvltfld->handleChange( FlatView::Viewer::All );

    txt += getFileInfo();
    infofld->setText( txt );
}

#define mRet(s) \
	{ ctio.setObj(0); delete &ctio; if ( s ) uiMSG().error(s); return; }

void uiSeisWvltMan::getFromOtherSurvey( CallBacker* )
{
    CtxtIOObj& ctio = *mMkCtxtIOObj(Wavelet);
    const BufferString basedatadir( GetBaseDataDir() );

    PtrMan<DirList> dirlist = new DirList( basedatadir, DirList::DirsOnly );
    dirlist->sort();

    uiSelectFromList::Setup setup( "Survey", *dirlist );
    setup.dlgtitle( "Select survey" );
    uiSelectFromList dlg( this, setup );
    if ( !dlg.go() ) return;

    FilePath fp( basedatadir ); fp.add( dlg.selFld()->getText() );
    const BufferString tmprootdir( fp.fullPath() );
    if ( !File_exists(tmprootdir) ) mRet("Survey doesn't seem to exist")
    const BufferString realrootdir( GetDataDir() );

    // No returns from here ...
    IOM().setRootDir( tmprootdir );

    ctio.ctxt.forread = true;

    uiIOObjSelDlg objdlg( this, ctio );
    Wavelet* wvlt = 0;
    bool didsel = true;
    if ( !objdlg.go() || !objdlg.ioObj() )
	didsel = false;
    else
    {
	ctio.setObj( objdlg.ioObj()->clone() );
	wvlt = Wavelet::get( ctio.ioobj );
	ctio.setName( ctio.ioobj->name() );
	ctio.setObj( 0 );
    }

    IOM().setRootDir( realrootdir );
    // ... to here

    if ( !wvlt )
	mRet((didsel?"Could not read wavelet":0))
    IOM().getEntry( ctio );
    if ( !ctio.ioobj )
	mRet("Cannot create new entry in Object Management")
    else if ( !wvlt->put(ctio.ioobj) )
	mRet("Cannot write wavelet to disk")

    selgrp->fullUpdate( ctio.ioobj->key() );
    mRet( 0 )
}
