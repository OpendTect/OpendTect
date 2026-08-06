/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackimpevent.h"

#include "ctxtioobj.h"
#include "filepath.h"
#include "ioobj.h"
#include "prestackeventascio.h"
#include "prestackeventio.h"
#include "prestackeventtransl.h"
#include "tabledef.h"

#include "uifileinput.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"


namespace PreStack
{

uiEventImport::uiEventImport( uiParent* p )
    : uiDialog(p,Setup(uiStrings::phrImport(uiStrings::sPreStackEvents()),
		       mODHelpKey(mPreStackEventImportHelpID)))
    , fd_(*EventAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    filefld_ = new uiASCIIFileInput( this, true );
    mAttachCB( filefld_->valueChanged, uiEventImport::inputChgd );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
				      mODHelpKey(mTableImpDataSelpicksHelpID));
    dataselfld_->attach( alignedBelow, filefld_ );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread_ = false;
    outputfld_ = new uiIOObjSel( this, ctxt, uiStrings::sPreStackEvents() );
    outputfld_->attach( alignedBelow, dataselfld_ );
}


uiEventImport::~uiEventImport()
{
    delete &fd_;
}


void uiEventImport::inputChgd( CallBacker* )
{
    const FilePath fp( filefld_->fileName() );
    outputfld_->setInputText( fp.baseName() );
}


bool uiEventImport::acceptOK( CallBacker* )
{
    if ( !filefld_->fileName() )
    {
	uiMSG().error( tr("No input file selected") );
	return false;
    }

    outputfld_->reset();
    const IOObj* ioobj = outputfld_->ioobj();
    if ( !ioobj )
	return false;

    RefMan<EventManager> mgr = new EventManager;
    mgr->setStorageID( outputfld_->key(), false );
    EventImporter importer( filefld_->fileName(), fd_, *mgr );
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute(&taskrunner,importer) )
	return false;

    EventWriter writer( *ioobj, *mgr );
    if ( !TaskRunner::execute(&taskrunner,writer) )
	return false;

    const uiString msg = tr("Prestack events successfully imported."
				"\n\nDo you want to import more data?");
    const bool ret= uiMSG().askGoOn( msg, uiStrings::sYes(),
				uiStrings::sNoCloseWindow() );
    return !ret;
}

} // namespace PreStack
