/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisbayesclass.h"
#include "seisbayesclass.h"
#include "uivarwizarddlg.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "od_helpids.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"

#define mInpPDFsHelpID	mODHelpKey(mSeisBayesPDFInp)
#define mGetNormHelpID	mODHelpKey(mSeisBayesNorm)
#define mInpSeisHelpID	mODHelpKey(mSeisBayesSeisInp)
#define mOutputHelpID	mODHelpKey(mSeisBayesOut)

#define mSetState(st) { state_ = st; nextAction(); return; }
static const int cMaxNrPDFs = 5;
static const uiString sKeyBayesClss()
{ return  od_static_tr("sKeyBayesClss","Bayesian classification"); }
#define mInpPDFs	10
#define mGetNorm	11
#define mInpSeis	12
#define mOutput		13


static ProbDenFunc* getPDF( const char* id, uiString& emsg )
{
    if ( !id || !*id ) { emsg = od_static_tr("getPDF","No ID"); return 0; }
    PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
    if ( !ioobj ) { emsg = od_static_tr("getPDF","No IOObj"); return 0; }
    return ProbDenFuncTranslator::read(*ioobj,&emsg);
}


uiSeisBayesClass::uiSeisBayesClass( uiParent* p, bool is2d )
    : uiVarWizard(p)
    , is2d_(is2d)
    , inppdfdlg_(0)
    , normdlg_(0)
    , inpseisdlg_(0)
    , outdlg_(0)
{
    pars_.set( sKey::Type(), is2d_ ? "2D" : "3D" );

    state_ = mInpPDFs;
    nextAction();
}

// Destructor at end of file (deleting local classes)


void uiSeisBayesClass::closeDown()
{
    uiVarWizard::closeDown();
}


void uiSeisBayesClass::doPart()
{
    switch ( state_ )
    {
    case mInpPDFs:
	getInpPDFs();
    break;
    case mGetNorm:
	getNorm();
    break;
    case mInpSeis:
	getInpSeis();
    break;
    case mOutput:
	doOutput();
    break;
    }
}


class uiSeisBayesPDFInp : public uiVarWizardDlg
{ mODTextTranslationClass(uiSeisBayesPDFInp);
public:

uiSeisBayesPDFInp( uiParent* p, IOPar& pars )
    : uiVarWizardDlg(p,uiDialog::Setup(uiStrings::phrJoinStrings(sKeyBayesClss()
				,tr("- PDFs")), tr("[1] Specify PDF input"),
				 mODHelpKey(mSeisBayesPDFInpHelpID) ),
                                 pars,Start)
    , nrdisp_(1)
{
    rmbuts_.allowNull(); addbuts_.allowNull();
    IOObjContext ctxt( mIOObjContext(ProbDenFunc) );
    ctxt.forread_ = true;

    const CallBack pushcb = mCB(this,uiSeisBayesPDFInp,butPush);
    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	uiIOObjSel* fld = new uiIOObjSel(this, ctxt,
				 uiStrings::phrInput(tr("PDF %1").arg(idx+1)));
	if ( idx == 0 )
	    rmbuts_ += 0;
	else
	{
	    fld->attach( alignedBelow, flds_[idx-1] );
	    uiButton* rmbut = new uiPushButton( this, tr("<- Less"),
						pushcb, true);
	    rmbut->attach( rightAlignedBelow, fld );
	    rmbuts_ += rmbut;
	}
	if ( idx == cMaxNrPDFs-1 )
	    addbuts_ += 0;
	else
	{
	    uiButton* addbut = new uiPushButton( this, tr("More ->"),
						 pushcb,true);
	    addbut->attach( leftAlignedBelow, fld );
	    addbuts_ += addbut;
	}

	MultiID mid;
	pars_.get( mGetSeisBayesPDFIDKey(idx), mid );
	if ( !mid.isUdf() || idx )
	{
	    fld->setInput( mid );
	    if ( !mid.isUdf() )
		nrdisp_ = idx+1;
	}

	flds_ += fld;
    }

    mAttachCB(postFinalize(), uiSeisBayesPDFInp::handleDisp );
}

~uiSeisBayesPDFInp()
{
    detachAllNotifiers();
}

void butPush( CallBacker* cb )
{
    mDynamicCastGet(uiButton*,but,cb)
    if ( !but ) return;

    bool isadd = false;
    int idx = addbuts_.indexOf( but );
    if ( idx < 0 )	idx = rmbuts_.indexOf( but );
    else		isadd = true;
    if ( idx < 0 ) return;

    nrdisp_ += isadd ? 1 : -1;
    handleDisp( 0 );
}

void handleDisp( CallBacker* )
{
    for ( int idx=0; idx<flds_.size(); idx++ )
    {
	const bool dodisp = idx < nrdisp_;
	flds_[idx]->display( dodisp );
	if ( addbuts_[idx] ) addbuts_[idx]->display( idx == nrdisp_-1 );
	if ( rmbuts_[idx] ) rmbuts_[idx]->display( idx == nrdisp_-1 );
    }
}

bool acceptOK( CallBacker* ) override
{
    PtrMan<ProbDenFunc> pdf0 = 0;
    pars_.removeSubSelection( SeisBayesClass::sKeyPDFID() );

    for ( int idx=0; idx<nrdisp_; idx++ )
    {
	uiIOObjSel* fld = flds_[idx];

	const IOObj* ioobj = fld->ioobj();
	if ( !ioobj ) return false;
	uiString emsg;
	ProbDenFunc* pdf = ProbDenFuncTranslator::read( *ioobj, &emsg );
	if ( !pdf )
	    { uiMSG().error(emsg); delete pdf; return false; }
	else if ( !idx )
	    pdf0 = pdf;
	else
	{
	    const bool iscompat = pdf->isCompatibleWith( *pdf0 );
	    delete pdf;
	    if ( !iscompat )
	    {
		uiMSG().error(tr("'%1'\nis not compatible with the first")
			    .arg(ioobj->name()));
		return false;
	    }
	}

	pars_.set( mGetSeisBayesPDFIDKey(idx), fld->key() );
    }

    return true;
}

    ObjectSet<uiIOObjSel>	flds_;
    ObjectSet<uiButton>		addbuts_;
    ObjectSet<uiButton>		rmbuts_;
    int				nrdisp_;

};



void uiSeisBayesClass::getInpPDFs()
{
    inppdfdlg_ = new uiSeisBayesPDFInp( parent_, pars_ );
    mLaunchVWDialog(inppdfdlg_,uiSeisBayesClass,inpPDFsGot);
}


void uiSeisBayesClass::inpPDFsGot( CallBacker* )
{
    mHandleVWCancel(inppdfdlg_,cCancelled())
    inppdfdlg_ = 0;
    mSetState( mGetNorm );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


class uiSeisBayesNorm : public uiVarWizardDlg
{ mODTextTranslationClass(uiSeisBayesNorm);
public:

uiSeisBayesNorm( uiParent* p, IOPar& pars )
    : uiVarWizardDlg(p,uiDialog::Setup(uiStrings::phrJoinStrings(sKeyBayesClss()
			 ,tr("- Scaling")), tr("[2] Normalization/Scaling"),
			 mODHelpKey(mSeisBayesNormHelpID) ), pars, Middle )
    , is2d_(pars[sKey::Type()].firstChar() == '2')
    , prenormfld_(nullptr)
    , nrpdfs_(0)
{
    const CallBack dispcb( mCB(this,uiSeisBayesNorm,updDisp) );

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	const BufferString id = pars_.find( mGetSeisBayesPDFIDKey(idx) );
	if ( id.isEmpty() )
	    break;

	nrpdfs_++;
    }

    if ( nrpdfs_ > 1 )
    {
	const bool dopre = !pars_.isFalse( SeisBayesClass::sKeyPreNorm() );
	prenormfld_ = new uiGenInput( this, tr("Normalize Input PDFs"),
					    BoolInpSpec(dopre) );
    }

    useglobfld_ = new uiGenInput( this, tr("A priori weights"),
				  BoolInpSpec(false,tr("Constant"),
				  tr("Variable")) );
    if ( prenormfld_ )
	useglobfld_->attach( alignedBelow, prenormfld_ );

    const Seis::GeomType gt = is2d_ ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup su( gt ); su.optional(true);
    const IOObjContext ctxt( uiSeisSel::ioContext(gt,true) );

    bool havevariable = false;
    uiGenInput* alobj = useglobfld_;
    for ( int idx=0; idx<nrpdfs_; idx++ )
    {
	const BufferString id = pars_.find( mGetSeisBayesPDFIDKey(idx) );
	uiString fldtxt = tr("For '%1'").
				    arg(toUiString(IOM().nameOf(id.buf())));

	float scl = 1;
	BufferString res = pars_.find( mGetSeisBayesPreScaleKey(idx) );
	if ( !res.isEmpty() )
	    scl = res.toFloat();

	uiGenInput* fld = new uiGenInput( this, fldtxt, FloatInpSpec(scl) );
	fld->attach( alignedBelow, alobj );
	sclflds_ += fld;

	uiIOObjSel* os = new uiIOObjSel( this, ctxt, fldtxt );
	MultiID mid;
	pars_.get( mGetSeisBayesAPProbIDKey(idx), mid );
	os->setInput( mid );
	os->attach( alignedBelow, alobj );
	apflds_ += os;
	if ( !res.isEmpty() )
	    havevariable = true;

	alobj = fld;
    }
    useglobfld_->setValue( !havevariable );

    if ( prenormfld_ )
    {
	const bool dopost = !pars_.isFalse( SeisBayesClass::sKeyPostNorm() );
	postnormfld_ = new uiGenInput( this, tr("Normalize output"),
					    BoolInpSpec(dopost) );
	postnormfld_->attach( alignedBelow, alobj );
    }


    useglobfld_->valuechanged.notify( dispcb );
    postFinalize().notify( dispcb );
}


void updDisp( CallBacker* )
{
    const bool isglob = useglobfld_->getBoolValue();
    for ( int idx=0; idx<apflds_.size(); idx++ )
    {
	sclflds_[idx]->display( isglob );
	apflds_[idx]->display( !isglob );
    }
}

bool rejectOK( CallBacker* cb ) override
{
    if ( nrpdfs_ < 1 ) return true;
    bool rv = uiVarWizardDlg::rejectOK( cb );
    getFromScreen( true );
    return rv;
}

bool acceptOK( CallBacker* ) override
{
    if ( nrpdfs_ < 1 ) return false;
    pars_.removeSubSelection( SeisBayesClass::sKeyAPProbID() );
    pars_.removeSubSelection( SeisBayesClass::sKeyPreScale() );
    return getFromScreen( false );
}

bool getFromScreen( bool permissive )
{
    const bool isglob = useglobfld_->getBoolValue();
    for ( int idx=0; idx<nrpdfs_; idx++ )
    {
	if ( isglob )
	{
	    float scl = sclflds_[idx]->getFValue();
	    if ( scl <= 0 )
	    {
		if ( permissive ) continue;
		mErrRet(tr("Please enter only valid scales (> 0)"))
	    }
	    if ( mIsUdf(scl) ) scl = 1;
	    pars_.set( mGetSeisBayesPreScaleKey(idx), scl );
	}
	else
	{
	    uiIOObjSel* fld = apflds_[idx];
	    const IOObj* ioobj = fld->ioobj( permissive );
	    if ( ioobj )
		pars_.set( mGetSeisBayesAPProbIDKey(idx), ioobj->key() );
	    else if ( !permissive )
		return false;
	}
    }

    if ( prenormfld_ )
    {
	pars_.setYN( SeisBayesClass::sKeyPreNorm(),
		     prenormfld_->getBoolValue() );
	pars_.setYN( SeisBayesClass::sKeyPostNorm(),
		     postnormfld_->getBoolValue() );
    }

    return true;
}

    bool		is2d_;
    int			nrpdfs_;
    uiGenInput*		useglobfld_;
    uiGenInput*		prenormfld_;
    uiGenInput*		postnormfld_;
    ObjectSet<uiGenInput> sclflds_;
    ObjectSet<uiIOObjSel> apflds_;

};


void uiSeisBayesClass::getNorm()
{
    normdlg_ = new uiSeisBayesNorm( parent_, pars_ );
    mLaunchVWDialog(normdlg_,uiSeisBayesClass,normGot);
}


void uiSeisBayesClass::normGot( CallBacker* )
{
    mHandleVWCancel(normdlg_,mInpPDFs)
    normdlg_ = 0;
    mSetState( mInpSeis );
}


class uiSeisBayesSeisInp : public uiVarWizardDlg
{ mODTextTranslationClass(uiSeisBayesSeisInp);
public:

uiSeisBayesSeisInp( uiParent* p, IOPar& pars )
    : uiVarWizardDlg(p, uiDialog::Setup(tr("%1- Seismics").arg(sKeyBayesClss()),
					tr("[3] Specify Seismic input"),
					mODHelpKey(mSeisBayesSeisInpHelpID) ),
					pars,Middle)
    , lsfld_(nullptr)
    , is2d_(pars[sKey::Type()].firstChar() == '2')
{
    uiString emsg;
    PtrMan<ProbDenFunc> pdf = getPDF( pars_.find( mGetSeisBayesPDFIDKey(0) ),
					emsg );
    if ( !pdf ) { new uiLabel(this,emsg); return; }

    const int nrvars = pdf->nrDims();
    const Seis::GeomType gt = is2d_ ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup su( gt );
    const IOObjContext ctxt( uiSeisSel::ioContext(gt,true) );

    if ( is2d_ )
	lsfld_ = new uiSeisSel( this, ctxt, su );
    else
    {
	for ( int idx=0; idx<nrvars; idx++ )
	{
	    su.seltxt_ = tr("Input for '%1'").arg( pdf->dimName(idx) );
	    uiSeisSel* fld = new uiSeisSel( this, ctxt, su );
	    MultiID mid;
	    pars_.get( mGetSeisBayesSeisInpIDKey(idx), mid );
	    fld->setInput( mid );
	    if ( idx )
		fld->attach( alignedBelow, flds3d_[idx-1] );

	    flds3d_ += fld;
	}
    }
}

bool rejectOK( CallBacker* cb ) override
{
    bool rv = uiVarWizardDlg::rejectOK( cb );
    getFromScreen( true );
    return rv;
}

bool acceptOK( CallBacker* ) override
{
    pars_.removeSubSelection( SeisBayesClass::sKeySeisInpID() );
    return getFromScreen( false );
}


bool getFromScreen( bool permissive )
{
    if ( is2d_ ) { uiMSG().error( tr("2D not implemented") ); return false; }

    for ( int idx=0; idx<flds3d_.size(); idx++ )
    {
	const IOObj* ioobj = flds3d_[idx]->ioobj();
	if ( ioobj )
	    pars_.set( mGetSeisBayesSeisInpIDKey(idx), ioobj->key() );
	else if ( !permissive )
	    return false;
    }

    return true;
}

    const bool			is2d_;

    uiSeisSel*			lsfld_;
    ObjectSet<uiSeisSel>	flds3d_;

};


void uiSeisBayesClass::getInpSeis()
{
    inpseisdlg_ = new uiSeisBayesSeisInp( parent_, pars_ );
    mLaunchVWDialog(inpseisdlg_,uiSeisBayesClass,inpSeisGot);
}


void uiSeisBayesClass::inpSeisGot( CallBacker* )
{
    mHandleVWCancel(inpseisdlg_,mGetNorm)
    inpseisdlg_ = 0;
    mSetState( mOutput );
}


class uiSeisBayesOut : public uiVarWizardDlg
{ mODTextTranslationClass(uiSeisBayesOut);
public:

uiSeisBayesOut( uiParent* p, IOPar& pars )
    : uiVarWizardDlg(p, uiDialog::Setup(tr("%1- Output").arg(sKeyBayesClss()),
					tr("[4] Select and specify output"),
					mODHelpKey(mSeisBayesOutHelpID) ),
					pars,DoWork)
    , is2d_(pars[sKey::Type()].firstChar() == '2')
    , haveclass_(true)
{
    if ( is2d_ ) { new uiLabel( this, tr("2D not implemented") ); return; }

    uiString emsg;
    PtrMan<ProbDenFunc> pdf = getPDF( pars_.find( mGetSeisBayesPDFIDKey(0) ),
					emsg );
    if ( !pdf ) { new uiLabel(this,emsg); return; }

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	const BufferString id = pars_.find( mGetSeisBayesPDFIDKey(idx) );
	if ( id.isEmpty() )
	    break;

	addOut( IOM().nameOf(id.buf()), true );
    }
    if ( flds3d_.size() < 2 )
	haveclass_ = false;
    else
    {
	addOut( "Classification: Class", false );
	addOut( "Classification: Confidence", false );
    }
    addOut( "Determination strength", false );

    Seis::SelSetup sss( is2d_, false ); sss.fornewentry(true).onlyrange(false);
    subselfld_ = uiSeisSubSel::get( this, sss );
    subselfld_->attach( alignedBelow, flds3d_[ flds3d_.size()-1 ] );
    MultiID mid;
    pars_.get( mGetSeisBayesSeisInpIDKey(0), mid );
    if ( !mid.isUdf() )
	subselfld_->setInput( mid);

    subselfld_->usePar( pars_ );
}

void addOut( const char* nm, bool ispdf )
{
    const Seis::GeomType gt = is2d_ ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup su( gt ); su.optional(true);
    const IOObjContext ctxt( uiSeisSel::ioContext(gt,false) );

    if ( !ispdf )
	su.seltxt_ = toUiString(nm);
    else
	su.seltxt_ = uiString(tr("Probability: '%1'")).arg( nm );

    const int nrflds = flds3d_.size();
    int curidx = nrflds;
    if ( !ispdf && !haveclass_ )
	curidx += 2;

    uiSeisSel* fld = new uiSeisSel( this, ctxt, su );
    MultiID mid;
    pars_.get( mGetSeisBayesSeisOutIDKey(curidx), mid );
    fld->setInput( mid );
    if ( fld->ctxtIOObj(true).ioobj_ )
	fld->setChecked( true );

    if ( nrflds > 0 )
	fld->attach( alignedBelow, flds3d_[nrflds-1] );

    flds3d_ += fld;
}


bool rejectOK( CallBacker* cb ) override
{
    bool rv = uiVarWizardDlg::rejectOK( cb );
    getFromScreen( true );
    return rv;
}

bool acceptOK( CallBacker* ) override
{
    pars_.removeSubSelection( SeisBayesClass::sKeySeisOutID() );
    return getFromScreen( false );
}

bool getFromScreen( bool permissive )
{
    if ( is2d_ ) return false;

    int nrout = 0;
    for ( int idx=0; idx<flds3d_.size(); idx++ )
    {
	uiSeisSel* sel = flds3d_[idx];
	const bool isneeded = sel->isChecked();
	const IOObj* ioobj = isneeded ? sel->ioobj(permissive) : 0;
	if ( isneeded && !ioobj )
	{
	    if ( permissive )
		continue;
	    else
		mErrRet(tr("Please specify all selected outputs"))
	}

	if ( ioobj )
	{
	    const int iopidx = idx == flds3d_.size()-1 && !haveclass_
			     ? idx + 2 : idx;
	    pars_.set( mGetSeisBayesSeisOutIDKey(iopidx), ioobj->key() );
	    nrout++;
	}
    }

    subselfld_->fillPar( pars_ );

    if ( !permissive && nrout < 1 )
	mErrRet(tr("Please specify at least one output"))

    return true;
}

    const bool			is2d_;
    bool			haveclass_;

    ObjectSet<uiSeisSel>	flds3d_;
    uiSeisSubSel*		subselfld_;


};


void uiSeisBayesClass::doOutput()
{
    outdlg_ = new uiSeisBayesOut( parent_, pars_ );
    mLaunchVWDialog(outdlg_,uiSeisBayesClass,outputDone);
}


void uiSeisBayesClass::outputDone( CallBacker* )
{
    mHandleVWCancel(outdlg_,mInpSeis)

    SeisBayesClass exec( pars_ );
    uiTaskRunner taskrunner( outdlg_ );
    const bool isok = TaskRunner::execute( &taskrunner, exec );

    outdlg_ = 0;
    mSetState( isok ? cFinished() : mOutput );
}


#define mRaise(dlg) if ( dlg ) dlg->raise()

void uiSeisBayesClass::raiseCurrent()
{
	 mRaise( inppdfdlg_ );
    else mRaise( normdlg_ );
    else mRaise( inpseisdlg_ );
    else mRaise( outdlg_ );
}


uiSeisBayesClass::~uiSeisBayesClass()
{
    delete inppdfdlg_;
    delete normdlg_;
    delete inpseisdlg_;
    delete outdlg_;
}
