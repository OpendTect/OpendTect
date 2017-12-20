/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
________________________________________________________________________

-*/

#include "uisegybulkimporter.h"

#include "repos.h"
#include "rowcol.h"

#include "uisegyfileselector.h"
#include "uisegyreadfinisher.h"
#include "uisegyreadstarter.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"



uiSEGYMultiVintageImporter::uiSEGYMultiVintageImporter( uiParent* p )
    : uiDialog(p, uiDialog::Setup(tr("Import Bulk SEGY Data"),
	       mNoDlgTitle,mNoHelpKey) )
    , fsdlg_(0)
    , rsdlg_(0)
{
    setOkText( tr("Next >>") );

    imptypefld_ = new uiSEGYImpType( this, false, 0, true );
    imptypefld_->asUiObject().setSensitive( false );
    table_ = new uiTable( this, uiTable::Setup(), "Bulk Import Table" );
    table_->setPrefWidth( 650 );
    table_->setSelectionMode( uiTable::SingleRow );
    table_->attach( centeredBelow, imptypefld_ );
    table_->setNrCols( 3 );
    table_->setColumnLabel( 0, tr("Vintage") );
    table_->setColumnLabel( 1, tr("File names") );
    table_->setColumnLabel( 2, tr("") );
    table_->setNrRows(0);
    uiGroup* toolgrp = new uiGroup( this, "Tool group" );
    uiToolButton* addbut = new uiToolButton(toolgrp, "plus",
				    uiStrings::phrAdd(tr("Vintage")),
				    mCB(this,uiSEGYMultiVintageImporter,addCB));
    uiToolButton* removebut = new uiToolButton(toolgrp, "minus",
				 uiStrings::phrRemove(tr("Vintage")),
				 mCB(this,uiSEGYMultiVintageImporter,removeCB));
    removebut->attach( alignedBelow, addbut );
    uiToolButton* editbut = new uiToolButton(toolgrp, "edit",
				uiStrings::phrEdit(tr("Vintage")),
				mCB(this,uiSEGYMultiVintageImporter,editVntCB));
    editbut->attach( alignedBelow, removebut );
    toolgrp->attach( rightOf, table_ );
    addCB( 0 );
}


uiSEGYMultiVintageImporter::~uiSEGYMultiVintageImporter()
{
    deepErase( vntinfos_ );
    if ( rsdlg_ ) delete rsdlg_;
    if ( fsdlg_ ) delete fsdlg_;
}


bool uiSEGYMultiVintageImporter::selectVintage()
{
    while( true ) // do not let user escape before good selection or cancel
    {
	const SEGY::ImpType imptype = imptypefld_->impType();
	uiSEGYReadStarter::Setup rsdlgsu( false, &imptype );
	rsdlgsu.filenm( 0 ).fixedfnm( false ).vintagecheckmode( true );
	vntinfos_.isEmpty() ? rsdlgsu.vntinfos( 0 )
			    : rsdlgsu.vntinfos( &vntinfos_ );
	rsdlg_ = new uiSEGYReadStarter( this, rsdlgsu );
	if ( !rsdlg_->go() )
	    return false;

	fsdlg_ = new uiSEGYFileSelector( this, rsdlg_->userFileName(),
					 imptype, vntinfos_ );
	if ( fsdlg_->isEmpty() )
	{
	    uiMSG().message( tr("No files found."
			"\nAll the files at this location have already "
			"been added to a vintage in bulk import table.") );
	    delete fsdlg_;
	    return false;
	}

	fsdlg_->setVintagName( rsdlg_->getCurrentParName() );

	if ( fsdlg_->go() )
	    break;
    }

    fsdlg_->getVintagName( vintagenm_ );
    saveIfNewVintage( vintagenm_ );
    fsdlg_->getSelNames( selfilenms_ );
    selfilenms_.sort();
    uiSEGYVintageInfo* vntinfo = new uiSEGYVintageInfo();
    vntinfo->vintagenm_ = vintagenm_;
    vntinfo->filenms_ = selfilenms_;
    vntinfo->fp_ = File::Path( rsdlg_->userFileName() );
    vntinfos_.add( vntinfo );

    return true;
}


void uiSEGYMultiVintageImporter::saveIfNewVintage( const BufferString& vntnm )
{
    Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
    int selidx = parset.find( vntnm );
    bool newvnt = selidx < 0;
    if ( !newvnt )
	return;

    IOPar par;
    rsdlg_->getDefaultPar( par );
    Repos::IOPar* iop = new Repos::IOPar( par );
    iop->setName( vntnm );
    parset.add( iop );
    parset.write( Repos::Data );
    return;
}


void uiSEGYMultiVintageImporter::addCB( CallBacker* )
{
    if ( !selectVintage() )
	return;

    table_->setNrRows( table_->nrRows() + 1 );
    CallBack selcb = mCB(this,uiSEGYMultiVintageImporter,selectFilesCB);
    uiPushButton* selbut = new uiPushButton( 0, uiStrings::sSelect(),
					     selcb, false );
    selbut->setIcon( "selectfromlist" );
    int rowidx =  table_->nrRows() - 1;
    table_->setCellObject( RowCol(rowidx,2), selbut );
    table_->setColumnStretchable( 2, false );
    table_->setColumnReadOnly( 0, true );
    table_->setColumnReadOnly( 1, true );
    fillRow( rowidx );
}


void uiSEGYMultiVintageImporter::fillRow( int rowid )
{
    uiSEGYVintageInfo* vntinfo = vntinfos_.get( rowid );
    if ( !vntinfo )
	pErrMsg("Something went wrong.segysetup file modified?; return;");

    BufferStringSet selnms( vntinfo->filenms_ );
    BufferString dispstr( selnms.getDispString( selnms.size(), true) );
    table_->setText( RowCol(rowid,0), vintagenm_.buf() );
    table_->setText( RowCol(rowid,1), dispstr );
}


void uiSEGYMultiVintageImporter::selectFilesCB( CallBacker* cb )
{
     mDynamicCastGet(uiPushButton*,obj,cb);
     if ( !obj )
	 return;

     const RowCol rc( table_->getCell(obj) );
     uiSEGYVintageInfo* vntinfo = vntinfos_.get( rc.row() );
     if ( !vntinfo )
	 return;

     const BufferStringSet selnms( vntinfo->filenms_ );
     uiSEGYFileSelector segyfs( this, vntinfo->fp_.fullPath(),
				imptypefld_->impType(), vntinfos_, true,
				vntinfo->vintagenm_);
     segyfs.setSelectableNames( selnms, true );
     if ( !segyfs.go() )
	 return;

     BufferStringSet editednms;
     segyfs.getSelNames( editednms );
     editednms.sort();
     vntinfo->filenms_ = editednms;
     fillRow( rc.row() );

}


void uiSEGYMultiVintageImporter::removeCB( CallBacker* cb )
{
    if ( !uiMSG().askGoOn( tr("Do you want to delete the selected row?") ))
	return;

    const int rowid( table_->currentRow() );
    table_->removeRow( rowid );
}


void uiSEGYMultiVintageImporter::editVntCB( CallBacker* )
{
    int currow = table_->currentRow();
    BufferString vntnm( table_->text( RowCol(currow,0) ) );
    Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
    int selidx = parset.find( vntnm );
    if ( selidx < 0 )
	return;

    Repos::IOPar* iop = parset[selidx];
    if ( !iop )
	return;

    BufferString fnm;
    iop->get( "File name", fnm );

    uiSEGYReadStarter::Setup su( false, &imptypefld_->impType() );
    su.filenm(fnm).fixedfnm(true).vintagecheckmode(true).vintagenm(vntnm);
    uiSEGYReadStarter* readstrdlg = new uiSEGYReadStarter( this, su );
    readstrdlg->setCaption( tr("Edit vintage '%1'").arg(vntnm) );
    readstrdlg->usePar( *iop );

    if ( !readstrdlg->go() )
	return;
}

bool uiSEGYMultiVintageImporter::acceptOK()
{
    const Seis::GeomType gt = imptypefld_->impType().geomType();
    Repos::IOParSet parset = Repos::IOParSet( "SEGYSetups" );
    int selidx = parset.find( vntinfos_[0]->vintagenm_ );
    if ( selidx < 0 )
	return false;

    Repos::IOPar* iop = parset[selidx];
    if ( !iop )
	return false;

    FullSpec fullspec( gt, false);
    fullspec.usePar( *iop );

    uiSEGYReadFinisher dlg( this, fullspec,
			    uiStrings::sEmptyString().getOriginalString(),
			    true, false, &vntinfos_ );
    return dlg.go();
}
