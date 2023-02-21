/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexprandline.h"
#include "googlexmlwriter.h"
#include "uigeninput.h"
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
#include "uiseparator.h"

#include <iostream>


uiGISExportRandomLine::uiGISExportRandomLine( uiParent* p,
		const TypeSet<Coord>* crds, const char* nm )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport(tr("Random Line to GIS")),
				 tr("Specify how to export"),
				 mODHelpKey(mGoogleExportRandomLineHelpID)) )
    , crds_(crds)
{
    uiStringSet choices;
    choices.add( uiStrings::sNo() );
    choices.add( tr("At Start/End") );
    choices.add( tr("At Start only") );
    choices.add( tr("At End only") );
    putlnmfld_ = new uiGenInput( this, tr("Annotate line"),
				 StringListInpSpec(choices) );
    putlnmfld_->setValue( 2 );
    mAttachCB(putlnmfld_->valueChanged,uiGISExportRandomLine::putSel);

    lnmfld_ = new uiGenInput( this, tr("Line annotation"),
			      StringInpSpec(nm) );
    lnmfld_->attach( alignedBelow, putlnmfld_ );

    OD::LineStyle ls( OD::LineStyle::Solid, 20, OD::Color(200,0,200) );
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


uiGISExportRandomLine::~uiGISExportRandomLine()
{
    detachAllNotifiers();
    if ( crds_ )
	crds_->empty();
}

void uiGISExportRandomLine::putSel( CallBacker* )
{
    lnmfld_->display( putlnmfld_->getIntValue() != 0 );
}


bool uiGISExportRandomLine::acceptOK( CallBacker* )
{
    if ( crds_->size() < 1 )
	return true;

    const int lnmchoice = putlnmfld_->getIntValue();
    const char* lnm = lnmfld_->text();
    if ( !lnmchoice || !*lnm )
	lnm = sKey::RandomLine();

    PtrMan<GISWriter> wrr = expfld_->createWriter();
    if ( !wrr )
	return false;

    GISWriter::Property prop;
    prop.color_ = lsfld_->getColor();
    prop.width_ = lsfld_->getWidth() * .1;
    wrr->setProperties( prop );

    mDynamicCastGet(ODGoogle::KMLWriter*,kmlwriter,wrr.ptr())
    if ( kmlwriter )
    {
	if ( lnmchoice != 0 && lnmchoice < 3 )
	    wrr->writePoint( crds_->get(0), lnm );

	if ( lnmchoice == 1 || lnmchoice == 3 )
	    wrr->writePoint( crds_->get(crds_->size()-1), lnm );
    }

    if ( !wrr->writeLine(*crds_,lnm) )
    {
	uiMSG().error( wrr->errMsg() );
	return false;
    }

    wrr->close();
    bool ret = uiMSG().askGoOn(wrr->successMsg() );
    return !ret;
}
