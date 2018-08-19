/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2017
________________________________________________________________________

-*/

#include "uisegymultivintageimporter.h"

#include "repos.h"
#include "rowcol.h"

#include "uisegyfileselector.h"
#include "uisegyreadfinisher.h"
#include "uisegyreadstarter.h"
#include "segyvintageimporter.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitoolbutton.h"



uiSEGYMultiVintageImporter::uiSEGYMultiVintageImporter( uiParent* p )
    : uiDialog(p, uiDialog::Setup(tr("Import Bulk SEGY Data"),
	       mNoDlgTitle,mNoHelpKey) )
    , fsdlg_(0)
    , rsdlg_(0)
    , rfdlg_(0)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sDismiss() );
    imptypefld_ = new uiSEGYImpType( this, false, 0, true );
    imptypefld_->asUiObject().setSensitive( false );
    table_ = new uiTable( this, uiTable::Setup(), "Bulk Import Table" );
    table_->setPrefWidth( 700 );
    table_->setSelectionMode( uiTable::SingleRow );
    table_->attach( centeredBelow, imptypefld_ );
    table_->setNrCols( 5 );
    table_->setNrRows( 0 );
    table_->setColumnLabel( 0, uiStrings::sVintage() );
    table_->setColumnLabel( 1, tr("File names") );
    table_->setColumnLabel( 2, uiString::empty() );
    table_->setColumnLabel( 3, tr("Import status") );
    table_->setColumnReadOnly( 3, true );
    table_->setColumnLabel( 4, uiString::empty() );
    table_->setColumnWidth( 4, 1 );
    uiGroup* toolgrp = new uiGroup( this, "Tool group" );
    addbut_ = new uiToolButton(toolgrp, "create",
				    uiStrings::phrAdd(uiStrings::sVintage()),
				    mCB(this,uiSEGYMultiVintageImporter,addCB));
    removebut_ = new uiToolButton(toolgrp, "remove",
				 uiStrings::phrRemove(uiStrings::sVintage()),
				 mCB(this,uiSEGYMultiVintageImporter,removeCB));
    removebut_->attach( alignedBelow, addbut_ );
    editbut_ = new uiToolButton(toolgrp, "edit",
				uiStrings::phrEdit(uiStrings::sVintage()),
				mCB(this,uiSEGYMultiVintageImporter,editVntCB));
    editbut_->attach( alignedBelow, removebut_ );
    toolgrp->attach( rightOf, table_ );
    addCB( 0 );
}


uiSEGYMultiVintageImporter::~uiSEGYMultiVintageImporter()
{
    deepErase( vntinfos_ );
    if ( rsdlg_ ) delete rsdlg_;
    if ( fsdlg_ ) delete fsdlg_;
    if ( rfdlg_ ) delete rfdlg_;
}


bool uiSEGYMultiVintageImporter::selectVintage()
{
    while( true ) // do not let user escape before good selection or cancel
    {
	const SEGY::ImpType imptype = imptypefld_->impType();
	uiSEGYReadStarter::Setup rsdlgsu( false, 0 );
	rsdlgsu.filenm( 0 ).fixedfnm( false ).vintagecheckmode( true );
	vntinfos_.isEmpty() ? rsdlgsu.vntinfos( 0 )
			    : rsdlgsu.vntinfos( &vntinfos_ );
	rsdlg_ = new uiSEGYReadStarter( this, rsdlgsu );
	if ( !rsdlg_->go() )
	    return false;

	const Seis::GeomType gt = rsdlg_->impType().geomType();
	imptypefld_->setGeomType( gt );

	fsdlg_ = new uiSEGYFileSelector( this, rsdlg_->userFileName(),
					 imptype, vntinfos_ );
	if ( fsdlg_->isEmpty() )
	{
	    uiMSG().error( tr("No files found."
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
    SEGY::Vintage::Info* vntinfo = new SEGY::Vintage::Info();
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


void uiSEGYMultiVintageImporter::setButtonSensitivity( bool sensitive )
{
    const bool hasrows = table_->nrRows() != 0;
    removebut_->setSensitive( hasrows && sensitive );
    editbut_->setSensitive( hasrows && sensitive );
    uiButton* okbut = button( uiDialog::OK );
    if ( okbut ) okbut->setSensitive( hasrows && sensitive );
    addbut_->setSensitive( sensitive );
}


void uiSEGYMultiVintageImporter::addCB( CallBacker* )
{
    if ( !selectVintage() )
	return;

    table_->setNrRows( table_->nrRows() + 1 );
    int rowidx =  table_->nrRows() - 1;
    table_->setColumnReadOnly( 0, true );
    table_->setColumnReadOnly( 1, true );
    table_->setColumnReadOnly( 3, true );
    table_->setColumnStretchable( 2, false );
    table_->setColumnStretchable( 4, false );
    table_->resizeHeaderToContents( true );
    CallBack selcb = mCB(this,uiSEGYMultiVintageImporter,selectFilesCB);
    uiPushButton* selbut = new uiPushButton( 0, uiStrings::sSelect(),
					     selcb, false );
    selbut->setIcon( "selectfromlist" );
    table_->setCellObject( RowCol(rowidx,2), selbut );

    uiToolButton* infobut = new uiToolButton( 0, "info", tr("Show report"),
			  mCB(this,uiSEGYMultiVintageImporter,displayReportCB));
    table_->setCellObject( RowCol(rowidx,4), infobut );
    fillRow( rowidx );
    setButtonSensitivity( true );
}


void uiSEGYMultiVintageImporter::fillRow( int rowid )
{
    SEGY::Vintage::Info* vntinfo = vntinfos_.get( rowid );
    if ( !vntinfo )
	pErrMsg("Something went wrong.segysetup file modified?; return;");

    BufferStringSet selnms( vntinfo->filenms_ );
    BufferString dispstr( selnms.getDispString(selnms.size(), true) );
    table_->setText( RowCol(rowid,0), vintagenm_.buf() );
    table_->setText( RowCol(rowid,1), dispstr );
    table_->setText( RowCol(rowid,3), "Process not started" );
}


void uiSEGYMultiVintageImporter::selectFilesCB( CallBacker* cb )
{
     mDynamicCastGet(uiPushButton*,obj,cb);
     if ( !obj )
	 return;

     const RowCol rc( table_->getCell(obj) );
     SEGY::Vintage::Info* vntinfo = vntinfos_.get( rc.row() );
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
    SEGY::Vintage::Info* vintage = vntinfos_.removeSingle( rowid );
    delete vintage;
    table_->removeRow( rowid );
    setButtonSensitivity( true );
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
    rsdlg_ = new uiSEGYReadStarter( this, su );
    rsdlg_->setCaption( tr("Edit vintage '%1'").arg(vntnm) );
    rsdlg_->usePar( *iop );

    if ( !rsdlg_->go() )
	return;
}


void uiSEGYMultiVintageImporter::updateStatus( CallBacker* )
{
    if ( !rfdlg_ )
	return;


    const BufferString& vntnm( rfdlg_->getCurrentProcessingVntnm() );
    int curvntidx = -1;
    for ( int vidx=0; vidx<vntinfos_.size(); vidx++ )
    {
	if ( vntnm.isEqual( vntinfos_[vidx]->vintagenm_ ) )
	{
	    curvntidx = vidx;
	    break;
	}
    }

    if ( curvntidx < 0 )
	return;

    uiSEGYImportResult* resultdlg = rfdlg_->getImportResult( curvntidx );
    if ( !resultdlg )
	return;

    table_->setText( RowCol(curvntidx,3), resultdlg->status_ );
}


void uiSEGYMultiVintageImporter::displayReportCB( CallBacker* )
{
    if ( !rfdlg_ )
	return;

    const RowCol rc( table_->notifiedCell() );
    if ( rc.col() != 3 )
	return;

    uiSEGYImportResult* resultdlg = rfdlg_->getImportResult( rc.row() );
    if ( !resultdlg )
	return;

    resultdlg->go();
}


bool uiSEGYMultiVintageImporter::acceptOK()
{
    const Seis::GeomType gt = imptypefld_->impType().geomType();
    FullSpec fullspec( gt, false);
    rfdlg_ = new uiSEGYReadFinisher( this, fullspec, "", rsdlg_->fileIsInTime(),
				     false, &vntinfos_ );
    rfdlg_->updateStatus.notify( mCB(this,uiSEGYMultiVintageImporter,
				      updateStatus) );

    const bool res = rfdlg_->go();
    setButtonSensitivity( !res );
    return false;
}
