/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestackexpevent.h"

#include "ctxtioobj.h"
#include "executor.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "prestackeventascio.h"
#include "prestackeventtransl.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


namespace PreStack
{

uiEventExport::uiEventExport( uiParent* p, const MultiID* mid )
    : uiDialog(p,Setup(uiStrings::phrExport(uiStrings::sPreStackEvents()),
		       mODHelpKey(mPreStackEventExportHelpID)))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread_ = true;
    eventsel_ = new uiIOObjSel( this, ctxt, uiStrings::sPreStackEvents()  );
    if ( mid )
	eventsel_->setInput( *mid );

    subsel_ = uiSeisSubSel::get(this, Seis::SelSetup(Seis::Vol).withoutz(true));
    subsel_->attach( alignedBelow, eventsel_ );

    outputfile_ = new uiASCIIFileInput( this, false );
    outputfile_->attach( alignedBelow, subsel_ );
}


uiEventExport::~uiEventExport()
{}


bool uiEventExport::acceptOK( CallBacker* )
{
    if ( !outputfile_->fileName() )
    {
	uiMSG().error(tr("No file selected"));
	return false;
    }

    if ( !eventsel_->ioobj() )
	return false;

    RefMan<EventManager> events = new EventManager;
    PtrMan<Executor> loader =
	events->setStorageID( eventsel_->key(), false );

    if ( loader && !loader->execute() )
    {
	uiMSG().error( tr("Cannot load prestack events") );
	return false;
    }

    od_ostream strm( outputfile_->fileName() );
    if ( !strm.isOK() )
    {
	uiString msg = tr("%1 for writing").arg(uiStrings::phrCannotOpen(
					 toUiString(outputfile_->fileName())));
	strm.addErrMsgTo( msg );
	uiMSG().error( msg );
	return false;
    }

    TrcKeySampling hrg;
    subsel_->getSampling( hrg );

    EventExporter exporter( strm, *events );
    exporter.setHRange( hrg );
    uiTaskRunner runner( this );
    if ( !TaskRunner::execute( &runner, exporter ) )
    {
	uiMSG().error(tr("Could not export prestack events"));
	return false;
    }

    return true;
}

} // namespace PreStack
