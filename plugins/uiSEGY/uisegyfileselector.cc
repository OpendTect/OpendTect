/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		August 2017
________________________________________________________________________

-*/

#include "uisegyfileselector.h"

#include "dirlist.h"
#include "filepath.h"
#include "uilistbox.h"

uiSEGYFileSelector::uiSEGYFileSelector(uiParent* p, const char* fnm,
				       const char* vntname )
    : uiDialog(p, uiDialog::Setup(tr("Select SEGY file(s)"),
	       mNoDlgTitle,mNoHelpKey) )
    , filenmsfld_(0)
{
    uiString captstr;
    captstr = tr("Defined vintage: '%1' " ).arg( vntname );
    setTitleText( captstr );
    File::Path fp( fnm );
    BufferString path = fp.pathOnly();
    BufferString msk("*.", fp.extension() );
    DirList filelist( path, File::FilesInDir, msk );
    filenmsfld_ = new uiListBox( this, "Select files", OD::ChooseAtLeastOne );
    filenmsfld_->addItems( filelist );
    filenmsfld_->setCurrentItem( fp.fileName() );
    filenmsfld_->setChosen( filenmsfld_->currentItem() );
}


void uiSEGYFileSelector::getSelNames( BufferStringSet& nms )
{
    filenmsfld_->getChosen( nms );
}


bool uiSEGYFileSelector::acceptOK()
{
    if ( !filenmsfld_->nrChosen() )
	return false;

    return true;
}
