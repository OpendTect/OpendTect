/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/


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
				   mODHelpKey(mPreStackEventImportHelpID) ) )
    , fd_(*EventAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    filefld_ = new uiASCIIFileInput( this, true );

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
	return false;

    EventWriter writer( outputfld_->getIOObj(), *mgr );
    return TaskRunner::execute( &taskrunner, writer );
}

} // namespace PreStack
