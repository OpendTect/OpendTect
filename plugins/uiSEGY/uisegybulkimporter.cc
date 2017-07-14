/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
________________________________________________________________________

-*/

#include "uisegybulkimporter.h"
#include "filepath.h"

#include "uibutton.h"
#include "uisegydef.h"
#include "uisegyreadstarter.h"
#include "uitable.h"

uiSEGYBulkImporter::uiSEGYBulkImporter( uiParent* p,
					const BufferStringSet& selfiles )
    : uiDialog(p, uiDialog::Setup(tr("Import Bulk SEGY Data"),
	       mNoDlgTitle,mNoHelpKey) )
    , selfilenms_(selfiles)
{
    bulktable_ = new uiTable( this, uiTable::Setup(), "Bulk Import Table" );
    bulktable_->setPrefWidth( 700 );
    bulktable_->setSelectionMode( uiTable::SingleRow );
    fillTable();
}


void uiSEGYBulkImporter::fillTable()
{
    const char* collbls[] = { "File name", "Data type",
			      "Output name","Advanced", 0 };
    bulktable_->setNrCols( 4 );
    bulktable_->setColumnLabels( collbls );
    int nrselfiles = selfilenms_.size();
    if( !nrselfiles )
	return;

    bulktable_->setNrRows( nrselfiles );
    for ( int idx=0; idx<nrselfiles; idx++ )
    {
	File::Path fp( selfilenms_.get(idx) );
	bulktable_->setText( RowCol(idx,0), fp.fileName().str() );
	uiSEGYImpType* typfld = new uiSEGYImpType( 0, false, 0, false );
	bulktable_->setCellObject( RowCol(idx,1), &typfld->asUiObject() );
	bulktable_->setText( RowCol(idx,2), fp.baseName().str() );
	uiPushButton* advancebut = new uiPushButton(0, uiStrings::sOptions(),
				mCB(this,uiSEGYBulkImporter,advanceCB), true );
	bulktable_->setCellObject(RowCol(idx,3), advancebut );
	bulktable_->setCellToolTip( RowCol(idx,0),
				    toUiString(selfilenms_.get(idx)) );
    }

    bulktable_->setColumnReadOnly( 0, true );
}


void uiSEGYBulkImporter::advanceCB( CallBacker* cb )
{
    mDynamicCastGet(uiPushButton*,obj,cb);

    if ( !obj )
	return;

    RowCol curcell = bulktable_->getCell( obj );
    uiGroup* dataobj = bulktable_->getCellGroup( RowCol(curcell.row(),1) );
    mDynamicCastGet(uiSEGYImpType*,datatype,dataobj)
    if ( !datatype )
	return;

    const SEGY::ImpType imptype = datatype->impType();
    uiSEGYReadStarter readstdlg( this, false, &imptype,
				 selfilenms_.get(curcell.row()), true );
    readstdlg.go();
}
