/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/

static const char* rcsID = "$Id: uiprestackexpevent.cc,v 1.2 2009-07-22 16:01:41 cvsbert Exp $";

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
    , ctxt_( new CtxtIOObj( PSEventTranslatorGroup::ioContext() ) )
{
    ctxt_->ctxt.forread = true;
    if ( mid )
	ctxt_->setObj( IOM().get( *mid ) );
    eventsel_ = new uiIOObjSel( this, *ctxt_, "Prestack Events" );

    subsel_ = uiSeisSubSel::get(this, Seis::SelSetup(Seis::Vol).withoutz(true));
    subsel_->attach( alignedBelow, eventsel_ );

    outputfile_ = new uiFileInput( this, 0, uiFileInput::Setup(0).forread(false) );
    outputfile_->attach( alignedBelow, subsel_ );
}


uiEventExport::~uiEventExport()
{
    delete ctxt_->ioobj;
    delete ctxt_;
}


bool uiEventExport::acceptOK( CallBacker* )
{
    if ( !outputfile_->fileName() )
    {
	uiMSG().error("No file selected");
	return false;
    }

    if ( !eventsel_->ctxtIOObj().ioobj )
    {
	uiMSG().error( "No prestack events selected" );
	return false;
    }

    RefMan<EventManager> events = new EventManager;
    PtrMan<Executor> loader =
	events->setStorageID( eventsel_->ctxtIOObj().ioobj->key(), false );
    
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
