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

#include <fstream>

namespace PreStack
{

uiEventImport::uiEventImport( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Prestack event import",0,"dgb:104.0.3") )
    , fd_(*EventAscIO::getDesc())
{
    filefld_ = new uiFileInput( this, "Input ASCII file",
	    			uiFileInput::Setup(0).forread(true)
						     .withexamine(true) );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "105.0.5" );
    dataselfld_->attach( alignedBelow, filefld_ );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread = false;
    outputfld_ = new uiIOObjSel( this, ctxt, "Prestack Events" );
    outputfld_->attach( alignedBelow, dataselfld_ );
}


bool uiEventImport::acceptOK( CallBacker* )
{
    if ( !filefld_->fileName() )
    {
	uiMSG().error("No input file selected");
	return false;
    }

    if ( !outputfld_->ioobj() )
	return false;

    RefMan<EventManager> mgr = new EventManager;
    mgr->setStorageID( outputfld_->key(), false );
    EventImporter importer( filefld_->fileName(), fd_, *mgr );
    uiTaskRunner tr( this );
    if ( !tr.execute(importer) )
	return false;

    EventWriter writer( outputfld_->getIOObj(), *mgr );
    return tr.execute( writer );
}

}; //namespace
