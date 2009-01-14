/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpsegyvsp.cc,v 1.4 2009-01-14 17:09:40 cvsbert Exp $";

#include "uiwellimpsegyvsp.h"

#include "uicompoundparsel.h"
#include "uisegydefdlg.h"
#include "uisegyexamine.h"
#include "uisegyread.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uiselsimple.h"
#include "uibutton.h"
#include "uimsg.h"

#include "seistrc.h"
#include "seisread.h"
#include "segytr.h"
#include "welltransl.h"
#include "welldata.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "wellman.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "keystrs.h"
#include "survinfo.h"
#include "unitofmeasure.h"


class uiSEGYVSPBasicPars : public uiCompoundParSel
{
public:

uiSEGYVSPBasicPars( uiWellImportSEGYVSP* p )
    : uiCompoundParSel( p, "SEG-Y input" )
    , imp_(*p)
    , fnm_("-")
{
    butPush.notify( mCB(this,uiSEGYVSPBasicPars,selPush) );
    finaliseDone.notify( mCB(this,uiSEGYVSPBasicPars,doSel) );
}

void selPush( CallBacker* )
{
    uiSEGYDefDlg::Setup su; su.defgeom_ = Seis::Line;
    su.geoms_ += su.defgeom_;
    uiSEGYDefDlg* defdlg = new uiSEGYDefDlg( &imp_, su, imp_.sgypars_ );
    defdlg->readParsReq.notify( mCB(this,uiSEGYVSPBasicPars,readParsReq) );
    const bool dlgok = defdlg->go();
    const int nrexam = dlgok ? defdlg->nrTrcExamine() : 0;
    delete defdlg;
    if ( !dlgok ) return;

    FilePath fp( imp_.sgypars_.find(sKey::FileName) );
    fnm_ = fp.fileName();
    uiSEGYExamine::Setup exsu( nrexam );
    exsu.modal( false ); exsu.usePar( imp_.sgypars_ );
    if ( nrexam > 0 )
	uiSEGYExamine::launch( exsu );

    BufferString emsg;
    PtrMan<SeisTrcReader> rdr = uiSEGYExamine::getReader( exsu, emsg );
    if ( !rdr )
    {
	if ( !emsg.isEmpty() )
	    uiMSG().warning( emsg );
	return;
    }

    mDynamicCastGet(SEGYSeisTrcTranslator*,tr,rdr->translator())
    if ( !tr ) return;

    SeisTrc trc;
    if ( !tr->read(trc) ) return;
    imp_.use( trc );
}

void readParsReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYDefDlg*,dd,cb)
    if ( !dd ) return;

    PtrMan<CtxtIOObj> ctio = uiSEGYRead::getCtio( true, Seis::Line );
    ctio->ctxt.parconstraints.clear(); ctio->ctxt.allowcnstrsabsent = true;
    uiIOObjSelDlg dlg( dd, *ctio, "Select SEG-Y setup" );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    imp_.sgypars_.merge( ioobj->pars() );
    dd->use( ioobj, false );
}

BufferString getSummary() const
{
    return fnm_;
}

    uiWellImportSEGYVSP& imp_;
    BufferString	fnm_;

};


uiWellImportSEGYVSP::uiWellImportSEGYVSP( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Zero-offset VSP",
				 "Import Zero-offset VSP as Well Log",
				 mTODOHelpID) )
    , istimefld_(0)
    , unitfld_(0)
    , ctio_(*mMkCtxtIOObj(Well))
    , dispinpsamp_(mUdf(float),1)
{
    const bool definft = SI().xyInFeet();
    setCtrlStyle( DoAndStay );

    bparsfld_ = new uiSEGYVSPBasicPars( this );
    const FloatInpIntervalSpec fspec( false );
    inpsampfld_ = new uiGenInput( this,
			"Overrule Input Sampling (start/step)", fspec );
    inpsampfld_->attach( alignedBelow, bparsfld_ );
    inpsampfld_->setWithCheck( true );

    const bool zist = SI().zIsTime();
    uiGenInput* uigi;
    if ( zist )
    {
	istimefld_ = uigi = new uiGenInput( this, "Data is in ",
				     BoolInpSpec(true,"Time","Depth") );
	istimefld_->valuechanged.notify(
				mCB(this,uiWellImportSEGYVSP,isTimeChg) );
    }
    else
    {
	ObjectSet<const UnitOfMeasure> uns;
	UoMR().getRelevant( PropertyRef::Dist, uns );
	BufferStringSet unnms;
	for ( int idx=0; idx<uns.size(); idx++ )
	    unnms.add( uns[idx]->name() );
	unitfld_ = uigi = new uiGenInput( this, "Input sampling is in",
				   StringListInpSpec(unnms) );
    }
    uigi->attach( alignedBelow, inpsampfld_ );
    inpinftfld_ = new uiCheckBox( this, "Feet" );
    if ( definft ) inpinftfld_->setChecked( true );
    inpinftfld_->attach( rightOf, uigi );
    istvdfld_ = new uiGenInput( this, "", BoolInpSpec(false,"TVDSS","MD") );
    istvdfld_->attach( rightOf, inpinftfld_ );

    uiSeparator* sep = new uiSeparator( this, "hor sep", true, false );
    sep->attach( stretchedBelow, uigi );

    wellfld_ = new uiIOObjSel( this, ctio_, "Add to Well" );
    wellfld_->attach( alignedBelow, uigi );
    wellfld_->attach( ensureBelow, sep );
    wellfld_->selectiondone.notify( mCB(this,uiWellImportSEGYVSP,wllSel) );

    lognmfld_ = new uiGenInput( this, "Output log name", "VSP" );
    lognmfld_->attach( alignedBelow, wellfld_ );
    uiPushButton* but = new uiPushButton( this, "Se&lect",
	    		mCB(this,uiWellImportSEGYVSP,selLogNm), false );
    but->attach( rightOf, lognmfld_ );

    const FloatInpIntervalSpec fispec( true );
    outsampfld_ = new uiGenInput( this, "Resample output to", fispec );
    outsampfld_->attach( alignedBelow, lognmfld_ );
    outsampfld_->setWithCheck( true );
    outsampfld_->checked.notify( mCB(this,uiWellImportSEGYVSP,outSampChk) );
    outinftfld_ = new uiCheckBox( this, "Feet" );
    if ( definft ) outinftfld_->setChecked( true );
    outinftfld_->attach( rightOf, outsampfld_ );

    finaliseDone.notify( mCB(this,uiWellImportSEGYVSP,isTimeChg) );
}


uiWellImportSEGYVSP::~uiWellImportSEGYVSP()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiWellImportSEGYVSP::inpIsTime() const
{
    return istimefld_ && istimefld_->getBoolValue();
}


void uiWellImportSEGYVSP::wllSel( CallBacker* )
{
    if ( !ctio_.ioobj ) return;
    existinglognms_.deepErase();
    const char* nm = Well::IO::getMainFileName( *ctio_.ioobj );
    if ( !nm || !*nm ) return;
    Well::Data wd; Well::Reader wr( nm, wd );
    wr.getLogInfo( existinglognms_ );
}


void uiWellImportSEGYVSP::selLogNm( CallBacker* )
{
    uiGetObjectName::Setup su( "Specify output log name", existinglognms_ );
    su.inptxt( "Name for VSP log" ).deflt( lognmfld_->text() );
    if ( su.deflt_.isEmpty() ) su.deflt_ = "VSP";
    uiGetObjectName dlg( this, su );
    if ( dlg.go() )
	lognmfld_->setText( dlg.text() );
}


#define mSetSDFldVals(fld,sd,nr) \
	{ fld->setValue( sd.start, 0 ); fld->setValue( sd.step, nr ); }


void uiWellImportSEGYVSP::use( const SeisTrc& trc )
{
    const bool ist = inpIsTime();
    dispinpsamp_ = trc.info().sampling;
    dispinpsamp_.start *= 1000; dispinpsamp_.step *= 1000;
    mSetSDFldVals( inpsampfld_, dispinpsamp_, 1 );
    if ( !ist )
	mSetSDFldVals( outsampfld_, dispinpsamp_, 2 );
}


void uiWellImportSEGYVSP::isTimeChg( CallBacker* )
{
    const bool isdpth = !inpIsTime();
    istvdfld_->display( isdpth );
    inpinftfld_->display( isdpth );
    outSampChk( 0 );
}


void uiWellImportSEGYVSP::outSampChk( CallBacker* )
{
    outinftfld_->display( outsampfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiWellImportSEGYVSP::acceptOK( CallBacker* )
{
    if ( bparsfld_->fnm_.isEmpty() || sgypars_.isEmpty() )
	mErrRet("Please define the input SEG-Y")
    if ( !wellfld_->commitInput(false) )
	mErrRet("Please select the output well")
    const BufferString lognm( lognmfld_->text() );
    if ( lognm.isEmpty() )
	mErrRet("Please enter a valid name for the new log")

    SamplingData<float> inpsamp( mUdf(float), mUdf(float) );
    StepInterval<float> outsamp( mUdf(float), mUdf(float), mUdf(float) );
    if ( inpsampfld_->isChecked() )
    {
	inpsamp.start = inpsampfld_->getfValue( 0 ) * 0.001;
	inpsamp.step = inpsampfld_->getfValue( 1 ) * 0.001;
    }
    if ( outsampfld_->isChecked() )
	outsamp = outsampfld_->getFStepInterval();

    SeisTrc trc;
    if ( !fetchTrc(trc) )
	return false;
    if ( !mIsUdf(inpsamp.start) ) trc.info().sampling.start = inpsamp.start;
    if ( !mIsUdf(inpsamp.step) ) trc.info().sampling.step = inpsamp.step;

    createLog( trc, outsamp, lognm );
    return false;
}


bool uiWellImportSEGYVSP::fetchTrc( SeisTrc& trc )
{
    Translator* tr = ctio_.ioobj->getTranslator();
    if ( !tr )
	mErrRet("Internal: Cannot create SEG-Y Translator")
    mDynamicCastGet(SEGYSeisTrcTranslator*,sgytr,tr)
    if ( !sgytr )
	mErrRet("Internal: not a SEG-Y object")

    sgytr->usePar( sgypars_ );
    bool rv = sgytr->read(trc);
    delete tr;
    return rv;
}


bool uiWellImportSEGYVSP::createLog( const SeisTrc& trc,
				     const StepInterval<float>& outsamp,
				     const char* lognm )
{
    const MultiID key( ctio_.ioobj->key() );
    const bool wasloaded = Well::MGR().isLoaded( key );

    Well::Data* wd = Well::MGR().get( key );
    if ( !wd )
	mErrRet("Cannot load the selected well")
    const bool ist = inpIsTime();
    if ( ist && !wd->d2TModel() )
	mErrRet("Selected well has no Depth vs Time model")

    int wlidx = wd->logs().indexOf( lognm );
    if ( wlidx >= 0 )
	delete wd->logs().remove( wlidx );
    Well::Log* wl = new Well::Log( lognm );
    wl->pars().set( sKey::FileName, sgypars_.find(sKey::FileName) );

    //TODO support all options, now just do all
    for ( int isamp=0; isamp<trc.size(); isamp++ )
    {
	float z = trc.samplePos( isamp );
	if ( ist )
	    z = wd->d2TModel()->getDepth( z );
	wl->addValue( z, trc.get(isamp,0) );
    }

    if ( wlidx >= 0 )
	wd->logs().add( wl );

    Well::Writer wtr( Well::IO::getMainFileName(*ctio_.ioobj), *wd );
    wtr.putLogs();

    if ( wasloaded )
	Well::MGR().reload( key );
    else
	Well::MGR().release( key );

    return true;
}
