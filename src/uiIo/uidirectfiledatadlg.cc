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
#include "oddirs.h"
#include "transl.h"

#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"

#define mErrLabelRet(s) \
	{ isusable_ = false; lbl = new uiLabel( this, s ); return; }

uiEditDirectFileDataDlg::uiEditDirectFileDataDlg( uiParent* p,
						  const IOObj& obj )
    : uiDialog(p,Setup(tr("%1 File Editor").arg(obj.translator()),
			toUiString(obj.name()),
			mODHelpKey(mEditSEGYFileDataDlgHelpID)))
    , ioobj_(obj)
    , isusable_(true)
{}


uiEditDirectFileDataDlg::~uiEditDirectFileDataDlg()
{
    detachAllNotifiers();
}


void uiEditDirectFileDataDlg::prepareFileNames()
{
    ioobj_.implFileNames( filenames_ );
}


void uiEditDirectFileDataDlg::createInterface()
{
    const BufferString deffnm = ioobj_.fullUserExpr( true );
    uiLabel* lbl = nullptr;
    prepareFileNames();
    const BufferString& firstfnm = *filenames_.first();
    if ( filenames_.isEmpty() || firstfnm.isEmpty() )
	mErrLabelRet(tr("No files linked to %1").arg(ioobj_.name()));

    const int nrfiles = filenames_.size();
    const FilePath fp( firstfnm );
    const FileSpec fs( fp.pathOnly() );
    const BufferString absfnm = fs.absFileName();
    ConstPtrMan<Translator> transl = ioobj_.createTranslator();
    const uiString dispnm = transl->displayName();
    uiString oldfiletxt;
    if ( nrfiles > 1 )
	oldfiletxt = tr( "Current specified location of %1 files:  %2" )
			 .arg(dispnm).arg(absfnm.buf());
    else
	oldfiletxt = tr( "Current selected %1 file:  %2" )
			 .arg(dispnm).arg(fp.fullPath().buf());

    lbl = new uiLabel( this, oldfiletxt );
    const uiFileDialog::Mode mode = nrfiles > 1 ? uiFileDialog::Directory
						: uiFileDialog::ExistingFile;
    const uiString newloctxt = nrfiles > 1
				    ? tr("Specify new location for %1 files")
					 .arg(dispnm)
				    : tr("Change/modify selected %1 file to")
					 .arg(dispnm);

    BufferString fientry;
    bool lookinsamedir = false;
    for ( const auto* filenm : filenames_ )
    {
	if ( File::exists(filenm->buf()) )
	{
	    lookinsamedir = true;
	    break;
	}
    }

    if ( nrfiles > 1 )
    {
	if ( lookinsamedir )
	    fientry = absfnm;
	else
	{
	    const FilePath fpfullpath( fp.pathOnly() );
	    const FilePath basedatadirfp( GetBaseDataDir() );
	    if ( fpfullpath.isSubDirOf(basedatadirfp)
		 || fpfullpath==basedatadirfp )
		fientry = basedatadirfp.fullPath();
	    else
		fientry = FilePath(GetPersonalDir()).fullPath();
	}
    }
    else
	fientry = fp.fullPath();

    selfld_ = new uiFileInput( this, newloctxt, fientry );
    selfld_->setSelectMode( mode );
    selfld_->setObjType( tr("Location") );
    selfld_->valueChanged.notify( mCB(this,uiEditDirectFileDataDlg,dirSelCB) );
    selfld_->attach( leftAlignedBelow, lbl );
}


void uiEditDirectFileDataDlg::doDirSel()
{}


void uiEditDirectFileDataDlg::dirSelCB( CallBacker* )
{
    doDirSel();
}
