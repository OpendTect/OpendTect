/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpsegyvsp.cc,v 1.3 2009-01-12 17:04:49 cvsbert Exp $";

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
    const bool ist = inpIsTime();
    StepInterval<float> outsamp( outsampfld_->getFStepInterval() );

    
    return false;
}
