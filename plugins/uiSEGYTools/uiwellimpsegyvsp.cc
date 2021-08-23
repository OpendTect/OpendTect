/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2009
_______________________________________________________________________

-*/

#include "uiwellimpsegyvsp.h"

#include "uicompoundparsel.h"
#include "uisegydefdlg.h"
#include "uisegyexamine.h"
#include "uisegyread.h"
#include "uiwellsel.h"
#include "uiioobjseldlg.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uiselsimple.h"
#include "uicombobox.h"
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
#include "welltrack.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "filepath.h"
#include "keystrs.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "od_helpids.h"


class uiSEGYVSPBasicPars : public uiCompoundParSel
{ mODTextTranslationClass(uiSEGYVSPBasicPars);
public:

uiSEGYVSPBasicPars( uiWellImportSEGYVSP* p )
    : uiCompoundParSel( p, tr("SEG-Y input") )
    , imp_(*p)
    , fnm_("-")
{
    butPush.notify( mCB(this,uiSEGYVSPBasicPars,selPush) );
    p->afterPopup.notify( mCB(this,uiSEGYVSPBasicPars,doSel) );
}

void selPush( CallBacker* )
{
    uiSEGYDefDlg::Setup su; su.defgeom_ = Seis::Line;
    su.geoms_ += su.defgeom_;
    uiSEGYDefDlg* defdlg = new uiSEGYDefDlg( &imp_, su, imp_.sgypars_ );
    defdlg->setModal( true );
    defdlg->readParsReq.notify( mCB(this,uiSEGYVSPBasicPars,readParsReq) );
    const bool dlgok = defdlg->go();
    const int nrexam = dlgok ? defdlg->nrTrcExamine() : 0;
    delete defdlg;
    if ( !dlgok ) return;

    FilePath fp( imp_.sgypars_.find(sKey::FileName()) );
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
	    uiMSG().warning( toUiString(emsg) );
	return;
    }

    mDynamicCastGet(SEGYSeisTrcTranslator*,trans,rdr->translator())
    if ( !trans ) return;

    SeisTrc trc;
    if ( !trans->read(trc) ) return;
    imp_.use( trc );
}

void readParsReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYDefDlg*,dd,cb)
    if ( !dd ) return;

    PtrMan<CtxtIOObj> ctio = uiSEGYRead::getCtio( true, Seis::Line );
    ctio->ctxt_.toselect_.require_.setEmpty();
    ctio->ctxt_.toselect_.dontallow_.setEmpty();
    uiIOObjSelDlg dlg( dd, *ctio, tr("Select SEG-Y setup") );
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
    : uiDialog(p,uiDialog::Setup(tr("Import Zero-offset VSP"),
				 mNoDlgTitle,
				 mODHelpKey(mWellImpSegyVspHelpID) ) )
    , istimefld_(0)
    , unitfld_(0)
    , dispinpsamp_(mUdf(float),1)
    , isdpth_(false)
{
    const bool definft = SI().xyInFeet();
    setOkCancelText( uiStrings::sImport(), uiStrings::sCancel() );

    bparsfld_ = new uiSEGYVSPBasicPars( this );

    const bool zist = SI().zIsTime();
    uiGenInput* uigi;
    if ( zist )
    {
	istimefld_ = uigi = new uiGenInput( this, tr("Data is in "),
				     BoolInpSpec(true,uiStrings::sTime(),
                                                 uiStrings::sDepth()) );
	istimefld_->valuechanged.notify(
				mCB(this,uiWellImportSEGYVSP,isTimeChg) );
    }
    else
    {
	ObjectSet<const UnitOfMeasure> uns;
	UoMR().getRelevant( Mnemonic::Dist, uns );
	BufferStringSet unnms;
	for ( int idx=0; idx<uns.size(); idx++ )
	    unnms.add( uns[idx]->name() );
	unitfld_ = uigi = new uiGenInput( this, tr("Input sampling is in"),
				   StringListInpSpec(unnms) );
    }
    uigi->attach( alignedBelow, bparsfld_ );
    inpinftfld_ = new uiCheckBox( this, tr("Feet") );
    if ( definft ) inpinftfld_->setChecked( true );
    inpinftfld_->attach( rightOf, uigi );
    inpistvdfld_ = new uiCheckBox( this, tr("TVDSS") );
    inpistvdfld_->attach( rightOf, inpinftfld_ );

    const FloatInpIntervalSpec fspec( false );
    inpsampfld_ = new uiGenInput( this,
			tr("Overrule SEG-Y Sampling (start/step)"), fspec );
    inpsampfld_->attach( alignedBelow, uigi );
    inpsampfld_->setWithCheck( true );

    uiSeparator* sep = new uiSeparator( this, "hor sep", OD::Horizontal, false);
    sep->attach( stretchedBelow, inpsampfld_ );

    wellfld_ = new uiWellSel( this, true, tr("Add to Well"), false );
    wellfld_->attach( alignedBelow, inpsampfld_ );
    wellfld_->attach( ensureBelow, sep );
    wellfld_->selectionDone.notify( mCB(this,uiWellImportSEGYVSP,wllSel) );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
				       uiStrings::phrOutput( tr("log name") ));
    lcb->attach( alignedBelow, wellfld_ );
    lognmfld_ = lcb->box();
    lognmfld_->setReadOnly( false );
    lognmfld_->setText( "VSP" );

    const FloatInpIntervalSpec fispec( false );
    outzrgfld_ = new uiGenInput( this, tr("Limit output interval"), fispec );
    outzrgfld_->attach( alignedBelow, lcb );
    outzrgfld_->setWithCheck( true );
    outzrgfld_->checked.notify( mCB(this,uiWellImportSEGYVSP,outSampChk) );
    outinftfld_ = new uiCheckBox( this, tr("Feet") );
    if ( definft ) outinftfld_->setChecked( true );
    outinftfld_->attach( rightOf, outzrgfld_ );
    outistvdfld_ = new uiCheckBox( this, tr("TVDSS") );
    outistvdfld_->attach( rightOf, outinftfld_ );

    postFinalise().notify( mCB(this,uiWellImportSEGYVSP,wllSel) );
    postFinalise().notify( mCB(this,uiWellImportSEGYVSP,isTimeChg) );
}


uiWellImportSEGYVSP::~uiWellImportSEGYVSP()
{
}


void uiWellImportSEGYVSP::wllSel( CallBacker* )
{
    BufferStringSet nms; Well::MGR().getLogNamesByID( wellfld_->key(), nms );
    BufferString curlognm = lognmfld_->text();
    lognmfld_->setEmpty();
    lognmfld_->addItems( nms );
    if ( curlognm.isEmpty() )
	curlognm = "VSP";
    lognmfld_->setText( curlognm );
}


static void setInpSamp( uiGenInput* fld, SamplingData<float>& sd, float fac )
{
    sd.start *= fac; sd.step *= fac;
    fld->setValue( sd.start, 0 );
    fld->setValue( sd.step, 1 );
}


void uiWellImportSEGYVSP::use( const SeisTrc& trc )
{
    dispinpsamp_ = trc.info().sampling;
    setInpSamp( inpsampfld_, dispinpsamp_, mCast(float,isdpth_ ? 1 : 1000) );
    if ( isdpth_ )
    {
	outzrgfld_->setValue( dispinpsamp_.start, 0 );
	outzrgfld_->setValue( trc.endPos(), 1 );
    }
}


void uiWellImportSEGYVSP::isTimeChg( CallBacker* )
{
    const bool oldisdpth = isdpth_;
    isdpth_ = !istimefld_ || !istimefld_->getBoolValue();

    if ( oldisdpth != isdpth_ )
	setInpSamp( inpsampfld_, dispinpsamp_, isdpth_ ? 0.001f : 1000 );
    inpistvdfld_->display( isdpth_ );
    inpinftfld_->display( isdpth_ );
    outSampChk( 0 );
}


void uiWellImportSEGYVSP::outSampChk( CallBacker* )
{
    outinftfld_->display( outzrgfld_->isChecked() );
    outistvdfld_->display( outzrgfld_->isChecked() );
}


#define mErrRet(s) { uiMSG().error( s ); return false; }
#define mScaleVal(val,fac) if ( !mIsUdf(val) ) val *= fac

bool uiWellImportSEGYVSP::acceptOK( CallBacker* )
{
    if ( bparsfld_->fnm_.isEmpty() || sgypars_.isEmpty() )
	mErrRet(tr("Please define the input SEG-Y"))
    if ( !wellfld_->ioobj() )
	mErrRet(tr("Please select the output well"))
    const BufferString lognm( lognmfld_->text() );
    if ( lognm.isEmpty() )
	mErrRet(tr("Please enter a valid name for the new log"))

    SamplingData<float> inpsamp( mUdf(float), mUdf(float) );
    Interval<float> outzrg( mUdf(float), mUdf(float) );
    if ( inpsampfld_->isChecked() )
    {
	inpsamp.start = inpsampfld_->getFValue( 0 );
	inpsamp.step = inpsampfld_->getFValue( 1 );
	if ( !isdpth_ )
	    { mScaleVal(inpsamp.start,0.001); mScaleVal(inpsamp.step,0.001); }
	else if ( inpinftfld_->isChecked() )
	    { mScaleVal(inpsamp.start,mFromFeetFactorF);
		mScaleVal(inpsamp.step,mFromFeetFactorF); }
    }
    if ( outzrgfld_->isChecked() )
    {
	outzrg = outzrgfld_->getFInterval();
	if ( outinftfld_->isChecked() )
	    { mScaleVal(outzrg.start,mFromFeetFactorF);
		mScaleVal(outzrg.stop,mFromFeetFactorF); }
    }

    SeisTrc trc;
    if ( !fetchTrc(trc) )
	return false;

    if ( !mIsUdf(inpsamp.start) ) trc.info().sampling.start = inpsamp.start;
    if ( !mIsUdf(inpsamp.step) ) trc.info().sampling.step = inpsamp.step;

    if ( createLog(trc,outzrg,lognm) )
	uiMSG().message(tr("%1 created and saved").arg(lognm.buf()));
    return false;
}


bool uiWellImportSEGYVSP::fetchTrc( SeisTrc& trc )
{
    PtrMan<SEGYSeisTrcTranslator> trans = SEGYSeisTrcTranslator::getInstance();
    trans->usePar( sgypars_ );
    SEGY::FileSpec fs; fs.usePar( sgypars_ );
    PtrMan<IOObj> ioobj = fs.getIOObj( true );
    if ( !trans->initRead( ioobj->getConn(Conn::Read), Seis::Scan ) )
	mErrRet(trans->errMsg())

    if ( !trans->read(trc) )
	mErrRet(trans->errMsg())

    return true;
}


bool uiWellImportSEGYVSP::createLog( const SeisTrc& trc,
				     const Interval<float>& ozr,
				     const char* lognm )
{
    const MultiID key(	wellfld_->key() );
    const bool wasloaded = Well::MGR().isLoaded( key );

    RefMan<Well::Data> wd = Well::MGR().get( key );
    if ( !wd )
	mErrRet(tr("Cannot load the selected well\n%1")
			.arg(Well::MGR().errMsg()))
    if ( !isdpth_ && !wd->d2TModel() )
	mErrRet(tr("Selected well has no Depth vs Time model"))

    const Well::Track& track = wd->track();
    int wlidx = wd->logs().indexOf( lognm );
    if ( wlidx >= 0 )
	delete wd->logs().remove( wlidx );
    Well::Log* wl = new Well::Log( lognm );
    wl->pars().set( sKey::FileName(), sgypars_.find(sKey::FileName()) );

    Interval<float> outzrg( ozr ); outzrg.sort();
    const bool havestartout = !mIsUdf(outzrg.start);
    const bool havestopout = !mIsUdf(outzrg.stop);
    if ( outistvdfld_->isChecked() )
    {
	if ( havestartout )
	    outzrg.start = track.getDahForTVD( outzrg.start );
	if ( havestopout )
	    outzrg.start = track.getDahForTVD( outzrg.stop );
    }

    const bool inptvd = inpistvdfld_->isChecked();
    const float zeps = 0.00001;
    float prevdah = mUdf(float);
    for ( int isamp=0; isamp<trc.size(); isamp++ )
    {
	float z = trc.samplePos( isamp );
	if ( !isdpth_ )
	    z = wd->d2TModel()->getDah( z, track );
	else if ( inptvd )
	    prevdah = z = track.getDahForTVD( z, prevdah );

	if ( havestartout && z>outzrg.start-zeps )
	    continue;
	if ( havestopout && z>outzrg.stop+zeps )
	    break;

	wl->addValue( z, trc.get(isamp,0) );
    }

    wd->logs().add( wl );

    Well::Writer wtr( key, *wd );
    wtr.putLog( *wl );

    if ( wasloaded )
	Well::MGR().reload( key );

    return true;
}
