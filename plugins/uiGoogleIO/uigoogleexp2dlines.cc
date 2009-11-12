/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2009
-*/

static const char* rcsID = "$Id";

#include "uigoogleexp2dlines.h"
#include "odgooglexmlwriter.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uisellinest.h"
#include "uiseis2dfileman.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "posinfo.h"
#include "strmprov.h"
#include "draw.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "bendpointfinder.h"
#include "latlong.h"
#include <iostream>

static const char* sIconFileName = "markerdot";

static const char* getDlgTitle( uiSeis2DFileMan& sfm )
{
    static BufferString ret; ret = "Export to KML: ";

    const uiListBox& lb( *sfm.getListBox(false) );
    const int nrsel = lb.nrSelected();
    if ( nrsel == 0 || (nrsel > 1 && nrsel == lb.size()) )
	{ ret += "entire '"; ret += sfm.lineset_->name(); ret += "'"; }
    else
    {
	int nrdone = 0;
	for ( int idx=0; idx<lb.size(); idx++ )
	{
	    if ( lb.isSelected(idx) )
	    {
		if ( nrdone > 2 )
		    { ret += ", ..."; break; }
		if ( nrdone > 0 )
		    ret += ", ";
		ret += lb.textOfItem(idx);
		nrdone++;
	    }
	}
    }

    return ret.buf();
}


uiGoogleExport2DSeis::uiGoogleExport2DSeis( uiSeis2DFileMan* p )
    : uiDialog(p,uiDialog::Setup("Export selected 2D seismics to KML",
				 getDlgTitle(*p),"0.3.10") )
    , s2dfm_(p)
{
    static const char* choices[]
		= { "No", "At Start/End", "At Start only", "At End only", 0 };
    putlnmfld_ = new uiGenInput( this, "Put line names",
	    			 StringListInpSpec(choices) );
    putlnmfld_->setValue( 2 );

    LineStyle ls( LineStyle::Solid, 10, Color(0,0,255) );
    lsfld_ = new uiSelLineStyle( this, ls, "Line style", false, true, true );
    lsfld_->attach( alignedBelow, putlnmfld_ );

    fnmfld_ = new uiFileInput( this, "Output file",
		uiFileInput::Setup(uiFileDialog::Gen,GetBaseDataDir())
		.forread(false).filter("*.kml") );
    fnmfld_->attach( alignedBelow, lsfld_ );
}


uiGoogleExport2DSeis::~uiGoogleExport2DSeis()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiGoogleExport2DSeis::acceptOK( CallBacker* )
{
    const BufferString fnm( fnmfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet("Please enter a file name" )

    const uiListBox& lb( *s2dfm_->getListBox(false) );
    BufferStringSet sellnms;
    const int nrsel = lb.nrSelected();
    const bool selall = nrsel == 0 || nrsel == lb.size();
    for ( int idx=0; idx<lb.size(); idx++ )
    {
	if ( selall || lb.isSelected(idx) )
	    sellnms.add( lb.textOfItem(idx) );
    }

    ODGoogle::XMLWriter wrr( "2D Lines", fnm, SI().name() );
    if ( !wrr.isOK() )
	mErrRet(wrr.errMsg())

    BufferString ins( "\t\t<LineStyle>\n\t\t\t<color>" );
    ins += lsfld_->getColor().getStdStr(false,-1);
    ins += "</color>\n\t\t\t<width>";
    ins += lsfld_->getWidth() * .1;
    ins += "</width>\n\t\t</LineStyle>";
    wrr.writeIconStyles( 0, 0, ins );

    const uiSeisIOObjInfo& oinf( *s2dfm_->objinfo_ );
    for ( int idx=0; idx<sellnms.size(); idx++ )
    {
	BufferStringSet attrnms;
	oinf.getAttribNamesForLine( sellnms.get(idx), attrnms );
	if ( attrnms.isEmpty() ) continue;
	int iattr = attrnms.indexOf( LineKey::sKeyDefAttrib() );
	if ( iattr < 0 ) iattr = 0;
	addLine( wrr, sellnms.get(idx), iattr );
    }

    wrr.close();
    return true;
}


void uiGoogleExport2DSeis::addLine( ODGoogle::XMLWriter& wrr, const char* lnm,
				    int iattr )
{
    const Seis2DLineSet& lset( *s2dfm_->lineset_ );
    LineKey lk( lnm, lset.attribute(iattr) );
    const int iln = lset.indexOf( lk );
    PosInfo::Line2DData l2dd;
    if ( iln < 0 || !lset.getGeometry(iln,l2dd) )
	return;
    const int nrposns = l2dd.posns_.size();
    if ( nrposns < 2 )
	return;

    const int lnmchoice = putlnmfld_->getIntValue();
    if ( lnmchoice != 0 && lnmchoice < 3 )
	wrr.writePlaceMark( 0, l2dd.posns_[0].coord_, lnm );
    if ( lnmchoice == 1 || lnmchoice == 3 )
	wrr.writePlaceMark( 0, l2dd.posns_[nrposns-1].coord_, lnm );

    TypeSet<Coord> crds;
    for ( int idx=0; idx<nrposns; idx++ )
	crds += l2dd.posns_[idx].coord_;

    BendPointFinder2D bpf( crds, 1 );
    bpf.execute();

    TypeSet<Coord> plotcrds;
    for ( int idx=0; idx<bpf.bendPoints().size(); idx++ )
	plotcrds += crds[ bpf.bendPoints()[idx] ];

    wrr.writeLine( 0, plotcrds, lnm );
}
