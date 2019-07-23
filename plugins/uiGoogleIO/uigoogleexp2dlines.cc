/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2009
-*/


#include "uigoogleexp2dlines.h"
#include "googlexmlwriter.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uilistbox.h"
#include "uisellinest.h"
#include "uiseis2dfileman.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "draw.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "posinfo2dsurv.h"
#include "bendpointfinder.h"
#include "latlong.h"
#include "od_helpids.h"
#include "uiseparator.h"

#include <iostream>

uiGISExport2DSeis::uiGISExport2DSeis( uiSeis2DFileMan* p )
    : uiDialog(p,uiDialog::Setup(
		uiStrings::phrExport( tr("selected 2D seismics to GIS") ),
		tr("Specify how to export"),
		mODHelpKey (mGoogleExport2DSeisHelpID) ) )
    , s2dfm_(p)
    , putallfld_(0)
    , allsel_(false)
{
    getInitialSelectedLineNames();
    const int nrsel = sellnms_.size();

    if ( !allsel_ )
	putallfld_ = new uiGenInput( this, uiStrings::sExport(),
                                     BoolInpSpec(true,uiStrings::sAll(),
		    nrsel > 1 ? tr("Selected lines"):tr("Selected line")) );

    const char* choices[]
		= { "No", "At Start/End", "At Start only", "At End only", 0 };
    putlnmfld_ = new uiGenInput( this, tr("Put line names"),
				 StringListInpSpec(choices) );
    putlnmfld_->setValue( 1 );
    if ( putallfld_ )
	putlnmfld_->attach( alignedBelow, putallfld_ );

    OD::LineStyle ls( OD::LineStyle::Solid, 20, Color(0,0,255) );
    uiSelLineStyle::Setup lssu; lssu.drawstyle( false );
    lsfld_ = new uiSelLineStyle( this, ls, lssu );
    lsfld_->attach( alignedBelow, putlnmfld_ );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, lsfld_ );
    BufferString flnm = "2DLines";
    expfld_ = new uiGISExpStdFld( this, flnm );
    expfld_->attach( stretchedBelow, sep );
    expfld_->attach( leftAlignedBelow, lsfld_ );
}


uiGISExport2DSeis::~uiGISExport2DSeis()
{
}


void uiGISExport2DSeis::getFinalSelectedLineNames()
{
    if ( !allsel_ )
	allsel_ = putallfld_ ? putallfld_->getBoolValue() : false;
    if ( !allsel_ )
	return;

    const uiListBox& lb( *s2dfm_->getListBox(false) );
    sellnms_.erase();
    for ( int idx=0; idx<lb.size(); idx++ )
	sellnms_.add( lb.itemText(idx) );
}


void uiGISExport2DSeis::getInitialSelectedLineNames()
{
    const uiListBox& lb( *s2dfm_->getListBox(false) );
    sellnms_.erase();
    const int nrsel = lb.nrChosen();
    allsel_ = nrsel == 0;
    for ( int idx=0; idx<lb.size(); idx++ )
    {
	if ( allsel_ || lb.isChosen(idx) )
	    sellnms_.add( lb.itemText(idx) );
    }
    allsel_ = sellnms_.size() == lb.size();
}


bool uiGISExport2DSeis::acceptOK()
{
    getFinalSelectedLineNames();
    GISWriter* wrr = expfld_->createWriter();
    if (!wrr)
	return false;

    GISWriter::Property props;
    props.color_ = lsfld_->getColor();
    props.width_ = lsfld_->getWidth() * .1;
    props.nmkeystr_ = "Line_No";
    wrr->setProperties( props );
    ObjectSet<Pick::Set> picks;
    TypeSet<Coord> coords;
    for ( int idx=0; idx<sellnms_.size(); idx++ )
	getCoordsForLine( picks, sellnms_.get(idx) );

    wrr->writeLine( picks );

    wrr->close();
    bool ret = uiMSG().askGoOn(
			tr("Successfully created %1 for selected 2D Lines"
					" Do you want to create more?" )
					.arg( wrr->factoryDisplayName() ) );
    return !ret;
}


void uiGISExport2DSeis::getCoordsForLine( ObjectSet<Pick::Set>& picks,
							    const char* lnm )
{
    const Seis2DDataSet& dset( *s2dfm_->dataset_ );
    const int iln = dset.indexOf( lnm );
    const auto& geom2d = SurvGeom::get2D( lnm );
    if ( iln < 0 || geom2d.isEmpty() )
	return;

    PosInfo::Line2DData l2dd = geom2d.data();
    const int nrposns = l2dd.positions().size();
    if ( nrposns < 2 )
	return;

    //const int lnmchoice = putlnmfld_->getIntValue();
    /*if ( lnmchoice != 0 && lnmchoice < 3 )
	wrr.writePoint( l2dd.positions()[0].coord_, lnm );
    if ( lnmchoice == 1 || lnmchoice == 3 )
	wrr.writePoint( l2dd.positions()[nrposns - 1].coord_, lnm );
	*/
    TypeSet<Coord> crds;
    for ( int idx=0; idx<nrposns; idx++ )
	crds += l2dd.positions()[idx].coord_;

    BendPointFinder2D bpf( crds, 1 );
    bpf.execute();

    Pick::Set* pick = new Pick::Set( lnm );

    for ( int idx=0; idx<bpf.bendPoints().size(); idx++ )
    {
	const Coord coord = crds[bpf.bendPoints()[idx]];
	Pick::Location loc( coord );
	pick->add( loc );
    }
    picks.add( pick );
    //wrr.writeLine( plotcrds, lnm );
}
