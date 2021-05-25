/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2021
________________________________________________________________________

-*/


#include "uilaswriter.h"

#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiwellsel.h"

#include "filepath.h"
#include "ioobj.h"
#include "laswriter.h"
#include "oddirs.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellman.h"
#include "wellreader.h"


uiLASWriter::uiLASWriter( uiParent* p )
    : uiDialog(p,Setup(tr("Export to LAS"),mNoDlgTitle,mTODOHelpKey))
{
    setOkText( uiStrings::sExport() );

    wellfld_ = new uiWellSel( this, true );
    mAttachCB( wellfld_->selectionDone, uiLASWriter::wellSelCB );

    uiListBox::Setup lbsu( OD::ChooseAtLeastOne, tr("Select Log(s)") );
    logsfld_ = new uiListBox( this, lbsu );
    logsfld_->setStretch( 1, 1 );
    logsfld_->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedBelow, wellfld_ );

    lasfld_ = new uiASCIIFileInput( this, false );
    lasfld_->setFilter( Well::LASImporter::fileFilter() );
    lasfld_->setDefaultExtension( "las" );
    lasfld_->attach( alignedBelow, logsfld_ );

    mAttachCB( postFinalise(), uiLASWriter::wellSelCB );
}


uiLASWriter::~uiLASWriter()
{
    detachAllNotifiers();
}


void uiLASWriter::wellSelCB( CallBacker* )
{
    logsfld_->setEmpty();

    const IOObj* ioobj = wellfld_->ioobj( true );
    if ( !ioobj )
	return;

    const MultiID wellid = ioobj->key();
    if ( wellid.isUdf() )
	return;

    BufferStringSet lognms;
    Well::MGR().getLogNamesByID( wellid, lognms );
    logsfld_->addItems( lognms );

    const FilePath fp = ioobj->fullUserExpr();
    BufferString fnm( fp.baseName(), "_logs" );
    FilePath laspath( GetSurveyExportDir(), fnm );
    laspath.setExtension( "las" );
    lasfld_->setFileName( laspath.fullPath() );
}


bool uiLASWriter::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wellfld_->ioobj();
    if ( !ioobj )
	return false;

    const BufferString lasfnm = lasfld_->fileName();
    Well::LoadReqs reqs( Well::Trck );
    RefMan<Well::Data> wd = Well::MGR().get( ioobj->key(), reqs );
    if ( !wd )
    {
	uiMSG().error( toUiString(Well::MGR().errMsg()) );
	return false;
    }

    Well::Reader rdr( ioobj->key(), *wd );
    if ( !rdr.isUsable() )
    {
	uiMSG().error( rdr.errMsg() );
	return false;
    }

    BufferStringSet lognms;
    logsfld_->getChosen( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const char* lognm = lognms.get( idx ).buf();
	rdr.getLog( lognm );
    }

    LASWriter laswriter( *wd, lognms, lasfnm );
    laswriter.execute();
    return false;
}
