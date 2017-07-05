/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2016
________________________________________________________________________

-*/


#include "uibulkhorizonexp.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiBulkHorizonExport::uiBulkHorizonExport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(tr("Multiple Horizons")),
				 mNoDlgTitle,
				 mODHelpKey(mBulkHorizonExportHelpID) )
			    .modal(false))
{
    setOkText( uiStrings::sExport() );

    outfld_ = new uiFileSel( this,
		      uiStrings::sOutputASCIIFile(),
		      uiFileSel::Setup().withexamine(false) );
}


uiBulkHorizonExport::~uiBulkHorizonExport()
{
}


bool uiBulkHorizonExport::acceptOK( CallBacker* )
{
    return false;
}
