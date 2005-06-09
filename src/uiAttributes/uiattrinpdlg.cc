/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.cc,v 1.1 2005-06-09 13:11:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiattrinpdlg.h"

#include "bufstringset.h"
#include "uiseissel.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"


static const char* seislbl[] = { "Select Seismics", 0 };
static const char* steerlbl[] = { "Select Steering cube", 0 };


uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
		       issteer ? "Select Steering input"
			       : "Select Seismic input",
				 "101.1.2"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
{
    BufferString infotxt( "Provide input for the following attributes: " );
    uiLabel* infolbl = new uiLabel( this, infotxt );

    BufferString txt;
    for ( int idx=0; idx<refset.size(); idx++ )
	{ txt += refset.get(idx); txt += "\n"; }
    
    txtfld = new uiTextEdit( this, "File Info", true );
    txtfld->setPrefHeightInChar( 10 );
    txtfld->setPrefWidthInChar( 40 );
    txtfld->setText( txt );
    txtfld->attach( alignedBelow, infolbl );
  
    IOM().setRootDir( GetDataDir() ); 
    IOM().to( ctio.ctxt.stdSelKey() ); 
    ctio.ctxt.forread = true;
    ctio.ctxt.ioparkeyval[0] = sKey::Type;
    ctio.ctxt.ioparkeyval[1] = sKey::Steering;
    ctio.ctxt.includekeyval = issteer;

    inpfld = new uiSeisSel( this, ctio, SeisSelSetup(), false, 
	    		    issteer ? steerlbl : seislbl );
    inpfld->attach( alignedBelow, txtfld );
}


void uiAttrInpDlg::set2DPol( Pol2D pol )
{
    inpfld->set2DPol( pol );
}


bool uiAttrInpDlg::is2D() const
{
    return inpfld->is2D();
}


bool uiAttrInpDlg::acceptOK( CallBacker* )
{
    if ( !inpfld->commitInput( false ) )
    {
	uiMSG().error( "Please, select input" );
	return false;
    }

    return true;
}

uiAttrInpDlg::~uiAttrInpDlg()
{
    delete ctio.ioobj; delete &ctio;
}


const char* uiAttrInpDlg::getUserRef()
{
    return inpfld->getInput();
}

const char* uiAttrInpDlg::getDefStr()
{
    return inpfld->getKey();
}
