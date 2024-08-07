/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigoogleexpwells.h"
#include "googlexmlwriter.h"
#include "ioman.h"
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
{
    if ( !mid.isUdf() )
    {
	wellids_.add( mid );
	setTitleText( tr("Well: %1").arg(IOM().nameOf(mid)) );
    }

    createFields();
}


uiGISExportWells::uiGISExportWells( uiParent* p, const TypeSet<MultiID>& mids )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrExport( tr("Wells to GIS")),
				 uiString::empty(),
				 mODHelpKey(mGoogleExportWellsHelpID)))
    , wellids_(mids)
{
    createFields();
}


void uiGISExportWells::createFields()
{
    if ( wellids_.isEmpty() )
	selfld_ = new uiMultiWellSel( this, false );

    uiColorInput::Setup su( OD::Color::Blue() );
    su.lbltxt_ = uiStrings::sColor();
    colinput_ = new uiColorInput( this, su );
    if ( selfld_ )
	colinput_->attach( alignedBelow, selfld_ );

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

    const bool ismultisel = wellids_.size() > 1 || selfld_;
    const uiString label = ismultisel ?
				tr("Base Annotation") : tr("Well Annotation");
    lnmfld_ = new uiGenInput( this, label );
    lnmfld_->setWithCheck();
    lnmfld_->attach( alignedBelow, putnmfld_ );
    if ( ismultisel )
	lnmfld_->setToolTip(
			    tr("Base name will be prefixed to the well name") );
    else
    {
	ConstRefMan<Well::Data> wd = Well::MGR().get(
			wellids_.first(), Well::LoadReqs(Well::Inf));
	if ( wd )
	    lnmfld_->setText( wd->name() );
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
    if ( wellids_.isEmpty() && selfld_ )
	selfld_->getSelected( wellids_ );

    if ( wellids_.isEmpty() )
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
    for ( auto wellid : wellids_ )
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
	else if ( wellids_.size() > 1 )
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
