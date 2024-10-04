/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseisdirectfiledatadlg.h"

#include "filepath.h"
#include "filespec.h"
#include "ioobj.h"
#include "iopar.h"
#include "seistrctr.h"

#include "uibutton.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitable.h"

static const int cOldNameCol	= 0;
static const int cNewNameCol	= 1;
static const int cButtonCol	= 2;

uiSeisDirectFileDataDlg::uiSeisDirectFileDataDlg( uiParent* p,
						  const IOObj& obj )
    : uiEditDirectFileDataDlg( p, obj )
{
    if ( filenames_.size() < 2 )
	return;

    fillFileTable();
    filetable_->attach( stretchedBelow, selfld_ );
    if ( !isusable_ )
    {
	const BufferString deffnm = ioobj_.fullUserExpr( true );
	auto* lbl = new uiLabel( this,
			tr("Invalid SEG-Y Definition file:\n%1").arg(deffnm) );
	lbl->attach( alignedBelow, selfld_ );
    }
}


uiSeisDirectFileDataDlg::~uiSeisDirectFileDataDlg()
{
    detachAllNotifiers();
}


void uiSeisDirectFileDataDlg::fillFileTable()
{
    uiTable::Setup su( filenames_.size(), 3 );
    filetable_ = new uiTable( this, su, "FileTable" );
    filetable_->setColumnLabel( cOldNameCol, tr("Old File Name") );
    filetable_->setColumnLabel( cNewNameCol, tr("New File Name") );
    filetable_->setColumnLabel( cButtonCol, toUiString(" ") );
    filetable_->setColumnReadOnly( cOldNameCol, true );
    filetable_->setColumnStretchable( cButtonCol, false );
    filetable_->resizeColumnToContents( cButtonCol );
    filetable_->setPrefWidth( 700 );


    const BufferString fnm0 = *filenames_.first();
    if ( fnm0.isEmpty() )
    {
	isusable_ = false;
	return;
    }

    FilePath oldfp( fnm0 );
    const BufferString olddir = oldfp.pathOnly();
    const int nrrows = filenames_.size();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const BufferString& fnm = filenames_.get( idx );
	const FilePath fp( fnm );
	const BufferString oldfilename = fp.fileName();
	filetable_->setText( RowCol(idx,cOldNameCol),
			fp.pathOnly() == olddir ? oldfilename : fp.fullPath() );
	uiButton* selbut = uiButton::getStd( 0, OD::Select,
			       mCB(this,uiSeisDirectFileDataDlg,fileSelCB),
			       true );
	filetable_->setCellObject( RowCol(idx,cButtonCol), selbut );
    }

    updateFileTable( -1 );
    mAttachCB( filetable_->valueChanged, uiSeisDirectFileDataDlg::editCB );
}


void uiSeisDirectFileDataDlg::updateFileTable( int rowidx )
{
    if ( !isusable_ || !filetable_ )
	return;

    NotifyStopper ns( filetable_->valueChanged );
    const BufferString seldir = selfld_->fileName();
    for ( int idx=0; idx<filenames_.size(); idx++ )
    {
	if ( rowidx >= 0 && rowidx != idx )
	    continue;

	const FilePath oldfp( filenames_.get(idx) );
	const BufferString oldfnm = oldfp.fileName();
	RowCol rc( idx, cNewNameCol );
	BufferString newfnm = filetable_->text( rc );
	if ( newfnm.isEmpty() )
	{
	    newfnm = oldfnm;
	    filetable_->setText( RowCol(idx,cNewNameCol), newfnm );
	}

	const FilePath fp( seldir.buf(), newfnm.buf() );
	if ( File::exists(fp.fullPath()) )
	{
	    filetable_->setCellToolTip( rc, uiStrings::sEmptyString() );
	    filetable_->setColor( rc, OD::Color::White() );
	}
	else
	{
	    uiString tttext( uiStrings::phrCannotFind(uiStrings::phrJoinStrings
			      (uiStrings::sFile(),toUiString(fp.fullPath()))) );
	    filetable_->setCellToolTip( rc, tttext );
	    filetable_->setColor( rc, OD::Color::Red() );
	}
    }
}


void uiSeisDirectFileDataDlg::editCB( CallBacker* )
{
    if ( !isusable_ || filetable_->currentCol() == cOldNameCol )
	return;

    updateFileTable( filetable_->currentRow() );
}


void uiSeisDirectFileDataDlg::fileSelCB( CallBacker* cb )
{
    mDynamicCastGet(uiObject*,uiobj,cb)
    if ( !uiobj || !isusable_ )
	return;

    const int rowidx = filetable_->getCell( uiobj ).row();
    if ( rowidx < 0 )
	return;

    BufferString newfnm = filetable_->text( RowCol(rowidx,cNewNameCol) );
    FilePath fp( selfld_->fileName(), newfnm );
    const bool selexists = File::exists( fp.fullPath() );
    // -->To-do  ==> fileFilter
    uiFileDialog dlg( this, true, selexists ? fp.fullPath().buf() : nullptr,
		      nullptr, tr("SEG-Y") );
    if ( !selexists )
	dlg.setDirectory( fp.pathOnly() );

    if ( !dlg.go() )
	return;

    FilePath newfp( dlg.fileName() );
    if ( newfp.pathOnly() != fp.pathOnly() )
    {
	uiMSG().error( uiStrings::phrSelect(tr("a file from %1")
							.arg(fp.pathOnly())) );
	return;
    }

    filetable_->setText( RowCol(rowidx,cNewNameCol), newfp.fileName() );
    updateFileTable( rowidx );
}


void uiSeisDirectFileDataDlg::doDirSel()
{
    updateFileTable( -1 );
}


#define mErrRet(s) { uiMSG().error( s ); return false; }
bool uiSeisDirectFileDataDlg::acceptOK( CallBacker* )
{
    if ( !isusable_ )
	return true;

    mDynamicCast(SeisTrcTranslator*,PtrMan<SeisTrcTranslator> trl,
		 ioobj_.createTranslator())
    if ( !trl )
	mErrRet( tr("Could not write new File location(s).") )

    uiString errmsg;
    bool allsuccess = true;
    if ( !filetable_ )
    {
	const BufferString newfnm = selfld_->fileName();
	if ( newfnm.isEmpty() )
	    mErrRet( tr("New file name cannot be empty") );

	FilePath fp( newfnm );
	if ( !File::exists( fp.fullPath() ) )
	    mErrRet( tr("File %1 does not exist").arg(fp.fullPath()) )

	allsuccess = trl->implRelocate( &ioobj_, fp.fullPath() );
	if ( !allsuccess )
	{
	    errmsg = tr( "Relocating failed: " );
	    errmsg.append( trl->errMsg() );
	}
    }
    else
    {
	const BufferString seldir = selfld_->fileName();
	for ( int idx=0; idx<filetable_->nrRows(); idx++ )
	{
	    const BufferString oldfnm
				= filetable_->text( RowCol(idx,cOldNameCol) );
	    const BufferString newfnm
				= filetable_->text( RowCol(idx,cNewNameCol) );
	    if ( newfnm.isEmpty() )
		mErrRet( tr("New file name cannot be empty") );

	    if ( oldfnm == newfnm )
		continue;

	    const FilePath oldfp( oldfnm );
	    const FilePath newfp( seldir, newfnm );
	    if ( !File::exists(newfp.fullPath()) )
		mErrRet( tr("File %1 does not exist").arg(newfp.fullPath()) )

	    const bool success = trl->implRelocate( &ioobj_,
						    newfp.fullPath().str(),
						    oldfp.fileName().str() );
	    if ( !success )
	    {
		errmsg = tr( "Relocating failed for %1" ).arg( oldfnm );
		errmsg.addNewLine().append(trl->errMsg()).addNewLine();
		allsuccess = false;
	    }
	}

	if ( !allsuccess )
	    uiMSG().error( errmsg );
    }

    return true;
}
