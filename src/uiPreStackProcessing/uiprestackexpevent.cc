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
#include "prestackeventascio.h"
#include "prestackeventtransl.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"

#include <fstream>

namespace PreStack
{

uiEventExport::uiEventExport( uiParent* p, const MultiID* mid )
    : uiDialog( p, uiDialog::Setup("Prestack event export",0,"dgb:104.0.2") )
{
    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread = true;
    eventsel_ = new uiIOObjSel( this, ctxt, "Prestack Events" );
    if ( mid )
	eventsel_->setInput( *mid );

    subsel_ = uiSeisSubSel::get(this, Seis::SelSetup(Seis::Vol).withoutz(true));
    subsel_->attach( alignedBelow, eventsel_ );

    outputfile_ = new uiFileInput( this, "Output file",
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

    std::ofstream strm( outputfile_->fileName() );
    if ( !strm )
    {
	BufferString msg = "Cannot open ";
	msg += outputfile_->fileName();
	msg += " for writing";
	uiMSG().error( msg.buf() );
	return false;
    }

    HorSampling hrg;
    subsel_->getSampling( hrg );

    EventExporter exporter( strm, *events );
    exporter.setHRange( hrg );
    uiTaskRunner runner( this );
    if ( !runner.execute( exporter ) )
    {
	uiMSG().error("Could not export prestack events");
	return false;
    }


    return true;
}

}; //namespace
