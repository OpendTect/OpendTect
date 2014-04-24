/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene
 Date:          November 2013
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimathpropeddlg.h"
#include "uimathformula.h"
#include "uirockphysform.h"
#include "uitoolbutton.h"
#include "mathproperty.h"
#include "od_helpids.h"


uiMathPropEdDlg::uiMathPropEdDlg( uiParent* p, MathProperty& pr,
				  const PropertyRefSelection& prs )
    : uiDialog( p, Setup("Math property",
		BufferString("Value generation by formula for ",pr.name()),
		mODHelpKey(mRockPhysFormHelpID)) )
    , prop_(pr)
{
    uiMathFormula::Setup umfsu( "Formula (like den * vel)" );
    umfsu.proptype( prop_.ref().stdType() );
    umfsu.stortype( "Math Property" );
    formfld_ = new uiMathFormula( this, prop_.getForm(), umfsu );

    BufferStringSet availpropnms;
    for ( int idx=0; idx<prs.size(); idx++ )
    {
	const PropertyRef* ref = prs[idx];
	if ( ref != &pr.ref() )
	    availpropnms.add( ref->name() );
    }
    formfld_->setNonSpecInputs( availpropnms );

    uiToolButtonSetup tbsu( "rockphys", "Choose RockPhysics Formula",
		    mCB(this,uiMathPropEdDlg,rockPhysReq), "&Rock Physics");
    formfld_->addButton( tbsu );
}


uiMathPropEdDlg::~uiMathPropEdDlg()
{
}


void uiMathPropEdDlg::rockPhysReq( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("Rock Physics",
		  "Use a rock physics formula", "dgb:108.1.5") );
    uiRockPhysForm* rpffld = new uiRockPhysForm( &dlg, prop_.ref().stdType() );
    TypeSet<PropertyRef::StdType> inputtypes;
    if ( dlg.go() && rpffld->getFormulaInfo(prop_.getForm(),&inputtypes) )
	formfld_->useForm( &inputtypes );
}


bool uiMathPropEdDlg::acceptOK( CallBacker* )
{
    if ( formfld_->updateForm() )
	return true;

    return false;
}
