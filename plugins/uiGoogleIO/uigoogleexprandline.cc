/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2009
-*/


#include "uigoogleexprandline.h"
#include "googlexmlwriter.h"
#include "uifileinput.h"
#include "uisellinest.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "posinfo.h"
#include "strmprov.h"
#include "draw.h"
#include "survinfo.h"
#include "latlong.h"
#include "od_helpids.h"

#include <iostream>


uiGoogleExportRandomLine::uiGoogleExportRandomLine( uiParent* p,
		const TypeSet<Coord>& crds, const char* nm )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(tr("Random Line to KML")),
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
			      StringInpSpec(nm) );
    lnmfld_->attach( alignedBelow, putlnmfld_ );

    OD::LineStyle ls( OD::LineStyle::Solid, 20, Color(200,0,200) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, lnmfld_ );

    mImplFileNameFld(nm);
    fnmfld_->attach( alignedBelow, lsfld_ );
}


void uiGoogleExportRandomLine::putSel( CallBacker* )
{
    lnmfld_->display( putlnmfld_->getIntValue() != 0 );
}


bool uiGoogleExportRandomLine::acceptOK( CallBacker* )
{
    if ( crds_.size() < 1 ) return true;

    const int lnmchoice = putlnmfld_->getIntValue();
    const char* lnm = lnmfld_->text();
    if ( !lnmchoice || !*lnm ) lnm = "Random line";
    mCreateWriter( lnm, SI().name() );

    BufferString ins( "\t\t<LineStyle>\n\t\t\t<color>" );
    ins += lsfld_->getColor().getStdStr(false,-1);
    ins += "</color>\n\t\t\t<width>";
    ins += lsfld_->getWidth() * .1;
    ins += "</width>\n\t\t</LineStyle>";
    wrr.writeIconStyles( 0, 0, ins );


    if ( lnmchoice != 0 && lnmchoice < 3 )
	wrr.writePlaceMark( 0, crds_[0], lnm );
    if ( lnmchoice == 1 || lnmchoice == 3 )
	wrr.writePlaceMark( 0, crds_[crds_.size()-1], lnm );
    wrr.writeLine( 0, crds_, lnm );

    wrr.close();
    return true;
}
