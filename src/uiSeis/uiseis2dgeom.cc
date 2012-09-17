/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseis2dgeom.cc,v 1.24 2012/06/21 19:25:17 cvsnanne Exp $";

#include "uiseis2dgeom.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "surv2dgeom.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiseisioobjinfo.h"
#include "uiseissel.h"

static const BufferStringSet emptylnms;


uiSeisDump2DGeom::uiSeisDump2DGeom( uiParent* p, const IOObj* ioobj )
    : uiDialog(p,uiDialog::Setup("Dump 2D line geometry to file",
				 "Specify parameters for 2D geometry dump",
				 "103.1.4"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
    , incnrfld(0)
    , zfld(0)
{
    CallBack cb( mCB(this,uiSeisDump2DGeom,seisSel) );
    if ( ioobj )
    {
	ctio.setObj( ioobj->clone() );
	preFinalise().notify( cb );
    }

    uiSeisSel::Setup ss( Seis::Line ); ss.selattr( false );
    seisfld = new uiSeisSel( this, ctio, ss );
    seisfld->selectionDone.notify( cb );

    lnmsfld = new uiGenInput( this, "One line only",
			      StringListInpSpec(emptylnms) );
    lnmsfld->setWithCheck( true );
    lnmsfld->attach( alignedBelow, seisfld );

    outfld = new uiFileInput( this, "Output file",
	    			uiFileInput::Setup().forread(false) );
    outfld->attach( alignedBelow, lnmsfld );
}


uiSeisDump2DGeom::~uiSeisDump2DGeom()
{
    delete ctio.ioobj; delete &ctio;
}


static void getLineNames( const IOObj& ioobj, BufferStringSet& lnms )
{
    uiSeisIOObjInfo oinf( ioobj );
    oinf.ioObjInfo().getLineNames( lnms );
}


void uiSeisDump2DGeom::seisSel( CallBacker* )
{
    seisfld->commitInput();
    BufferStringSet lnms;
    if ( ctio.ioobj )
	getLineNames( *ctio.ioobj, lnms );

    lnmsfld->newSpec( StringListInpSpec(lnms), 0 );
}


bool uiSeisDump2DGeom::acceptOK( CallBacker* )
{
    if ( !seisfld->commitInput() )
    {
        uiMSG().error( "Please enter the input line set" );
        return false;
    }

    BufferString fnm( outfld->fileName() );
    if ( fnm.isEmpty() )
    {
        uiMSG().error( "Please enter the output file name" );
        return false;
    }

    StreamData sd = StreamProvider( fnm ).makeOStream();
    if ( !sd.usable() )
    {
        uiMSG().error( "Cannot open the output file" );
        return false;
    }

    BufferString lsnm( ctio.ioobj->name() );
    S2DPOS().setCurLineSet( lsnm );

    BufferStringSet lnms;
    if ( lnmsfld->isChecked() )
	lnms.add( lnmsfld->text() );
    else
	getLineNames( *ctio.ioobj, lnms );

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	PosInfo::Line2DData l2dd( lnms.get(idx) );
	S2DPOS().getGeometry( l2dd );
	l2dd.dump( *sd.ostrm, true );
	*sd.ostrm << "\n\n";
    }

    sd.close();
    return true;
}
