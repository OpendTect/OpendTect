/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Jan 2009
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpsegyvsp.cc,v 1.2 2009-01-12 12:42:45 cvsbert Exp $";

#include "uiwellimpsegyvsp.h"

#include "uicompoundparsel.h"
#include "uisegydefdlg.h"
#include "uisegyexamine.h"
#include "uisegyread.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uimsg.h"

#include "seistrc.h"
#include "seisread.h"
#include "segytr.h"
#include "welltransl.h"
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
    bparsfld_ = new uiSEGYVSPBasicPars( this );
    const FloatInpSpec fspec;
    inpsampfld_ = new uiGenInput( this, "Sampling in SEG-Y file (start/step)",
	    			  fspec, fspec );
    inpsampfld_->attach( alignedBelow, bparsfld_ );
    inpsampfld_->setReadOnly( true );

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
    istvdfld_ = new uiGenInput( this, "", BoolInpSpec(false,"TVDSS","MD") );
    istvdfld_->attach( rightOf, uigi );

    uiSeparator* sep = new uiSeparator( this, "hor sep", true, false );
    sep->attach( stretchedBelow, uigi );

    outsampfld_ = new uiGenInput( this, "Output sampling in MD (start/step)",
	    			  fspec, fspec );
    outsampfld_->attach( alignedBelow, uigi );
    outsampfld_->attach( ensureBelow, sep );

    lognmfld_ = new uiGenInput( this, "Output log name" );
    lognmfld_->attach( alignedBelow, outsampfld_ );

    wellfld_ = new uiIOObjSel( this, ctio_, "Add to Well" );
    wellfld_->attach( alignedBelow, lognmfld_ );

    finaliseDone.notify( mCB(this,uiWellImportSEGYVSP,initWin) );
}


uiWellImportSEGYVSP::~uiWellImportSEGYVSP()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiWellImportSEGYVSP::inpIsTime() const
{
    return istimefld_ && istimefld_->getBoolValue();
}


void uiWellImportSEGYVSP::initWin( CallBacker* )
{
}

#define mScaleSD(sd,fac) { sd.start *= fac; sd.step *= fac; }
#define mSetSDFldVals(fld,sd) \
{ \
    fld->setValue( sd.start, 0 ); \
    fld->setValue( sd.step, 1 ); \
}


void uiWellImportSEGYVSP::use( const SeisTrc& trc )
{
    const bool ist = inpIsTime();
    dispinpsamp_ = trc.info().sampling.start;
    if ( ist )
	mScaleSD(dispinpsamp_,1000)
    mSetSDFldVals( inpsampfld_, dispinpsamp_ );
    if ( !ist )
	mSetSDFldVals( outsampfld_, dispinpsamp_ );
}


void uiWellImportSEGYVSP::isTimeChg( CallBacker* )
{
    istvdfld_->display( !inpIsTime() );
}


#define mErrRet(s) { uiMSG().error( s ); return false; }

bool uiWellImportSEGYVSP::acceptOK( CallBacker* )
{
    const bool ist = inpIsTime();
    SamplingData<float> sd( outsampfld_->getfValue(0),
	    		    outsampfld_->getfValue(1) );
    if ( mIsUdf(sd.start) || mIsUdf(sd.step) )
	mErrRet("Please specify the sampling of the output VSP trace log" )
    if ( ist )
	mScaleSD( sd, 0.001 )

    
    return false;
}
