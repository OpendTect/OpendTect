/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiprestackimpevent.h"

#include "ctxtioobj.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "prestackeventascio.h"
#include "prestackeventio.h"
#include "prestackeventtransl.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

#include <fstream>

namespace PreStack
{

uiEventImport::uiEventImport( uiParent* p )
    : uiDialog( p, uiDialog::Setup(tr("Import Prestack Events"),mNoDlgTitle,
				   mODHelpKey(mPreStackEventImportHelpID) )
				   .modal(false) )
    , fd_(*EventAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    filefld_ = new uiFileInput( this, "Input ASCII file",
				uiFileInput::Setup(0).forread(true)
						     .withexamine(true) );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 
                                      mODHelpKey(mTableImpDataSelpicksHelpID));
    dataselfld_->attach( alignedBelow, filefld_ );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread_ = false;
    outputfld_ = new uiIOObjSel( this, ctxt, tr("Prestack Events") );
    outputfld_->attach( alignedBelow, dataselfld_ );
}


bool uiEventImport::acceptOK( CallBacker* )
{
    if ( !filefld_->fileName() )
    {
	uiMSG().error(tr("No input file selected"));
	return false;
    }

    if ( !outputfld_->ioobj() )
	return false;

    RefMan<EventManager> mgr = new EventManager;
    mgr->setStorageID( outputfld_->key(), false );
    EventImporter importer( filefld_->fileName(), fd_, *mgr );
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, importer ) )
    {
	uiMSG().error( tr("Could not import PreStack Events") );
	return false;
    }

    EventWriter writer( outputfld_->getIOObj(), *mgr );
    if( !TaskRunner::execute( &taskrunner, writer ) )
    {
	uiMSG().error( tr("Could not write PreStack Events") );
	return false;
    }

    uiString msg = tr( "PreStack Event successfully imported."
		      "\nDo you want to import more PreStack Events?" );
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

}; //namespace
