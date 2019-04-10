/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "uiprestackexpevent.h"

#include "ioobjctxt.h"
#include "executor.h"
#include "ioobj.h"
#include "seisselsetup.h"
#include "od_ostream.h"
#include "prestackeventascio.h"
#include "prestackeventtransl.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


namespace PreStack
{

uiEventExport::uiEventExport( uiParent* p, const DBKey* mid )
    : uiDialog( p, uiDialog::Setup(
			uiStrings::phrExport( uiStrings::sPreStackEvents() ),
			mNoDlgTitle,
			mODHelpKey(mPreStackEventExportHelpID) ).modal(false) )
{
    setOkText( uiStrings::sExport() );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread_ = true;
    eventsel_ = new uiIOObjSel( this, ctxt, uiStrings::sPreStackEvents()  );
    if ( mid )
	eventsel_->setInput( *mid );

    subsel_ = uiSeisSubSel::get(this, Seis::SelSetup(Seis::Vol).withoutz(true));
    subsel_->attach( alignedBelow, eventsel_ );

    uiFileSel::Setup fssu; fssu.setForWrite();
    outfld_ = new uiFileSel( this, uiStrings::sOutputASCIIFile(), fssu );
    outfld_->attach( alignedBelow, subsel_ );
}


bool uiEventExport::acceptOK()
{
    if ( !outfld_->fileName() )
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

    od_ostream strm( outfld_->fileName() );
    if ( !strm.isOK() )
    {
	uiString msg = uiStrings::phrCannotOpenForWrite( outfld_->fileName() );
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
	uiMSG().error(tr("Cannot export prestack events"));
	return false;
    }

    uiString msg = tr("Prestack Event successfully exported"
		      "\n\nDo you want to export more Prestack Events?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

}; //namespace
