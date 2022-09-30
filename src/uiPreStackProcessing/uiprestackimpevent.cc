/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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


uiEventImport::~uiEventImport()
{}


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
