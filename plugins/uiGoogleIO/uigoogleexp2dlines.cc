/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert Bril
 * DATE     : Nov 2009
-*/

static const char* rcsID = "$Id";

#include "uigoogleexp2dlines.h"
#include "odgooglexmlwriter.h"
#include "uifileinput.h"
#include "uisellinest.h"
#include "uiseis2dfileman.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "strmprov.h"
#include "draw.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "latlong.h"
#include <iostream>


uiGoogleExport2DSeis::uiGoogleExport2DSeis( uiSeis2DFileMan* p )
    : uiDialog(p,uiDialog::Setup("Export selected 2D seismics to KML",
				 "Specify properties","0.3.10") )
    , s2dfm_(p)
{
    static const char* choices[] = { "No", "Start", "Start/End", "End", 0 };
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

    ODGoogle::XMLWriter wrr( "2D Lines", fnm, SI().name() );
    if ( !wrr.isOK() )
	mErrRet(wrr.errMsg())

    BufferString ins( "\t\t<LineStyle>\n\t\t\t<color>" );
    ins += lsfld_->getColor().getStdStr(false,-1);
    ins += "</color>\n\t\t\t<width>";
    ins += lsfld_->getWidth() * .1;
    ins += "</width>\n\t\t</LineStyle>";
    wrr.writeIconStyles( "markerdot", ins );

    //TODO : write 2D line geometries

    wrr.close();
    return true;
}
