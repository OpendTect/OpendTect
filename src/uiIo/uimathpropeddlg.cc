/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          November 2013
________________________________________________________________________

-*/

#include "uimathpropeddlg.h"
#include "uimathformula.h"
#include "uimathexpressionvariable.h"
#include "uirockphysform.h"
#include "uitoolbutton.h"
#include "mathproperty.h"
#include "od_helpids.h"


uiMathPropEdDlg::uiMathPropEdDlg( uiParent* p, MathProperty& pr,
				  const PropertyRefSelection& prs )
    : uiDialog( p, Setup(tr("Math property"),
		tr("Value generation by formula for %1")
		.arg(toUiString(pr.name())), mODHelpKey(mMathPropEdDlgHelpID)) )
    , prop_(pr)
    , prs_(*new PropertyRefSelection(prs))
{
    uiMathFormula::Setup umfsu( tr("Formula (like den * vel)") );
    umfsu.proptype( prop_.ref().stdType() )
	 .stortype( "Math Property" )
	 .maxnrinps(8);
    formfld_ = new uiMathFormula( this, prop_.getForm(), umfsu );
    formfld_->inpSet.notify( mCB(this,uiMathPropEdDlg,inpSel) );
    formfld_->formSet.notify( mCB(this,uiMathPropEdDlg,formSet) );

    BufferStringSet availpropnms;
    for ( int idx=0; idx<prs_.size(); idx++ )
    {
	const PropertyRef* ref = prs_[idx];
	if ( ref != &pr.ref() )
	    availpropnms.add( ref->name() );
    }
    formfld_->setNonSpecInputs( availpropnms );

    uiToolButtonSetup tbsu( "rockphys", tr("Choose RockPhysics Formula"),
		    mCB(this,uiMathPropEdDlg,rockPhysReq),
		    uiStrings::sRockPhy() );
    formfld_->addButton( tbsu );
}


uiMathPropEdDlg::~uiMathPropEdDlg()
{
    delete &prs_;
}


void uiMathPropEdDlg::formSet( CallBacker* )
{
    const int nrinps = formfld_->nrInputs();
    for ( int iinp=0; iinp<nrinps; iinp++ )
	setPType4Inp( iinp );
}


void uiMathPropEdDlg::inpSel( CallBacker* )
{
    const int inpidx = formfld_->inpSelNotifNr();
    if ( inpidx >= 0 && inpidx < formfld_->nrInputs() )
	setPType4Inp( inpidx );
}


void uiMathPropEdDlg::setPType4Inp( int inpidx )
{
    if ( formfld_->isSpec(inpidx) || formfld_->isConst(inpidx) )
	return;

    const PropertyRef* pr = prs_.getByName( formfld_->getInput(inpidx) );
    Mnemonic::StdType ptyp = pr ? pr->stdType() : Mnemonic::Other;
    formfld_->inpFld(inpidx)->setPropType( ptyp );
}


void uiMathPropEdDlg::rockPhysReq( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(uiStrings::sRockPhy(),
		  tr("Use a rock physics formula"),
		  mODHelpKey(mRockPhysFormHelpID)) );
    uiRockPhysForm* rpffld = new uiRockPhysForm( &dlg, prop_.ref().stdType() );
    TypeSet<Mnemonic::StdType> inputtypes;
    if ( dlg.go() && rpffld->getFormulaInfo(prop_.getForm(),&inputtypes) )
	formfld_->useForm( &inputtypes );
}


bool uiMathPropEdDlg::acceptOK( CallBacker* )
{
    if ( formfld_->updateForm() )
	return true;

    return false;
}
