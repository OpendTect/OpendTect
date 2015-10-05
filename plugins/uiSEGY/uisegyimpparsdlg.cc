/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Oct 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id:$";

#include "uisegyimpparsdlg.h"
#include "uilistbox.h"
#include "uigeninput.h"



#include "uilabel.h"


uiSEGYReadImpParsDlg::uiSEGYReadImpParsDlg( uiParent* p, const char* defnm )
    : uiDialog(p,Setup(tr("Read SEG-Y setup"),mNoDlgTitle,mTODOHelpKey))
{
    new uiLabel( this, "TODO: implement" );
}


bool uiSEGYReadImpParsDlg::acceptOK( CallBacker* )
{
    return true;
}


uiSEGYStoreImpParsDlg::uiSEGYStoreImpParsDlg( uiParent* p, const IOPar& iop,
					      const char* defnm )
    : uiDialog(p,Setup(tr("Store SEG-Y setup"),mNoDlgTitle,mTODOHelpKey))
{
    new uiLabel( this, "TODO: implement" );
}


bool uiSEGYStoreImpParsDlg::acceptOK( CallBacker* )
{
    return true;
}
