/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: ui2dgeomman.cc,v 1.1 2010-09-29 03:38:54 cvssatyaki Exp $";


#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "surv2dgeom.h"

#include "uilistbox.h"
#include "uiseparator.h"
#include "uimsg.h"


ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("2D Geometry management", "Manage 2D lines",
				 "mTODOHelpID"))
{
    setCtrlStyle( LeaveOnly );

    BufferStringSet linesets;
    PosInfo::POS2DAdmin().getLineSets( linesets );
    uiLabeledListBox* lslb = new uiLabeledListBox( this, linesets, "Linesets" );
    linesetfld_ = lslb->box();
    linesetfld_->selectionChanged.notify(
	    mCB(this,ui2DGeomManageDlg,lineSetSelCB) );
    uiSeparator* versep = new uiSeparator( this, "", false );
    versep->attach( rightTo, lslb );

    uiLabeledListBox* lnlb = new uiLabeledListBox( this, "Linenames" );
    lnlb->attach( rightTo, versep );
    linenamefld_ = lnlb->box();
    linesetfld_->selectionChanged.notify(
	    mCB(this,ui2DGeomManageDlg,lineNameSelCB) );
    lineSetSelCB( 0 );
}


ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


void ui2DGeomManageDlg::lineSetSelCB( CallBacker* )
{
    BufferStringSet linenames;
    PosInfo::POS2DAdmin().getLines( linenames, linesetfld_->getText() );
    linenamefld_->empty();
    linenamefld_->addItems( linenames );
}


void ui2DGeomManageDlg::lineNameSelCB( CallBacker* )
{
}
