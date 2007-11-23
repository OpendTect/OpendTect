/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.cc,v 1.10 2007-11-23 11:59:06 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiattrinpdlg.h"

#include "bufstringset.h"
#include "uiseissel.h"
#include "uitextedit.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "ctxtioobj.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "uilabel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "oddirs.h"


static const char* seislbl[] = { "Select Seismics", 0 };
static const char* steerlbl[] = { "Select Steering cube", 0 };

uiAttrInpDlg::uiAttrInpDlg( uiParent* p, const BufferStringSet& refset, 
			    bool issteer, bool is2d )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
		       issteer ? "Select Steering input"
			       : "Select Seismic input",
				 "101.1.2"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
    , issteer_(issteer)
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
    IOM().to( ctio.ctxt.getSelKey() ); 
    ctio.ctxt.forread = true;
    ctio.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    ctio.ctxt.includeconstraints = issteer;
    ctio.ctxt.allowcnstrsabsent = !issteer;

    inpfld = new uiSeisSel( this, ctio, Seis::SelSetup(is2d), false, 
	    		    issteer ? steerlbl : seislbl );
    inpfld->attach( alignedBelow, txtfld );
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


const char* uiAttrInpDlg::getUserRef() const
{
    return inpfld->getInput();
}


const char* uiAttrInpDlg::getKey() const
{
    static LineKey lk;
    lk.setLineName( ctio.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( issteer_ ? sKey::Steering : inpfld->attrNm() );

    return lk;
}
