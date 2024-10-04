/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidirectfiledatadlg.h"

#include "filepath.h"
#include "filespec.h"
#include "ioobj.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"

#define mErrLabelRet(s) \
	{ isusable_ = false; lbl = new uiLabel( this, s ); return; }

uiEditDirectFileDataDlg::uiEditDirectFileDataDlg( uiParent* p,
						  const IOObj& obj )
    : uiDialog(p,Setup(tr("SEGYDirect File Editor"),
			toUiString(obj.name()),
			mODHelpKey(mEditSEGYFileDataDlgHelpID)))
    , ioobj_(obj)
    , isusable_(true)
{
    createInterface();
}


uiEditDirectFileDataDlg::~uiEditDirectFileDataDlg()
{
    detachAllNotifiers();
}


void uiEditDirectFileDataDlg::createInterface()
{
    const BufferString deffnm = ioobj_.fullUserExpr( true );
    uiLabel* lbl = nullptr;
    ioobj_.implFileNames( filenames_ );
    const BufferString& firstfnm = *filenames_.first();
    if ( filenames_.isEmpty() || firstfnm.isEmpty() )
	mErrLabelRet(tr("No files linked to %1").arg(ioobj_.name()));

    const FilePath fp( firstfnm );
    const FileSpec fs( fp.pathOnly() );
    const BufferString absfnm = fs.absFileName();
    uiString olddirtxt( tr("Old location of SEGY files:  %1")
			.arg(absfnm.buf()) );
    lbl = new uiLabel( this, olddirtxt );

    const int nrfiles = filenames_.size();
    const uiFileDialog::Mode mode = nrfiles > 1 ? uiFileDialog::Directory
						: uiFileDialog::ExistingFile;
    selfld_ = new uiFileInput( this, tr("New location"), absfnm.buf() );
    selfld_->setSelectMode( mode );
    selfld_->setObjType( tr("Location") );
    selfld_->valueChanged.notify( mCB(this,uiEditDirectFileDataDlg,dirSelCB) );
    selfld_->attach( leftAlignedBelow, lbl );
}


void uiEditDirectFileDataDlg::dirSelCB( CallBacker* )
{
    doDirSel();
}
