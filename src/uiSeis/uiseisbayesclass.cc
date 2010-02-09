/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseisbayesclass.cc,v 1.1 2010-02-09 16:01:56 cvsbert Exp $";

#include "uiseisbayesclass.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiobjdisposer.h"
#include "odusginfo.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "probdenfunc.h"
#include "probdenfunctr.h"
#include "seistrctr.h"

// for test
#	include "uilabel.h"


#define mSetState(st) { state_ = st; nextAction(); return; }
static const int cMaxPDFs = 5;
const char* uiSeisBayesClass::sKeyPDFID()		{ return "PDF.ID"; }
#define mGetPDFIDKey(nr) IOPar::compKey(uiSeisBayesClass::sKeyPDFID(),nr)


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
				 "Specify PDF input",mTODOHelpID))
    , pars_(pars)
{
    IOObjContext ctxt( mIOObjContext(ProbDenFunc) );
    ctxt.forread = true;
    for ( int idx=0; idx<cMaxPDFs; idx++ )
    {
	uiIOObjSel* fld = new uiIOObjSel( this, ctxt,
				BufferString("Input PDF ",idx+1) );
	if ( idx )
	    fld->attach( alignedBelow, flds_[idx-1] );

	// Fill inps from pars_

	flds_ += fld;
    }

    setModal( true );
}

~uiSeisBayesPDFInp()
{
}

bool acceptOK( CallBacker* )
{
    // Put inps into pars_
    pars_.set( mGetPDFIDKey(1), flds_[0]->key() );
    return true;
}

    IOPar&			pars_;

    ObjectSet<uiIOObjSel>	flds_;

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


class uiSeisBayesSeisInp : public uiDialog
{
public:

uiSeisBayesSeisInp( uiParent* p, IOPar& pars, bool is2d )
    : uiDialog(p,uiDialog::Setup("Bayesian Classification",
				 "Specify Seismic input",mTODOHelpID))
    , pars_(pars)
    , lsfld_(0)
{
    const char* id1 = pars_.find( mGetPDFIDKey(1) );
    if ( !id1 || !*id1 ) { new uiLabel(this,"Huh?-ID"); return; }
    PtrMan<IOObj> ioobj = IOM().get( MultiID(id1) );
    if ( !ioobj ) { new uiLabel(this,"Huh?-IOObj"); return; }
    BufferString emsg;
    ProbDenFunc* pdf = ProbDenFuncTranslator::read(*ioobj,&emsg);
    if ( !pdf ) { new uiLabel(this,emsg); return; }

    const int nrvars = pdf->nrDims();
    const Seis::GeomType gt = is2d ? Seis::Line : Seis::Vol;
    uiSeisSel::Setup su( gt );
    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    uiSeisSel::fillContext( gt, true, ctxt );

    if ( is2d )
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
	    if ( idx )
		fld->attach( alignedBelow, flds3d_[idx-1] );
	    flds3d_ += fld;
	}
    }

    setModal( true );
}

~uiSeisBayesSeisInp()
{
}

bool acceptOK( CallBacker* )
{
    return true;
}

    IOPar&			pars_;

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


void uiSeisBayesClass::doOutput()
{
}


void uiSeisBayesClass::outputDone( CallBacker* )
{
}


uiSeisBayesClass::~uiSeisBayesClass()
{
    delete inppdfdlg_;
    delete inpseisdlg_;
    // delete outdlg_;
}
