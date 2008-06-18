/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2002
 RCS:           $Id: uiattrinpdlg.cc,v 1.14 2008-06-18 08:20:40 cvssatyaki Exp $
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
			    bool issteer, bool is2d, bool multiinpcube )
    : uiDialog(p,uiDialog::Setup("Attribute set definition",
		       issteer ? "Select Steering input"
			       : "Select Seismic input",
				 "101.1.2"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
    , ctiosteer(*mMkCtxtIOObj(SeisTrc))
    , issteer_(issteer)
    , multiinpcube_(multiinpcube)
{
    IOM().setRootDir( GetDataDir() ); 
    IOM().to( ctio.ctxt.getSelKey() ); 
    ctio.ctxt.forread = true;
    ctio.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    ctio.ctxt.includeconstraints = false;
    ctio.ctxt.allowcnstrsabsent = true;
    
    IOM().to( ctiosteer.ctxt.getSelKey() ); 
    ctiosteer.ctxt.forread = true;
    ctiosteer.ctxt.parconstraints.set( sKey::Type, sKey::Steering );
    
    if ( multiinpcube )
    {
	ctio.ctxt.includeconstraints = issteer;
	ctio.ctxt.allowcnstrsabsent = !issteer;

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
	
	inpfld = new uiSeisSel( this, ctio,
			    uiSeisSel::Setup( is2d ? Seis::Line : Seis::Vol )
			    .seltxt( issteer ? "Input Steering cube" 
					     : "Input Seismics") ); 
	inpfld->attach( alignedBelow, txtfld );
    }
    else
    {
	seisinpfld = new uiSeisSel( this, ctio,
			    uiSeisSel::Setup(is2d ? Seis::Line : Seis::Vol)
			    .seltxt("Input Seismics") ); 
	steerinpfld = new uiSeisSel( this, ctiosteer,
			    uiSeisSel::Setup( is2d ? Seis::Line : Seis::Vol )
			    .seltxt( "Input Steering cube" ) ); 
	steerinpfld->attach( alignedBelow, seisinpfld );
    }

}

bool uiAttrInpDlg::is2D() const
{
    return inpfld->is2D();
}

bool uiAttrInpDlg::acceptOK( CallBacker* )
{
    if ( multiinpcube_ ) 
    {
	if ( !inpfld->commitInput( false ) )
	{
	    uiMSG().error( "Please, select input" );
	    return false;
	}
    }
    else
    {
	if ( !seisinpfld->commitInput( false ) )
	{
	    uiMSG().error( "Please, select input" );
	    return false;
	}
	if ( !steerinpfld->commitInput( false ) )
	{
	    uiMSG().error( "Please, select input" );
	    return false;
	}
    }

    return true;
}


uiAttrInpDlg::~uiAttrInpDlg()
{
    delete ctio.ioobj; delete &ctio;
    delete ctiosteer.ioobj; delete &ctiosteer;
}


const char* uiAttrInpDlg::getUserRef() const
{
    return inpfld->getInput();
}


const char* uiAttrInpDlg::getSeisRef() const
{
    return seisinpfld->getInput();
}


const char* uiAttrInpDlg::getSteerRef() const
{
    return steerinpfld->getInput();
}


const char* uiAttrInpDlg::getKey() const
{
    static LineKey lk;
    lk.setLineName( ctio.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( issteer_ ? sKey::Steering : inpfld->attrNm() );

    return lk;
}


const char* uiAttrInpDlg::getSeisKey() const
{
    static LineKey lk;
    lk.setLineName( ctio.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( seisinpfld->attrNm() );

    return lk;
}


const char* uiAttrInpDlg::getSteerKey() const
{
    static LineKey lk;
    lk.setLineName( ctiosteer.ioobj->key() );
    if ( is2D() )
	lk.setAttrName( steerinpfld->attrNm() );

    return lk;
}
