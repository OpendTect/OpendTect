/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "uiwellsel.h"
#include "uimsg.h"
#include "oddirs.h"
#include "survinfo.h"
#include "wellman.h"
#include "latlong.h"
#include "manobjectset.h"
#include "od_ostream.h"
#include "od_helpids.h"
#include "uicolor.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "welldata.h"

uiGISExportWells::uiGISExportWells( uiParent* p, const MultiID& mid )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Wells to GIS")),
		mid.isUdf() ? tr("Specify wells to output") : uiString::empty(),
				 mODHelpKey(mGoogleExportWellsHelpID)))
    , multiid_(mid)
{
    ismultisel_ = mid.isUdf();
    uiSeparator* sep1 = nullptr;
    if ( ismultisel_ )
    {
	selfld_ = new uiMultiWellSel( this, false );
	sep1 = new uiSeparator( this );
	sep1->attach( stretchedBelow, selfld_ );
    }

    uiColorInput::Setup su( OD::Color::Blue() );
    su.lbltxt_ = uiStrings::sColor();
    colinput_ = new uiColorInput( this, su );
    if ( sep1 )
	colinput_->attach( alignedBelow, sep1 );

    uiStringSet choices;
    choices.add( uiStrings::sNo() );
    choices.add( uiStrings::sLeft() );
    choices.add( uiStrings::sRight() );
    choices.add( uiStrings::sTop() );
    choices.add( uiStrings::sBottom() );
    putnmfld_ = new uiGenInput( this, tr("Annotate position"),
				 StringListInpSpec(choices) );
    putnmfld_->setValue( 2 );
    mAttachCB(putnmfld_->valueChanged,uiGISExportWells::putSel);
    putnmfld_->attach( alignedBelow, colinput_ );

    const uiString label = ismultisel_ ?
				tr("Base Annotation") : tr("Well Annotation");
    lnmfld_ = new uiGenInput( this, label );
    lnmfld_->setWithCheck();
    lnmfld_->attach( alignedBelow, putnmfld_ );
    if ( ismultisel_ )
	lnmfld_->setToolTip(
			    tr("Base name will be prefixed to the well name") );
    else
    {
	lnmfld_->setText( Well::MGR().get(mid,
					  Well::LoadReqs(Well::Inf))->name() );
	lnmfld_->setToolTip( tr("If the field is left empty, "
				"well name will be used as annotation text") );
    }

    auto* sep2 = new uiSeparator( this );
    sep2->attach( alignedBelow, lnmfld_ );
    BufferString flnm = "Wells";
    expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep2 );
    expfld_->attach( leftAlignedBelow, lnmfld_ );
}


uiGISExportWells::~uiGISExportWells()
{
    detachAllNotifiers();
}


void uiGISExportWells::putSel( CallBacker* )
{
    lnmfld_->display( putnmfld_->getIntValue() != 0 );
}


bool uiGISExportWells::acceptOK( CallBacker* )
{
    TypeSet<MultiID> wellids;
    if ( ismultisel_ )
	selfld_->getSelected( wellids );
    else
	wellids.add( multiid_ );

    if ( wellids.isEmpty() )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sWell()) );
	return false;
    }

    PtrMan<GISWriter> wrr = expfld_->createWriter();
    if ( !wrr )
	return false;

    GISWriter::Property prop;
    prop.xpixoffs_ = 20;
    prop.color_ = colinput_->color();
    prop.iconnm_ = "wellpin";
    RefObjectSet<const Pick::Set> picks;

    const BufferString prefix = lnmfld_->text();
    for ( auto wellid : wellids )
    {
	ConstRefMan<Well::Data> data = Well::MGR().get( wellid,
						    Well::LoadReqs(Well::Inf) );
	if ( !data )
	    continue;

	const Coord coord = data->info().surfacecoord_;
	if ( coord.isUdf() )
	    continue;

	const BufferString nm = data->name();
	if ( prefix.isEmpty() )
	    prop.nmkeystr_ = nm;
	else if ( ismultisel_ )
	    prop.nmkeystr_ = BufferString( prefix, "_", nm );

	RefMan<Pick::Set> pick = new Pick::Set( nm );
	Pick::Location loc( coord );
	pick->add( loc );
	picks.add( pick );
    }

    wrr->setProperties( prop );
    const bool res = wrr->writePoint( picks );
    wrr->close();

    if ( !res )
    {
	uiMSG().error( wrr->errMsg() );
	return false;
    }

    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}
