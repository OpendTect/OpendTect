/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uieditpdf.cc,v 1.1 2010-02-18 16:07:19 cvsbert Exp $";

#include "uieditpdf.h"

#include "uigeninput.h"
#include "uimsg.h"

#include "probdenfunc.h"


uiEditProbDenFunc::uiEditProbDenFunc( uiParent* p, ProbDenFunc& pdf, bool ed )
    : uiDialog(p,uiDialog::Setup(
	BufferString( ed ? "Edit" : "Browse"," Probability Density Function"),
	BufferString( ed ? "Edit '" : "Browse '",
		      pdf.name().isEmpty() ? "PDF" : pdf.name().buf(),
		      "'"),
	mTODOHelpID))
    , pdf_(pdf)
    , editable_(ed)
{
    for ( int idim=0; idim<pdf_.nrDims(); idim++ )
    {
	BufferString txt( "Name of variable ", idim + 1 );
	uiGenInput* nmfld = new uiGenInput( this, txt, pdf_.dimName(idim) );
	if ( idim )
	    nmfld->attach( alignedBelow, nmflds_[idim-1] );
	nmflds_ += nmfld;
	if ( !editable_ )
	    nmfld->setReadOnly( true );
    }
}


uiEditProbDenFunc::~uiEditProbDenFunc()
{
}


bool uiEditProbDenFunc::acceptOK( CallBacker* )
{
    if ( !editable_ ) return true;

    for ( int idim=0; idim<pdf_.nrDims(); idim++ )
    {
	pdf_.setDimName( idim, nmflds_[idim]->text() );
    }
    return true;
}
