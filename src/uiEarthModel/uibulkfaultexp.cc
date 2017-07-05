/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2016
________________________________________________________________________

-*/


#include "uibulkfaultexp.h"

#include "emfault3d.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "executor.h"
#include "od_istream.h"
#include "survinfo.h"

#include "uifilesel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiBulkFaultExport::uiBulkFaultExport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(tr("Multiple Faults")),
				 mNoDlgTitle,
				 mODHelpKey(mBulkFaultExportHelpID))
				.modal(false))
    , fd_(BulkFaultAscIO::getDesc())
{
    setOkText( uiStrings::sExport() );

    outfld_ = new uiFileSel( this,
		      uiStrings::sInputASCIIFile(),
		      uiFileSel::Setup().withexamine(true)
		      .examstyle(File::Table) );
}


uiBulkFaultExport::~uiBulkFaultExport()
{
}


bool uiBulkFaultExport::acceptOK( CallBacker* )
{
    return false;
}
