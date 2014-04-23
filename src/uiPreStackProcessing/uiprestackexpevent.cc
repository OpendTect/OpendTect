/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID mUsedVar = "$Id$";

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
    : uiDialog( p, uiDialog::Setup("Export Prestack Events",mNoDlgTitle,
				   mODHelpKey(mImportHorAttribHelpID) ) )
{
    setOkText( uiStrings::sExport() );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread = true;
    eventsel_ = new uiIOObjSel( this, ctxt, "Prestack Events" );
    if ( mid )
	eventsel_->setInput( *mid );

    subsel_ = uiSeisSubSel::get(this, Seis::SelSetup(Seis::Vol).withoutz(true));
    subsel_->attach( alignedBelow, eventsel_ );

    outputfile_ = new uiFileInput( this, "Output ASCII file",
				   uiFileInput::Setup(0).forread(false) );
    outputfile_->attach( alignedBelow, subsel_ );
}


bool uiEventExport::acceptOK( CallBacker* )
{
    if ( !outputfile_->fileName() )
    {
	uiMSG().error("No file selected");
	return false;
    }

    if ( !eventsel_->ioobj() )
	return false;

    RefMan<EventManager> events = new EventManager;
    PtrMan<Executor> loader =
	events->setStorageID( eventsel_->key(), false );

    if ( loader && !loader->execute() )
    {
	uiMSG().error( "Cannot load prestack events" );
	return false;
    }

    od_ostream strm( outputfile_->fileName() );
    if ( !strm.isOK() )
    {
	BufferString msg = "Cannot open ";
	msg.add( outputfile_->fileName() ).add( " for writing" );
	strm.addErrMsgTo( msg );
	uiMSG().error( msg.buf() );
	return false;
    }

    HorSampling hrg;
    subsel_->getSampling( hrg );

    EventExporter exporter( strm, *events );
    exporter.setHRange( hrg );
    uiTaskRunner runner( this );
    if ( !TaskRunner::execute( &runner, exporter ) )
    {
	uiMSG().error("Could not export prestack events");
	return false;
    }

    return true;
}

}; //namespace
