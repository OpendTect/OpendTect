/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2010
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uieditpdf.cc,v 1.4 2010-03-01 09:29:18 cvsbert Exp $";

#include "uieditpdf.h"

#include "uigeninput.h"
#include "uitabstack.h"
#include "uitable.h"
#include "uimsg.h"

#include "sampledprobdenfunc.h"

#define mDeclArrNDPDF(var) mDynamicCastGet(ArrayNDProbDenFunc*,var,&pdf_)
#define mDeclNrDims \
    const int nrdims = pdf_.nrDims()
#define mDeclSzVars(var) \
    const int nrtbltabs = nrdims > 2 ? var->size(2) : 1; \
    const int nrrows = var->size( 0 ); \
    const int nrcols = nrdims < 2 ? 1 : var->size( 1 )
#define mDeclArrVars(var) \
    ArrayND<float>& data = var->getData(); \
    TypeSet<int> idxs( nrdims < 3 ? 2 : nrdims, 0 )
#define mDeclVars(var) \
    mDeclNrDims; mDeclArrNDPDF(var) mDeclSzVars(var); mDeclArrVars(var)


uiEditProbDenFunc::uiEditProbDenFunc( uiParent* p, ProbDenFunc& pdf, bool ed )
    : uiDialog(p,uiDialog::Setup(
	BufferString( ed ? "Edit" : "Browse"," Probability Density Function"),
	BufferString( ed ? "Edit '" : "Browse '",
		      pdf.name().isEmpty() ? "PDF" : pdf.name().buf(),
		      "'"),
	mTODOHelpID))
    , pdf_(pdf)
    , editable_(ed)
    , chgd_(false)
{
    mDeclNrDims;
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
    tabstack_->addTab( dimnmgrp, nrdims < 2 ? "Name" : "Names" );

    mDeclArrNDPDF(andpdf);
    if ( !andpdf || nrdims > 3 )
	return;

    mDeclSzVars(andpdf);
    uiTable::Setup su( nrrows, nrcols );
    su.rowdesc( pdf_.dimName(0) )
      .coldesc( nrdims > 1 ? pdf_.dimName(1) : "Values" )
      .fillrow(true).fillcol(true)
      .manualresize(true).sizesFixed(true);

    mDeclArrVars(andpdf);
    for ( int itab=0; itab<nrtbltabs; itab++ )
    {
	BufferString tabnm( "Values" );
	if ( nrdims > 2 ) tabnm.add(" [").add(itab+1).add("]");
	uiGroup* grp = new uiGroup( tabstack_->tabGroup(),
				    BufferString(tabnm," group").buf() );

	uiTable* tbl = new uiTable( grp, su, BufferString("dim ",itab) );
	tbl->insertColumns( 0, nrcols );
	tbl->insertRows( 0, nrrows );
	if ( nrcols == 1 )
	    tbl->setColumnLabel( 0, "Value" );
	else
	{
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		const float val = andpdf->sampling(1).atIndex(icol);
		tbl->setColumnLabel( icol, toString(val) );
	    }
	}

	if ( nrdims > 2 ) idxs[2] = itab;
	for ( int irow=0; irow<nrrows; irow++ )
	{
	    idxs[0] = irow;
	    const float rowval = andpdf->sampling(0).atIndex(irow);
	    tbl->setRowLabel( irow, toString(rowval) );
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		idxs[1] = icol;
		const float arrval = data.getND( idxs.arr() );
		tbl->setValue( RowCol(irow,icol), arrval );
	    }
	}
	tbls_ += tbl;
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
	const BufferString newnm( nmflds_[idim]->text() );
	if ( newnm != pdf_.dimName(idim) )
	{
	    pdf_.setDimName( idim, nmflds_[idim]->text() );
	    chgd_ = true;
	}
    }

    return tbls_.isEmpty() ? true : getTblVals();
}


bool uiEditProbDenFunc::getTblVals()
{
    mDeclVars(andpdf);

    for ( int itab=0; itab<nrtbltabs; itab++ )
    {
	const uiTable& tbl = *tbls_[itab];

	if ( nrdims > 2 ) idxs[2] = itab;
	for ( int irow=0; irow<nrrows; irow++ )
	{
	    idxs[0] = irow;
	    for ( int icol=0; icol<nrcols; icol++ )
	    {
		BufferString bstbltxt = tbl.text( RowCol(irow,icol) );
		char* tbltxt = bstbltxt.buf();
		mTrimBlanks(tbltxt);
		if ( !*tbltxt )
		{
		    uiMSG().error("Please fill all cells - or use 'Cancel'");
		    return false;
		}

		idxs[1] = icol;
		const float tblval = atof( tbltxt );
		const float arrval = data.getND( idxs.arr() );
		if ( !mIsEqual(tblval,arrval,mDefEps) )
		{
		    data.setND( idxs.arr(), tblval );
		    chgd_ = true;
		}
	    }
	}
    }

    return true;
}
