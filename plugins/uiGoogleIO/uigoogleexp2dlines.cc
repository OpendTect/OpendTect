/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2009
-*/


#include "uigoogleexp2dlines.h"
#include "googlexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uisellinest.h"
#include "uiseis2dfileman.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "strmprov.h"
#include "draw.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "posinfo2dsurv.h"
#include "bendpointfinder.h"
#include "latlong.h"
#include "od_helpids.h"

#include <iostream>

uiGoogleExport2DSeis::uiGoogleExport2DSeis( uiSeis2DFileMan* p )
    : uiDialog(p,uiDialog::Setup(
		uiStrings::phrExport( tr("selected 2D seismics to KML") ),
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

    mImplFileNameFld(s2dfm_->name());
    fnmfld_->attach( alignedBelow, lsfld_ );
}


uiGoogleExport2DSeis::~uiGoogleExport2DSeis()
{
}


void uiGoogleExport2DSeis::getFinalSelectedLineNames()
{
    if ( !allsel_ )
	allsel_ = putallfld_ ? putallfld_->getBoolValue() : false;
    if ( !allsel_ )
	return;

    const uiListBox& lb( *s2dfm_->getListBox(false) );
    sellnms_.erase();
    for ( int idx=0; idx<lb.size(); idx++ )
	sellnms_.add( lb.textOfItem(idx) );
}


void uiGoogleExport2DSeis::getInitialSelectedLineNames()
{
    const uiListBox& lb( *s2dfm_->getListBox(false) );
    sellnms_.erase();
    const int nrsel = lb.nrChosen();
    allsel_ = nrsel == 0;
    for ( int idx=0; idx<lb.size(); idx++ )
    {
	if ( allsel_ || lb.isChosen(idx) )
	    sellnms_.add( lb.textOfItem(idx) );
    }
    allsel_ = sellnms_.size() == lb.size();
}


bool uiGoogleExport2DSeis::acceptOK( CallBacker* )
{
    mCreateWriter( s2dfm_->name(), SI().name() );

    BufferString ins( "\t\t<LineStyle>\n\t\t\t<color>" );
    ins += lsfld_->getColor().getStdStr(false,-1);
    ins += "</color>\n\t\t\t<width>";
    ins += lsfld_->getWidth() * .1;
    ins += "</width>\n\t\t</LineStyle>";
    wrr.writeIconStyles( 0, 0, ins );

    getFinalSelectedLineNames();
    for ( int idx=0; idx<sellnms_.size(); idx++ )
	addLine( wrr, sellnms_.get(idx) );

    wrr.close();
    return true;
}


void uiGoogleExport2DSeis::addLine( ODGoogle::XMLWriter& wrr, const char* lnm )
{
    const Seis2DDataSet& dset( *s2dfm_->dataset_ );
    const int iln = dset.indexOf( lnm );
    const Survey::Geometry* geom = Survey::GM().getGeometry( lnm );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom)
    if ( iln < 0 || !geom2d )
	return;
    PosInfo::Line2DData l2dd = geom2d->data();
    const int nrposns = l2dd.positions().size();
    if ( nrposns < 2 )
	return;

    const int lnmchoice = putlnmfld_->getIntValue();
    if ( lnmchoice != 0 && lnmchoice < 3 )
	wrr.writePlaceMark( 0, l2dd.positions()[0].coord_, lnm );
    if ( lnmchoice == 1 || lnmchoice == 3 )
	wrr.writePlaceMark( 0, l2dd.positions()[nrposns-1].coord_, lnm );

    TypeSet<Coord> crds;
    for ( int idx=0; idx<nrposns; idx++ )
	crds += l2dd.positions()[idx].coord_;

    BendPointFinder2D bpf( crds, 1 );
    bpf.execute();

    TypeSet<Coord> plotcrds;
    for ( int idx=0; idx<bpf.bendPoints().size(); idx++ )
	plotcrds += crds[ bpf.bendPoints()[idx] ];

    wrr.writeLine( 0, plotcrds, lnm );
}
