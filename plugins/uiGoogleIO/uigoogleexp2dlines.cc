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
#include "uiseissel.h"
#include "uisellinest.h"
#include "uimsg.h"
#include "oddirs.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "seis2dline.h"
#include "latlong.h"
#include <iostream>


uiGoogleExport2DSeis::uiGoogleExport2DSeis( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Export 2D line positions to KML",
				 "Specify lines to output","0.3.10") )
{
    IOObjContext ctxt( mIOObjContext(SeisTrc) );
    uiSeisSel::fillContext( Seis::Line, true, ctxt );
    uiSeisSel::Setup su( true, false ); su.selattr( false );
    inpfld_ = new uiSeisSel( this, ctxt, su );
    inpfld_->selectiondone.notify( mCB(this,uiGoogleExport2DSeis,inpSel) );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Lines", true );
    selfld_ = llb->box();
    llb->attach( alignedBelow, inpfld_ );

    LineStyle ls( LineStyle::Solid, 10, Color(0,0,255) );
    lsfld_ = new uiSelLineStyle( this, ls, "Line style", false, true, true );
    lsfld_->attach( alignedBelow, llb );

    fnmfld_ = new uiFileInput( this, "Output file",
		uiFileInput::Setup(uiFileDialog::Gen,GetBaseDataDir())
		.forread(false).filter("*.kml") );
    fnmfld_->attach( alignedBelow, lsfld_ );


    finaliseStart.notify( mCB(this,uiGoogleExport2DSeis,initWin) );
}


uiGoogleExport2DSeis::~uiGoogleExport2DSeis()
{
}


void uiGoogleExport2DSeis::initWin( CallBacker* )
{
    inpSel( 0 );
}


void uiGoogleExport2DSeis::inpSel( CallBacker* )
{
    //TODO fill selfld_ with lines
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
