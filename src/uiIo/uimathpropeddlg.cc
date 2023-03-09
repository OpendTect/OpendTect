/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimathpropeddlg.h"
#include "uimathformula.h"
#include "uimathexpression.h"
#include "uimsg.h"
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
    uiMathFormula::Setup umfsu( tr("Formula") );
    umfsu.mn( &prop_.mn() )
	 .stortype( "Math Property" )
	 .maxnrinps(8);
    formfld_ = new uiMathFormula( this, prop_.getForm(), umfsu );
    formfld_->exprFld()->setPlaceholderText( toUiString("density * velocity") );

    BufferStringSet availpropnms;
    MnemonicSelection mnsel;
    for ( int idx=0; idx<prs_.size(); idx++ )
    {
	const PropertyRef* ref = prs_[idx];
	if ( ref != &pr.ref() )
	{
	    availpropnms.add( ref->name() );
	    mnsel.add( &ref->mn() );
	}
    }

    formfld_->setNonSpecInputs( availpropnms, -1, &mnsel );

    uiToolButtonSetup tbsu( "rockphys", tr("Choose RockPhysics Formula"),
			    mCB(this,uiMathPropEdDlg,rockPhysReq),
			    uiStrings::sRockPhy() );
    formfld_->addButton( tbsu );
}


uiMathPropEdDlg::~uiMathPropEdDlg()
{
    detachAllNotifiers();
    delete &prs_;
}


void uiMathPropEdDlg::rockPhysReq( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(uiStrings::sRockPhy(),
		  tr("Use a rock physics formula"),
		  mODHelpKey(mRockPhysFormHelpID)) );
    auto* rpffld = new uiRockPhysForm( &dlg, prop_.mn() );
    const uiRetVal uirv = rpffld->isOK();
    if ( !uirv.isOK() )
    {
	uiMSG().error( uirv );
	return;
    }

    if ( !dlg.go() )
	return;

    rpffld->getFormulaInfo( prop_.getForm() );
}


bool uiMathPropEdDlg::acceptOK( CallBacker* )
{
    return formfld_->updateForm();
}
