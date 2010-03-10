/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisbayesclass.cc,v 1.10 2010-03-10 16:19:04 cvsbert Exp $";

#include "uiseisbayesclass.h"
#include "seisbayesclass.h"
#include "uivarwizarddlg.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uigeninput.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uiobjdisposer.h"
#include "uitaskrunner.h"
#include "odusginfo.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"
#include "seistrctr.h"


#define mSetState(st) { state_ = st; nextAction(); return; }
static const int cMaxNrPDFs = 5;
static const char* sKeyBayesInv = "Bayesian Inversion";
#define mInpPDFs	10
#define mGetWghts	11
#define mInpSeis	12
#define mOutput		13


static ProbDenFunc* getPDF( const char* id, BufferString& emsg )
{
    if ( !id || !*id ) { emsg = "No ID"; return 0; }
    PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
    if ( !ioobj ) { emsg = "No IOObj"; return 0; }
    return ProbDenFuncTranslator::read(*ioobj,&emsg);
}


uiSeisBayesClass::uiSeisBayesClass( uiParent* p, bool is2d )
    : uiVarWizard(p)
    , Usage::Client(sKeyBayesInv)
    , is2d_(is2d)
    , inppdfdlg_(0)
    , inpseisdlg_(0)
    , outdlg_(0)
{
    prepUsgStart( "Definition" ); sendUsgInfo();
    pars_.set( sKey::Type, is2d_ ? "2D" : "3D" );

    state_ = mInpPDFs;
    nextAction();
}

// Destructor at end of file (deleting local classes)


void uiSeisBayesClass::closeDown()
{
    prepUsgEnd(); sendUsgInfo();
    uiVarWizard::closeDown();
}


void uiSeisBayesClass::doPart()
{
    switch ( state_ )
    {
    case mInpPDFs:
	getInpPDFs();
    break;
    case mGetWghts:
	getWeights();
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
{
public:

uiSeisBayesPDFInp( uiParent* p, IOPar& pars )
    : uiVarWizardDlg(p,uiDialog::Setup(sKeyBayesInv,"[1] Specify PDF input",
				 mTODOHelpID), pars,Start)
    , nrdisp_(1)
{
    rmbuts_.allowNull(); addbuts_.allowNull();
    IOObjContext ctxt( mIOObjContext(ProbDenFunc) );
    ctxt.forread = true;

    const CallBack pushcb = mCB(this,uiSeisBayesPDFInp,butPush);
    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	uiIOObjSel* fld = new uiIOObjSel( this, ctxt,
				BufferString("Input PDF ",idx+1) );
	if ( idx == 0 )
	    rmbuts_ += 0;
	else
	{
	    fld->attach( alignedBelow, flds_[idx-1] );
	    uiButton* rmbut = new uiPushButton( this, "<- Less", pushcb, true );
	    rmbut->attach( rightAlignedBelow, fld );
	    rmbuts_ += rmbut;
	}
	if ( idx == cMaxNrPDFs-1 )
	    addbuts_ += 0;
	else
	{
	    uiButton* addbut = new uiPushButton( this, "More ->", pushcb, true);
	    addbut->attach( leftAlignedBelow, fld );
	    addbuts_ += addbut;
	}

	const char* id = pars_.find( mGetSeisBayesPDFIDKey(idx) );
	const bool haveid = id && *id;
	if ( haveid || idx )
	{
	    fld->setInput( MultiID(id) );
	    if ( haveid ) nrdisp_ = idx+1;
	}

	flds_ += fld;
    }

    finaliseDone.notify( mCB(this,uiSeisBayesPDFInp,handleDisp) );
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
	flds_[idx]->display( idx < nrdisp_ );
	if ( addbuts_[idx] ) addbuts_[idx]->display( idx == nrdisp_-1 );
	if ( rmbuts_[idx] ) rmbuts_[idx]->display( idx == nrdisp_-1 );
    }
}

bool acceptOK( CallBacker* )
{
    PtrMan<ProbDenFunc> pdf0 = 0;

    for ( int idx=0; idx<flds_.size(); idx++ )
    {
	uiIOObjSel* fld = flds_[idx];
	if ( idx >= nrdisp_ )
	    pars_.removeWithKey( mGetSeisBayesPDFIDKey(idx) );
	else
	{
	    const IOObj* ioobj = fld->ioobj();
	    if ( !ioobj ) return false;
	    BufferString emsg;
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
		    uiMSG().error( BufferString( "'", ioobj->name(),
				"'\nis not compatible with the first" ) );
		    return false;
		}
	    }

	    pars_.set( mGetSeisBayesPDFIDKey(idx), fld->key() );
	}
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
    mSetState( mGetWghts );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


class uiSeisBayesWeights : public uiVarWizardDlg
{
public:

uiSeisBayesWeights( uiParent* p, IOPar& pars )
    : uiVarWizardDlg(p,uiDialog::Setup(sKeyBayesInv,"[2] Normalization/Scaling",
			 mTODOHelpID), pars,Middle)
{
    normpolfld_ = new uiGenInput( this, "Normalization mode",
		      StringListInpSpec(SeisBayesClass::NormPolNames()) );
    const char* res = pars_.find( SeisBayesClass::sKeyNormPol() );
    SeisBayesClass::NormPol pol = SeisBayesClass::PerBin;
    if ( res && *res ) pol = eEnum(SeisBayesClass::NormPol,res);
    normpolfld_->setValue( (int)pol );

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	const char* id = pars_.find( mGetSeisBayesPDFIDKey(idx) );
	if ( !id || !*id ) break;

	BufferString scltxt( "Pre-scale for '" );
	scltxt.add( IOM().nameOf(id) ).add( "'" );
	float scl = 1;
	res = pars_.find( mGetSeisBayesPreScaleKey(idx) );
	if ( res && *res ) scl = atof( res );
	uiGenInput* fld = new uiGenInput( this, scltxt, FloatInpSpec(scl) );
	fld->attach( alignedBelow, idx ? sclflds_[idx-1] : normpolfld_ );
	sclflds_ += fld;
    }
}

bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<sclflds_.size(); idx++ )
    {
	float scl = sclflds_[idx]->getfValue();
	if ( scl <= 0 )
	    mErrRet("Please enter only valid scales (> 0)")
	if ( mIsUdf(scl) ) scl = 1;
	pars_.set( mGetSeisBayesPreScaleKey(idx), scl );
    }

    const SeisBayesClass::NormPol pol
	= (SeisBayesClass::NormPol)normpolfld_->getIntValue();
    pars_.set( SeisBayesClass::sKeyNormPol(),
	    eString(SeisBayesClass::NormPol,pol) );
    return true;
}

    uiGenInput*		normpolfld_;
    ObjectSet<uiGenInput> sclflds_;

};


void uiSeisBayesClass::getWeights()
{
    wghtsdlg_ = new uiSeisBayesWeights( parent_, pars_ );
    mLaunchVWDialog(wghtsdlg_,uiSeisBayesClass,weightsGot);
}


void uiSeisBayesClass::weightsGot( CallBacker* )
{
    mHandleVWCancel(wghtsdlg_,mInpPDFs)
    mSetState( mInpSeis );
}


class uiSeisBayesSeisInp : public uiVarWizardDlg
{
public:

uiSeisBayesSeisInp( uiParent* p, IOPar& pars, bool is2d )
    : uiVarWizardDlg(p,uiDialog::Setup(sKeyBayesInv,"[3] Specify Seismic input",
			 mTODOHelpID), pars,Middle)
    , lsfld_(0)
    , is2d_(is2d)
{
    BufferString emsg;
    PtrMan<ProbDenFunc> pdf = getPDF( pars_.find( mGetSeisBayesPDFIDKey(0) ),
	    				emsg );
    if ( !pdf ) { new uiLabel(this,emsg); return; }

    const int nrvars = pdf->nrDims();
    const Seis::GeomType gt = is2d ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup su( gt );
    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    uiSeisSel::fillContext( gt, true, ctxt );

    if ( is2d_ )
    {
	su.selattr_ = false;
	lsfld_ = new uiSeisSel( this, ctxt, su );
    }
    else
    {
	for ( int idx=0; idx<nrvars; idx++ )
	{
	    su.seltxt_ = "Input for '";
	    su.seltxt_ += pdf->dimName(idx);
	    su.seltxt_ += "'";
	    uiSeisSel* fld = new uiSeisSel( this, ctxt, su );
	    const char* id = pars_.find( mGetSeisBayesSeisInpIDKey(idx) );
	    fld->setInput( MultiID(id) );
	    if ( idx )
		fld->attach( alignedBelow, flds3d_[idx-1] );
	    flds3d_ += fld;
	}
    }
}

bool acceptOK( CallBacker* )
{
    if ( is2d_ ) { uiMSG().error( "2D not implemented" ); return false; }

    for ( int idx=0; idx<flds3d_.size(); idx++ )
    {
	const IOObj* ioobj = flds3d_[idx]->ioobj();
	if ( !ioobj ) return false;
	pars_.set( mGetSeisBayesSeisInpIDKey(idx), ioobj->key() );
    }

    return true;
}

    const bool			is2d_;

    uiSeisSel*			lsfld_;
    ObjectSet<uiSeisSel>	flds3d_;

};


void uiSeisBayesClass::getInpSeis()
{
    inpseisdlg_ = new uiSeisBayesSeisInp( parent_, pars_, is2d_ );
    mLaunchVWDialog(inpseisdlg_,uiSeisBayesClass,inpSeisGot);
}


void uiSeisBayesClass::inpSeisGot( CallBacker* )
{
    mHandleVWCancel(inpseisdlg_,mGetWghts)
    mSetState( mOutput );
}


class uiSeisBayesOut : public uiVarWizardDlg
{
public:

uiSeisBayesOut( uiParent* p, IOPar& pars, bool is2d )
    : uiVarWizardDlg(p,uiDialog::Setup(sKeyBayesInv,
			"[4] Select and specify output",mTODOHelpID), pars,End)
    , is2d_(is2d)
{
    if ( is2d_ ) { new uiLabel( this, "2D not implemented" ); return; }

    BufferString emsg;
    PtrMan<ProbDenFunc> pdf = getPDF( pars_.find( mGetSeisBayesPDFIDKey(0) ),
	    				emsg );
    if ( !pdf ) { new uiLabel(this,emsg); return; }

    for ( int idx=0; idx<cMaxNrPDFs; idx++ )
    {
	const char* id = pars_.find( mGetSeisBayesPDFIDKey(idx) );
	if ( !id || !*id ) break;
	addOut( IOM().nameOf(id), true );
    }
    addOut( "Classification", false );
    addOut( "Confidence", false );

    Seis::SelSetup sss( is2d_, false ); sss.fornewentry(true).onlyrange(false);
    subselfld_ = uiSeisSubSel::get( this, sss );
    subselfld_->attach( alignedBelow, flds3d_[ flds3d_.size()-1 ] );
    const char* id = pars_.find( mGetSeisBayesSeisInpIDKey(0) );
    if ( id && *id )
	subselfld_->setInput( MultiID(id) );
    subselfld_->usePar( pars_ );
}

void addOut( const char* nm, bool ispdf )
{
    const Seis::GeomType gt = is2d_ ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup su( gt ); su.optional(true);
    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    uiSeisSel::fillContext( gt, false, ctxt );

    if ( !ispdf )
	su.seltxt_ = nm;
    else
	{ su.seltxt_ = "P: '"; su.seltxt_ += nm; su.seltxt_ += "'"; }

    const int curidx = flds3d_.size();
    uiSeisSel* fld = new uiSeisSel( this, ctxt, su );
    const char* id = pars_.find( mGetSeisBayesSeisOutIDKey(curidx) );
    fld->setInput( MultiID(id) );
    if ( fld->ctxtIOObj(true).ioobj )
	fld->setChecked( true );
    if ( curidx > 0 )
	fld->attach( alignedBelow, flds3d_[curidx-1] );
    flds3d_ += fld;
}

bool acceptOK( CallBacker* )
{
    if ( is2d_ ) return false;

    int nrout = 0;
    for ( int idx=0; idx<flds3d_.size(); idx++ )
    {
	uiSeisSel* sel = flds3d_[idx];
	const bool isneeded = sel->isChecked();
	const IOObj* ioobj = isneeded ? sel->ioobj() : 0;
	if ( isneeded && !ioobj )
	    mErrRet("Please specify all selected outputs")
	if ( !ioobj ) continue;

	pars_.set( mGetSeisBayesSeisOutIDKey(idx), ioobj->key() );
	nrout++;
    }

    subselfld_->fillPar( pars_ );

    if ( nrout < 1 )
	mErrRet("Please specify at least one output")
    return true;
}

    const bool			is2d_;

    ObjectSet<uiSeisSel>	flds3d_;
    uiSeisSubSel*		subselfld_;


};


void uiSeisBayesClass::doOutput()
{
    outdlg_ = new uiSeisBayesOut( parent_, pars_, is2d_ );
    mLaunchVWDialog(outdlg_,uiSeisBayesClass,outputDone);
}


void uiSeisBayesClass::outputDone( CallBacker* )
{
    mHandleVWCancel(outdlg_,mInpSeis)

    SeisBayesClass exec( pars_ );
    uiTaskRunner tr( outdlg_ );
    const bool isok = tr.execute( exec );

    mSetState( isok ? cFinished() : mOutput );
}


uiSeisBayesClass::~uiSeisBayesClass()
{
    delete inppdfdlg_;
    delete inpseisdlg_;
    delete outdlg_;
}
