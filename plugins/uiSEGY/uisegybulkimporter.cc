/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
________________________________________________________________________

-*/

#include "uisegybulkimporter.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uisegyreadstarter.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "repos.h"
#include "survinfo.h"
#include "uisegyfileselector.h"


uiSEGYBulkImporter::uiSEGYBulkImporter( uiParent* p )
    : uiDialog(p, uiDialog::Setup(tr("Import Bulk SEGY Data"),
	       mNoDlgTitle,mNoHelpKey) )
{
    imptypefld_ = new uiSEGYImpType( this, false, 0, true );
    table_ = new uiTable( this, uiTable::Setup(), "Bulk Import Table" );
    table_->setPrefWidth( 650 );
    table_->setSelectionMode( uiTable::SingleRow );
    table_->attach( centeredBelow, imptypefld_ );
    table_->setNrCols( 4 );
    table_->setColumnLabel( 0, tr("Vintage") );
    table_->setColumnLabel( 1, tr("Files") );
    table_->setColumnLabel( 2, tr("Select") );
    table_->setColumnLabel( 3, tr("Output name") );
    table_->setNrRows(0);
    uiGroup* toolgrp = new uiGroup( this, "Tool group" );
    uiToolButton* addbut = new uiToolButton(toolgrp, "plus",
					    uiStrings::phrAdd(tr("Vintage")),
					    mCB(this,uiSEGYBulkImporter,addCB));
    uiToolButton* removebut = new uiToolButton(toolgrp, "minus",
					 uiStrings::phrRemove(tr("Vintage")),
					 mCB(this,uiSEGYBulkImporter,removeCB));
    removebut->attach( alignedBelow, addbut );
    toolgrp->attach( rightOf, table_ );
    addCB( 0 );
}


bool uiSEGYBulkImporter::selectVintage()
{
    const SEGY::ImpType imptype = imptypefld_->impType();
    uiSEGYReadStarter::Setup su( false, &imptype );
    su.filenm(0).fixedfnm(false).vintagecheckmode(true);
    uiSEGYReadStarter* readstrdlg = new uiSEGYReadStarter( this, su);
    if ( !readstrdlg->go() )
	return false;

    readstrdlg->getVintagName( vintagenm_ );
    uiSEGYFileSelector* fsdlg =
			new uiSEGYFileSelector( this,
						readstrdlg->userFileName(),
						vintagenm_.buf() );
    if ( fsdlg->go() )
	fsdlg->getSelNames( selfilenms_ );

    return true;
}


void uiSEGYBulkImporter::addCB( CallBacker* )
{
    if ( !selectVintage() )
	return;

    table_->setNrRows( table_->nrRows() + 1 );
    CallBack selcb = mCB(this,uiSEGYBulkImporter,selectFilesCB);
    uiPushButton* selbut = new uiPushButton( 0, uiStrings::sSelect(),
					     selcb, false );
    selbut->setIcon( "selectfromlist" );
    int rowidx =  table_->nrRows() - 1;
    table_->setCellObject( RowCol(rowidx,2), selbut );
    table_->setColumnStretchable( 2, false );
    table_->setColumnReadOnly( 0, true );
    fillRow( rowidx );
}


void uiSEGYBulkImporter::fillRow( int rowid )
{
    BufferString dispstr( selfilenms_.getDispString( selfilenms_.size(),
						     true) );
    table_->setText( RowCol(rowid,0), vintagenm_.buf() );
    table_->setText( RowCol(rowid,1), dispstr );
}


void uiSEGYBulkImporter::selectFilesCB( CallBacker*)
{
}


void uiSEGYBulkImporter::removeCB( CallBacker* )
{
}


bool uiSEGYBulkImporter::acceptOK()
{
    //TODO Importing selected SEGY files
    return true;
}
