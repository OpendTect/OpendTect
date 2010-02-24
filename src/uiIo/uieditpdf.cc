/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uieditpdf.cc,v 1.2 2010-02-24 15:09:47 cvsbert Exp $";

#include "uieditpdf.h"

#include "uigeninput.h"
#include "uitabstack.h"
#include "uilabel.h"
#include "uimsg.h"

#include "sampledprobdenfunc.h"


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
    const int nrdims = pdf_.nrDims();

    tabstack_ = new uiTabStack( this, "Tabs" );

    uiGroup* dimnmgrp = new uiGroup( tabstack_->tabGroup(), "Dimension names" );
    for ( int idim=0; idim<nrdims; idim++ )
    {
	BufferString txt;
	if ( nrdims > 1 )
	    txt.add( "Name of dimension " ).add( idim + 1 );
	else
	    txt = "Variable name";
	uiGenInput* nmfld = new uiGenInput( dimnmgrp, txt, pdf_.dimName(idim) );
	if ( idim )
	    nmfld->attach( alignedBelow, nmflds_[idim-1] );
	nmflds_ += nmfld;
	if ( !editable_ )
	    nmfld->setReadOnly( true );
    }
    tabstack_->addTab( dimnmgrp, "Names" );

    mDynamicCastGet(ArrayNDProbDenFunc*,andpdf,&pdf_)
    if ( !andpdf || nrdims > 3 )
	return;

    const int nrtabs = nrdims > 2 ? andpdf->size(2) : 1;
    for ( int itab=0; itab<nrtabs; itab++ )
    {
	BufferString tabnm( "Values" );
	if ( nrdims > 2 ) tabnm.add(" [").add(itab+1).add("]");
	uiGroup* grp = new uiGroup( tabstack_->tabGroup(),
				    BufferString(tabnm," group").buf() );
	new uiLabel( grp, BufferString("TODO ",itab) );

	tabstack_->addTab( grp, tabnm );
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
