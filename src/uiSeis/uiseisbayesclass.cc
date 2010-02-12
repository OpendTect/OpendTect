/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisbayesclass.cc,v 1.5 2010-02-12 14:50:03 cvsbert Exp $";

#include "uiseisbayesclass.h"
#include "seisbayesclass.h"
#include "uibutton.h"
#include "uimsg.h"
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

// for test
#	include "uilabel.h"


#define mSetState(st) { state_ = st; nextAction(); return; }
static const int cMaxPDFs = 5;


uiSeisBayesClass::uiSeisBayesClass( uiParent* p, bool is2d, const IOPar* iop )
    : Usage::Client("Bayesian Classification")
    , is2d_(is2d)
    , state_(InpPDFS)
    , parent_(p)
    , inppdfdlg_(0)
    , inpseisdlg_(0)
    , outdlg_(0)
    , processEnded(this)
{
    prepUsgStart( "Definition" ); sendUsgInfo();
    pars_.set( sKey::Type, is2d_ ? "2D" : "3D" );

    nextAction();
}

// Destructor at end of file (deleting local classes)


void uiSeisBayesClass::closeDown()
{
    prepUsgEnd(); sendUsgInfo();
    uiOBJDISP()->go( this );
    processEnded.trigger();
}


void uiSeisBayesClass::nextAction()
{
    if ( state_ <= Finished )
	{ closeDown(); return; }

    switch ( state_ )
    {
    case Wait4Dialog:
	return;
    break;
    case InpPDFS:
	getInpPDFs();
    break;
    case InpSeis:
	getInpSeis();
    break;
    case Output:
	doOutput();
    break;
    }
}


class uiSeisBayesPDFInp : public uiDialog
{
public:

uiSeisBayesPDFInp( uiParent* p, IOPar& pars )
    : uiDialog(p,uiDialog::Setup("Bayesian Classification",
				 "Specify PDF input",mTODOHelpID).modal(false))
    , pars_(pars)
    , nrdisp_(1)
{
    setOkText( "Next &>>" );
    rmbuts_.allowNull(); addbuts_.allowNull();
    IOObjContext ctxt( mIOObjContext(ProbDenFunc) );
    ctxt.forread = true;

    const CallBack pushcb = mCB(this,uiSeisBayesPDFInp,butPush);
    for ( int idx=0; idx<cMaxPDFs; idx++ )
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
	if ( idx == cMaxPDFs-1 )
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

~uiSeisBayesPDFInp()
{
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
    for ( int idx=0; idx<flds_.size(); idx++ )
    {
	uiIOObjSel* fld = flds_[idx];
	if ( idx >= nrdisp_ )
	    pars_.removeWithKey( mGetSeisBayesPDFIDKey(idx) );
	else
	{
	    if ( !fld->ioobj() )
		return false;
	    pars_.set( mGetSeisBayesPDFIDKey(idx), fld->key() );
	}
    }
    return true;
}

    IOPar&			pars_;

    ObjectSet<uiIOObjSel>	flds_;
    ObjectSet<uiButton>		addbuts_;
    ObjectSet<uiButton>		rmbuts_;
    int				nrdisp_;

};


#define mLaunchDlg(dlg,fn) \
	dlg->windowClosed.notify( mCB(this,uiSeisBayesClass,fn) ); \
	dlg->setDeleteOnClose( true ); dlg->go()

void uiSeisBayesClass::getInpPDFs()
{
    inppdfdlg_ = new uiSeisBayesPDFInp( parent_, pars_ );
    mLaunchDlg(inppdfdlg_,inpPDFsGot);
    mSetState(Wait4Dialog);
}


void uiSeisBayesClass::inpPDFsGot( CallBacker* )
{
    if ( !inppdfdlg_ ) return;
    if ( !inppdfdlg_->uiResult() )
	mSetState(Cancelled);

    mSetState( InpSeis );
}


static ProbDenFunc* getPDF( const char* id, BufferString& emsg )
{
    if ( !id || !*id ) { emsg = "No ID"; return 0; }
    PtrMan<IOObj> ioobj = IOM().get( MultiID(id) );
    if ( !ioobj ) { emsg = "No IOObj"; return 0; }
    return ProbDenFuncTranslator::read(*ioobj,&emsg);
}


class uiSeisBayesSeisInp : public uiDialog
{
public:

uiSeisBayesSeisInp( uiParent* p, IOPar& pars, bool is2d )
    : uiDialog(p,uiDialog::Setup("Bayesian Classification",
				 "Specify Seismic input",mTODOHelpID)
	    			 .modal(false))
    , pars_(pars)
    , lsfld_(0)
    , is2d_(is2d)
{
    setOkText( "Next &>>" ); setCancelText( "&<< Back" );
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

~uiSeisBayesSeisInp()
{
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

    IOPar&			pars_;
    bool			is2d_;

    uiSeisSel*			lsfld_;
    ObjectSet<uiSeisSel>	flds3d_;


};


void uiSeisBayesClass::getInpSeis()
{
    inpseisdlg_ = new uiSeisBayesSeisInp( parent_, pars_, is2d_ );
    mLaunchDlg(inpseisdlg_,inpSeisGot);
    mSetState( Wait4Dialog );
}


void uiSeisBayesClass::inpSeisGot( CallBacker* )
{
    if ( !inpseisdlg_->uiResult() )
	{ inpseisdlg_ = 0; mSetState( InpPDFS ); }

    mSetState( Output );
}


class uiSeisBayesOut : public uiDialog
{
public:

uiSeisBayesOut( uiParent* p, IOPar& pars, bool is2d )
    : uiDialog(p,uiDialog::Setup("Bayesian Classification",
				 "Select and specify output",mTODOHelpID)
	    			  .modal(false))
    , pars_(pars)
    , is2d_(is2d)
    , nrvars_(0)
{
    setCancelText( "&<< Back" );
    if ( is2d_ ) { new uiLabel( this, "2D not implemented" ); return; }

    BufferString emsg;
    PtrMan<ProbDenFunc> pdf = getPDF( pars_.find( mGetSeisBayesPDFIDKey(0) ),
	    				emsg );
    if ( !pdf ) { new uiLabel(this,emsg); return; }
    nrvars_ = pdf->nrDims();

    for ( int idx=0; idx<cMaxPDFs; idx++ )
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

~uiSeisBayesOut()
{
}

#define mErrRet(s) { uiMSG().error(s); return false; }

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

    IOPar&			pars_;
    bool			is2d_;
    int				nrvars_;

    ObjectSet<uiSeisSel>	flds3d_;
    uiSeisSubSel*		subselfld_;


};


void uiSeisBayesClass::doOutput()
{
    outdlg_ = new uiSeisBayesOut( parent_, pars_, is2d_ );
    mLaunchDlg(outdlg_,outputDone);
    mSetState( Wait4Dialog );
}


void uiSeisBayesClass::outputDone( CallBacker* )
{
    if ( !outdlg_->uiResult() )
	{ outdlg_ = 0; mSetState( InpSeis ); }

    SeisBayesClass exec( pars_ );
    uiTaskRunner tr( outdlg_ );
    const bool isok = tr.execute( exec );
    mSetState( isok ? Finished : Output );
}


uiSeisBayesClass::~uiSeisBayesClass()
{
    delete inppdfdlg_;
    delete inpseisdlg_;
    delete outdlg_;
}
