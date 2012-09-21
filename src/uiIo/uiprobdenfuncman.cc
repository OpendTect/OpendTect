/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uiprobdenfuncman.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uieditpdf.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uiseparator.h"
#include "uimsg.h"

#include "bufstring.h"
#include "sampledprobdenfunc.h"
#include "probdenfunctr.h"

static const int cPrefWidth = 75;

uiProbDenFuncMan::uiProbDenFuncMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Probability Density Functions",
				     mNoDlgTitle,
				     "112.1.0").nrstatusflds(1),
	           ProbDenFuncTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp_->getListField()->doubleClicked.notify(
	    			mCB(this,uiProbDenFuncMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "browseprdf",
	    		 "Browse/edit this Probability Density Function",
			 mCB(this,uiProbDenFuncMan,browsePush) );
    manipgrp->addButton( "genprdf",
	    		 "Generate Probability Density Function",
			 mCB(this,uiProbDenFuncMan,genPush) );

    selgrp_->setPrefWidthInChar( cPrefWidth );
    infofld_->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}


uiProbDenFuncMan::~uiProbDenFuncMan()
{
}


#define mGetPDF(pdf) \
    PtrMan<ProbDenFunc> pdf = ProbDenFuncTranslator::read( *curioobj_ )


void uiProbDenFuncMan::browsePush( CallBacker* )
{
    if ( !curioobj_ ) return;
    mGetPDF(pdf);
    if ( !pdf ) return;

    uiEditProbDenFunc dlg( this, *pdf, true );
    if ( dlg.go() && dlg.isChanged() )
    {
	const int choice = uiMSG().question( "PDF changed. Save?", "&Yes",
						"&As new ...", "&No" );
	if ( choice < 0 ) return;

	PtrMan<IOObj> saveioobj = curioobj_->clone();
	if ( choice == 0 )
	{
	    CtxtIOObj ctio( ctxt_ );
	    ctio.ctxt.forread = false;
	    uiIOObjSelDlg seldlg( this, ctio, "Save As" );
	    if ( !seldlg.go() || !seldlg.ioObj() ) return;
	    saveioobj = seldlg.ioObj()->clone();
	}

	BufferString emsg;
	if ( !ProbDenFuncTranslator::write(*pdf,*saveioobj,&emsg) )
	    uiMSG().error( emsg );
	else
	    selgrp_->fullUpdate( saveioobj->key() );
    }
}


class uiProbDenFuncGen : public uiDialog
{
public:

uiProbDenFuncGen( uiParent* p )
    : uiDialog(p,Setup("Generate PDF",mNoDlgTitle,mTODOHelpID))
{
    const CallBack chgcb( mCB(this,uiProbDenFuncGen,chgCB) );

    uiLabeledSpinBox* lsb = new uiLabeledSpinBox( this, "Number of dimensions");
    nrdimsfld_ = lsb->box();
    nrdimsfld_->setInterval( 1, 3, 1 );
    nrdimsfld_->setValue( 2 );
    nrdimsfld_->valueChanging.notify( chgcb );

    gengaussfld_ = new uiGenInput( this, "Distribution type",
	    			   BoolInpSpec(true,"Gaussian","Uniform") );
    gengaussfld_->attach( alignedBelow, lsb );
    gengaussfld_->valuechanged.notify( chgcb );

    uiGroup* alfld = gengaussfld_;
    for ( int idx=0; idx<3; idx++ )
    {
	uiGenInput* nmfld = new uiGenInput( this,
			BufferString("Dimension ",idx+1,": Name") );
	uiGenInput* rgfld = new uiGenInput( this, "Range",
				FloatInpSpec(), FloatInpSpec() );
	nmflds_ += nmfld; rgflds_ += rgfld;
	nmfld->attach( alignedBelow, alfld );
	rgfld->attach( rightOf, nmfld );
	alfld = nmfld;
    }

    lsb = new uiLabeledSpinBox( this, "Size of the dimensions" );
    nrnodesfld_ = lsb->box();
    nrnodesfld_->setInterval( 3, 10000, 1 );
    nrnodesfld_->setValue( 25 );
    lsb->attach( alignedBelow, alfld );

    alfld = lsb;
    for ( int idx=0; idx<3; idx++ )
    {
	uiGenInput* expstdfld = new uiGenInput( this,
				BufferString("Dimension ",idx+1,": Exp/Std"),
				FloatInpSpec(), FloatInpSpec() );
	expstdfld->attach( alignedBelow, alfld );
	expstdflds_ += expstdfld;
	alfld = expstdfld;
    }
    dir01fld_ = new uiGenInput( this, "Angle (deg) Dim 1 -> Dim 2" );
    dir01fld_->setElemSzPol( uiObject::Small );
    dir01fld_->attach( rightOf, expstdflds_[1] );
    dir02fld_ = new uiGenInput( this, "Dim 1 -> Dim 3" );
    dir02fld_->attach( alignedBelow, dir01fld_ );
    dir02fld_->setElemSzPol( uiObject::Small );
    dir12fld_ = new uiGenInput( this, "Dim 2 -> Dim 3" );
    dir12fld_->attach( rightOf, dir02fld_ );
    dir12fld_->setElemSzPol( uiObject::Small );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, dir12fld_ );

    IOObjContext ctxt( ProbDenFuncTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt,"Output Probability Density Function");
    outfld_->attach( alignedBelow, alfld );
    outfld_->attach( ensureBelow, sep );

    postFinalise().notify( chgcb );
}

void chgCB( CallBacker* )
{
    const int nrdims = nrdimsfld_->getValue();
    const bool isgauss = gengaussfld_->getBoolValue();
    for ( int idx=0; idx<nmflds_.size(); idx++ )
    {
	const bool havedim = idx < nrdims;
	nmflds_[idx]->display( havedim );
	rgflds_[idx]->display( havedim );
	expstdflds_[idx]->display( havedim && isgauss );
    }
    dir01fld_->display( isgauss && nrdims > 1 );
    dir02fld_->display( isgauss && nrdims > 2 );
    dir12fld_->display( isgauss && nrdims > 2 );
}


MultiID newObjKey() const
{
    const IOObj* ioobj = outfld_->ioobj();
    return ioobj ? ioobj->key() : MultiID();
}

bool acceptOK( CallBacker* )
{
    uiMSG().error( "TODO: implement" );
    return false;
}

    uiSpinBox*			nrdimsfld_;
    uiSpinBox*			nrnodesfld_;
    uiGenInput*			gengaussfld_;
    ObjectSet<uiGenInput>	nmflds_;
    ObjectSet<uiGenInput>	rgflds_;
    ObjectSet<uiGenInput>	expstdflds_;
    uiGenInput*			dir01fld_;
    uiGenInput*			dir02fld_;
    uiGenInput*			dir12fld_;
    uiIOObjSel*			outfld_;			

};


void uiProbDenFuncMan::genPush( CallBacker* )
{
    uiProbDenFuncGen dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.newObjKey() );
}


void uiProbDenFuncMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    txt += getFileInfo();

    mGetPDF(pdf);
    if ( pdf )
    {
	IOPar par;
	pdf->fillPar( par );
	txt += BufferString( "Type: ", pdf->getTypeStr() );
	for ( int idx=0; idx<pdf->nrDims(); idx++ )
	{
	    BufferString lbl( "\nDimension ", idx+1, ": " );
	    txt += lbl; txt += pdf->dimName(idx);
	}
    }

    setInfo( txt );
}
