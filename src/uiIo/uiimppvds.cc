/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiimppvds.cc,v 1.3 2010-06-25 13:43:34 cvsbert Exp $";

#include "uiimppvds.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "posvecdatasettr.h"
#include "ioobj.h"
#include "strmprov.h"
#include "datapointset.h"
#include "tabledef.h"
#include "tableascio.h"


uiImpPVDS::uiImpPVDS( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import cross-plot data",
				 "Import column data for cross-plots",
				 mTODOHelpID))
    , fd_(*new Table::FormatDesc("Cross-plot data"))
{
    uiFileInput::Setup su( uiFileDialog::Txt );
    su.withexamine(true).examstyle(uiFileInput::Setup::Table).forread(true);
    inpfld_ = new uiFileInput( this, "Input file", su );

    fd_.bodyinfos_ += Table::TargetInfo::mkHorPosition( false );
    fd_.bodyinfos_ += Table::TargetInfo::mkZPosition( false );
    dataselfld_ = new uiTableImpDataSel( this, fd_, mTODOHelpID );
    dataselfld_->attach( alignedBelow, inpfld_ );

    row1isdatafld_ = new uiGenInput( this, "First row contains",
	    			BoolInpSpec(false,"Data","Column names") );
    row1isdatafld_->attach( alignedBelow, dataselfld_ );

    IOObjContext ctxt( mIOObjContext(PosVecDataSet) );
    ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctxt, "Output data set" );
    outfld_->attach( alignedBelow, row1isdatafld_ );
}


uiImpPVDS::~uiImpPVDS()
{
    delete &fd_;
}


bool uiImpPVDS::acceptOK( CallBacker* )
{
    return true;
}
