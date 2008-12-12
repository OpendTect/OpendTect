/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultcomputils.cc,v 1.3 2008-12-12 09:30:18 cvshelene Exp $";

#include "uimultcompdlg.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "uispinbox.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, LineKey lkey )
    	: uiDialog(p,uiDialog::Setup("Component dialog",
		    		     "Choose the component to display","") )
	, needdisplay_(true)
	, compfld_(0)
{
    int nrcomp = SeisIOObjInfo::getNrCompAvail( lkey );

    if ( nrcomp>1 )
    {
	compfld_ = new uiLabeledSpinBox( this, "Component nr", 0, "Compfld" );
	compfld_->box()->setInterval( StepInterval<int>(1,nrcomp,1) );
    }
    else
	needdisplay_ = false;
}


int uiMultCompDlg::getCompNr() const
{
    return compfld_->box()->getValue() - 1;
}
