/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          September 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: ui2dgeomman.cc,v 1.2 2010-09-29 07:24:07 cvssatyaki Exp $";


#include "ui2dgeomman.h"

#include "bufstringset.h"
#include "surv2dgeom.h"

#include "uibutton.h"
#include "uilistbox.h"
#include "uiseparator.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitable.h"


ui2DGeomManageDlg::ui2DGeomManageDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("2D Geometry management", "Manage 2D lines",
				 "mTODOHelpID"))
{
    setCtrlStyle( LeaveOnly );

    BufferStringSet linesets;
    PosInfo::POS2DAdmin().getLineSets( linesets );
    uiLabeledListBox* lslb =
	new uiLabeledListBox( this, linesets, "Linesets", false,
			      uiLabeledListBox::AboveMid );
    linesetfld_ = lslb->box();
    linesetfld_->selectionChanged.notify(
	    mCB(this,ui2DGeomManageDlg,lineSetSelCB) );
    linesetfld_->setPrefWidth( 200 );
    uiSeparator* versep = new uiSeparator( this, "", false );
    versep->attach( rightTo, lslb );

    uiLabeledListBox* lnlb =
	new uiLabeledListBox( this, "Linenames", false,
			      uiLabeledListBox::AboveMid );
    lnlb->attach( rightTo, versep );
    linenamefld_ = lnlb->box();
    linenamefld_->setPrefWidth( 200 );
    
    mangeombut_ =
	new uiToolButton( this, "Manage Line Geometry", "man_linegeom.png",
			  mCB(this,ui2DGeomManageDlg,manLineGeom) );
    mangeombut_->attach( rightAlignedBelow, lnlb );
    lineSetSelCB( 0 );
}


ui2DGeomManageDlg::~ui2DGeomManageDlg()
{
}


void ui2DGeomManageDlg::lineSetSelCB( CallBacker* )
{
    BufferStringSet linenames;
    PosInfo::POS2DAdmin().setCurLineSet( linesetfld_->getText() );
    PosInfo::POS2DAdmin().getLines( linenames );
    linenamefld_->empty();
    linenamefld_->addItems( linenames );
}

mClass uiManageLineGeomDlg : public uiDialog
{
    public:
uiManageLineGeomDlg( uiParent* p, const char* linenm )
    : uiDialog( p, uiDialog::Setup("Manage Line Geomtery",linenm,"mTODOHelpID"))
    , linenm_(linenm)
{
    BufferString lbl( "Lineset : ");
    lbl.add( PosInfo::POS2DAdmin().curLineSet() );
    lbl.add( ", Linename : ");
    lbl.add( linenm );
    
    uiLabel* titllbl = new uiLabel( this, lbl );
    titllbl->attach( hCentered );
    
    PosInfo::Line2DData geom( linenm );
    PosInfo::POS2DAdmin().getGeometry( geom );
    const TypeSet<PosInfo::Line2DPos>& positions = geom.positions();
    table_ = new uiTable( this, uiTable::Setup(positions.size(),3), "2DGeom" );
    table_->attach( ensureBelow, titllbl );
    table_->setPrefWidth( 300 );
    BufferStringSet collbls;
    collbls.add( "Trace Number" ); collbls.add( "X" ); collbls.add( "Y" );
    table_->setColumnLabels( collbls );

    for ( int idx=0; idx<positions.size(); idx++ )
    {
	table_->setValue( RowCol(idx,0), positions[idx].nr_ );
	table_->setValue( RowCol(idx,1), positions[idx].coord_.x );
	table_->setValue( RowCol(idx,2), positions[idx].coord_.y );
    }
}

bool acceptOK( CallBacker* )
{
    if ( !uiMSG().askGoOn("Do you really want to change the geometry?") )
	return false;
    PosInfo::Line2DData geom( linenm_ );
    for ( int idx=0; idx<table_->nrRows(); idx++ )
    {
	PosInfo::Line2DPos l2d( table_->getIntValue(RowCol(idx,0)) );
	l2d.coord_.x = table_->getdValue( RowCol(idx,1) );
	l2d.coord_.y = table_->getdValue( RowCol(idx,2) );
	geom.add( l2d );
    }

    PosInfo::POS2DAdmin().setGeometry( geom );
    return true;
}
    const char* linenm_;
    uiTable*	table_;
};

void ui2DGeomManageDlg::manLineGeom( CallBacker* )
{
    uiManageLineGeomDlg dlg( this, linenamefld_->getText() );
    dlg.go();
}
