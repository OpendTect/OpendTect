/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigisexpwells.h"

#include "giswriter.h"
#include "ioman.h"
#include "od_helpids.h"
#include "pickset.h"
#include "welldata.h"
#include "wellman.h"

#include "uicolor.h"
#include "uigeninput.h"
#include "uigisexp.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiwellsel.h"


uiGISExportWells::uiGISExportWells( uiParent* p, const TypeSet<MultiID>* mids )
    : uiDialog(p,Setup(uiStrings::phrExport(tr("Wells to GIS")),
		       mODHelpKey(mGoogleExportWellsHelpID)))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    selfld_ = new uiMultiWellSel( this, false );
    if ( mids )
	selfld_->setSelected( *mids );

    uiColorInput::Setup su( uiGISExpStdFld::sDefColor() );
    su.lbltxt_ = uiStrings::sColor();
    colinput_ = new uiColorInput( this, su );
    colinput_->attach( alignedBelow, selfld_ );

    const uiString label = tr("Base Annotation");
    prefixfld_ = new uiGenInput( this, label );
    prefixfld_->setWithCheck();
    prefixfld_->attach( alignedBelow, colinput_ );
    prefixfld_->setToolTip( tr("Base name will be prefixed to the well name") );

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, prefixfld_ );

    expfld_ = new uiGISExpStdFld( this, "Wells" );
    expfld_->attach( ensureBelow, sep );
    expfld_->attach( alignedBelow, prefixfld_ );
}


uiGISExportWells::~uiGISExportWells()
{
    detachAllNotifiers();
}


void uiGISExportWells::setSelected( const MultiID& wellid )
{
    const TypeSet<MultiID> wellids( 1, wellid );
    setSelected( wellids );
}


void uiGISExportWells::setSelected( const TypeSet<MultiID>& wellids )
{
    selfld_->setSelected( wellids );
}


bool uiGISExportWells::acceptOK( CallBacker* )
{
    if ( selfld_->nrSelected() < 1 )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sWell()) );
	return false;
    }

    PtrMan<GIS::Writer> wrr = expfld_->createWriter( SI().name().buf(),
						     "Wells" );
    if ( !wrr )
	return false;

    GIS::Property prop;
    wrr->getDefaultProperties( GIS::FeatureType::Point, prop );
    prop.color_ = colinput_->color();
    prop.iconnm_ = "wellpin";

    const BufferString prefix = prefixfld_->text();
    TypeSet<MultiID> wellids;
    selfld_->getSelected( wellids );
    bool haserrors = false;
    for ( const auto& wellid : wellids )
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
	else
	    prop.nmkeystr_ = BufferString( prefix, "_", nm );

	wrr->setProperties( prop );
	const bool res = wrr->writePoint( coord, nm );
	if ( !res )
	    haserrors = true;
    }

    if ( haserrors )
    {
	uiMSG().error( wrr->errMsg() );
	return false;
    }

    const bool ret = uiMSG().askGoOn( wrr->successMsg() );
    return !ret;
}
