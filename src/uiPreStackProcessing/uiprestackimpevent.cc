/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          May 2011
________________________________________________________________________

-*/


#include "uiprestackimpevent.h"

#include "ioobjctxt.h"
#include "executor.h"
#include "ioobj.h"
#include "prestackeventascio.h"
#include "prestackeventio.h"
#include "prestackeventtransl.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

#include <fstream>

namespace PreStack
{

uiEventImport::uiEventImport( uiParent* p )
    : uiDialog( p, uiDialog::Setup(
		uiStrings::phrImport(uiStrings::sPreStackEvents()),
		mNoDlgTitle,mODHelpKey(mPreStackEventImportHelpID) )
		       .modal(false) )
    , fd_(*EventAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    filefld_ = new uiFileSel( this, uiStrings::sInputASCIIFile(),
				uiFileSel::Setup().withexamine(true) );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
				mODHelpKey(mTableImpDataSelpicksHelpID) );
    dataselfld_->attach( alignedBelow, filefld_ );

    IOObjContext ctxt( PSEventTranslatorGroup::ioContext() );
    ctxt.forread_ = false;
    outputfld_ = new uiIOObjSel( this, ctxt, uiStrings::sPreStackEvents() );
    outputfld_->attach( alignedBelow, dataselfld_ );
}


bool uiEventImport::acceptOK()
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
	uiMSG().error(uiStrings::phrCannotImport(uiStrings::sPreStackEvents()));
	return false;
    }

    EventWriter writer( outputfld_->getIOObj(), *mgr );
    if( !TaskRunner::execute( &taskrunner, writer ) )
    {
	uiMSG().error( uiStrings::phrCannotWrite(tr("Prestack Events")) );
	return false;
    }

    uiString msg = tr("Prestack Event successfully imported."
		      "\n\nDo you want to import more Prestack Events?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

} // namespace PreStack
