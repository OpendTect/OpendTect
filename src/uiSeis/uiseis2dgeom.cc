/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:		$Id: uiseis2dgeom.cc,v 1.1 2004-12-06 20:16:13 bert Exp $
________________________________________________________________________

-*/

#include "uiseis2dgeom.h"
#include "bufstringset.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uiseisioobjinfo.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "survinfo.h"
#include "ioobj.h"

static const BufferStringSet emptylnms;


uiSeisDump2DGeom::uiSeisDump2DGeom( uiParent* p, const IOObj* ioobj )
    : uiDialog(p,uiDialog::Setup("Dump 2D line geometryto file",
				 "Specify parameters for 2D geometry dump",
				 "103.1.4"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
{
    CallBack cb( mCB(this,uiSeisDump2DGeom,seisSel) );
    if ( ioobj )
    {
	ctio.setObj( ioobj->clone() );
	mainObject()->finaliseStart.notify( cb );
    }
    seisfld = new uiSeisSel( this, ctio,
	    	  SeisSelSetup().pol2d(Only2D).subsel(false) );
    seisfld->selectiondone.notify( cb );

    lnmsfld = new uiGenInput( this, "One line only",
			      StringListInpSpec(emptylnms) );
    lnmsfld->setWithCheck( true );
    lnmsfld->attach( alignedBelow, seisfld );

    zfld = new uiGenInput( this, "Add Z value",
	    		   FloatInpSpec(SI().zRange().start) );
    zfld->setWithCheck( true );
    zfld->attach( alignedBelow, lnmsfld );

    outfld = new uiFileInput( this, "Output file",
	    			uiFileInput::Setup().forread(false) );
    outfld->setDefaultSelectionDir( GetDataDir() );
    outfld->attach( alignedBelow, zfld );
}


uiSeisDump2DGeom::~uiSeisDump2DGeom()
{
    delete ctio.ioobj; delete &ctio;
}


void uiSeisDump2DGeom::seisSel( CallBacker* )
{
    seisfld->commitInput( false );
    BufferStringSet lnms;
    if ( ctio.ioobj )
    {
	uiSeisIOObjInfo oinf( *ctio.ioobj );
	oinf.getLineNames( lnms );
    }

    lnmsfld->newSpec( StringListInpSpec(lnms), 0 );
}


bool uiSeisDump2DGeom::acceptOK( CallBacker* )
{
    if ( !seisfld->commitInput(false) )
    {
        uiMSG().error( "Please enter the input line set" );
        return false;
    }
    BufferString fnm( outfld->fileName() );
    if ( fnm == "" )
    {
        uiMSG().error( "Please enter the output file name" );
        return false;
    }

    //TODO
    uiMSG().message( "Sorry - not implemented yet" );
    return true;
}
