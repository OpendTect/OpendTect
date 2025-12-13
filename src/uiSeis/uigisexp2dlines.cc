/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigisexp2dlines.h"

#include "giswriter.h"

#include "bendpointfinder.h"
#include "od_helpids.h"
#include "seis2ddata.h"
#include "survgeom2d.h"
#include "survinfo.h"

#include "uigeninput.h"
#include "uifileinput.h"
#include "uigisexp.h"
#include "uilistbox.h"
#include "uisellinest.h"
#include "uiseislinesel.h"
#include "uimsg.h"
#include "uiseparator.h"


uiGISExport2DLines::uiGISExport2DLines( uiParent* p,
					const TypeSet<Pos::GeomID>* geomids )
    : uiDialog(p,Setup(uiStrings::phrExport(tr("2D Line geometries to GIS")),
		       mODHelpKey(mGoogleExport2DSeisHelpID)))
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    linesselfld_ = new uiSeis2DLineChoose( this, OD::ChooseAtLeastOne );
    if ( geomids && !geomids->isEmpty() )
	linesselfld_->setChosen( *geomids );

    const OD::LineStyle ls( OD::LineStyle::Solid,
			    uiGISExpStdFld::sDefLineWidth(),
			    uiGISExpStdFld::sDefColor() );
    uiSelLineStyle::Setup lssu;
    lssu.drawstyle( false );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, linesselfld_ );

    auto* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, lsfld_ );

    expfld_ = new uiGISExpStdFld( this, "2DLines" );
    expfld_->attach( ensureBelow, sep );
    expfld_->attach( alignedBelow, lsfld_ );
}


uiGISExport2DLines::~uiGISExport2DLines()
{
}


void uiGISExport2DLines::setSelected( const Pos::GeomID& gid )
{
    const TypeSet<Pos::GeomID> geomids( 1, gid );
    setSelected( geomids );
}


void uiGISExport2DLines::setSelected( const TypeSet<Pos::GeomID>& geomids )
{
    if ( !geomids.isEmpty() )
	linesselfld_->setChosen( geomids );
}


void uiGISExport2DLines::getCoordsForLine( const Pos::GeomID& gid,
					   BufferString& linenm,
					   TypeSet<Coord>& coords )
{
    if ( gid.isUdf() )
	return;

    ConstRefMan<Survey::Geometry> geom = Survey::GM().getGeometry( gid );
    if ( !geom || !geom->is2D() )
	return;

    const TypeSet<PosInfo::Line2DPos>& linepos =
					geom->as2D()->data().positions();
    if ( linepos.size() < 2 )
	return;

    linenm = geom->getName();
    TypeSet<Coord> crds;
    for ( const auto& pos : linepos )
	crds += pos.coord_;

    BendPointFinder2D bpf( crds, 1 );
    bpf.execute();

    for ( int idx=0; idx<bpf.bendPoints().size(); idx++ )
	coords += crds[bpf.bendPoints()[idx]];
}


bool uiGISExport2DLines::acceptOK( CallBacker* )
{
    TypeSet<Pos::GeomID> geomids;
    linesselfld_->getChosen( geomids );
    if ( geomids.isEmpty() )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sLine()) );
	return false;
    }

    PtrMan<GIS::Writer> wrr = expfld_->createWriter( SI().name().buf(),
						     "Line geometries" );
    if ( !wrr )
	return false;

    GIS::Property props;
    wrr->getDefaultProperties( GIS::FeatureType::LineString, props );
    props.linestyle_.color_ = lsfld_->getColor();
    props.linestyle_.width_ = lsfld_->getWidth() * .1;
    props.nmkeystr_ = "Line_No";
    wrr->setProperties( props );
    bool haserrors = false;
    for ( const auto& gid : geomids )
    {
	BufferString linenm;
	TypeSet<Coord> coords;
	getCoordsForLine( gid, linenm, coords );
	const bool res = wrr->writeLine( coords, linenm.buf() );
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
