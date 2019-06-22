/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
-*/


#include "uigoogleexprandline.h"
#include "googlexmlwriter.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uisellinest.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "posinfo.h"
#include "draw.h"
#include "survinfo.h"
#include "latlong.h"
#include "od_helpids.h"
#include "uiseparator.h"

#include <iostream>


uiGoogleExportRandomLine::uiGoogleExportRandomLine( uiParent* p,
		const TypeSet<Coord>& crds, const uiString& nm )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(tr("Random Line to GIS")),
				 tr("Specify how to export"),
                                 mODHelpKey(mGoogleExportRandomLineHelpID) ) )
    , crds_(crds)
{
    const char* choices[]
		= { "No", "At Start/End", "At Start only", "At End only", 0 };
    putlnmfld_ = new uiGenInput( this, tr("Annotate line"),
				 StringListInpSpec(choices) );
    putlnmfld_->setValue( 2 );
    putlnmfld_->valuechanged.notify(
				mCB(this,uiGoogleExportRandomLine,putSel) );

    lnmfld_ = new uiGenInput( this, tr("Line annotation"),
			      StringInpSpec(mFromUiStringTodo(nm)) );
    lnmfld_->attach( alignedBelow, putlnmfld_ );

    OD::LineStyle ls( OD::LineStyle::Solid, 20, Color(200,0,200) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, lnmfld_ );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, lsfld_ );
    BufferString flnm = "RandomLine";
    flnm.add( lnmfld_->text() );
    expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, lsfld_ );
}


void uiGoogleExportRandomLine::putSel( CallBacker* )
{
    lnmfld_->display( putlnmfld_->getIntValue() != 0 );
}


bool uiGoogleExportRandomLine::acceptOK()
{
    if ( crds_.size() < 1 ) return true;

    const int lnmchoice = putlnmfld_->getIntValue();
    const char* lnm = lnmfld_->text();
    if ( !lnmchoice || !*lnm )
	lnm = "Random line";

    GISWriter* wrr = expfld_->createWriter();
    if ( !wrr )
	return false;
    GISWriter::Property prop;
    prop.color_ = lsfld_->getColor();
    prop.width_ = lsfld_->getWidth() * .1;
    wrr->setProperties( prop );
    /*if ( lnmchoice != 0 && lnmchoice < 3 )
	wrr->writePoint( crds_[0], lnm );
    if ( lnmchoice == 1 || lnmchoice == 3 )
	wrr->writePoint( crds_[crds_.size()-1], lnm );*/
    wrr->writeLine( crds_, lnm );
    wrr->close();
    bool ret = uiMSG().askGoOn(
		    tr("Successfully created %1 for selected RandomLine"
	    " Do you want to create more?").arg(wrr->factoryDisplayName()) );
    return !ret;
}
