/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiseis2dgeom.cc,v 1.26 2012-05-02 15:12:16 cvskris Exp $";

#include "uiseis2dgeom.h"
#include "bufstringset.h"
#include "seistrctr.h"
#include "seisselection.h"
#include "seis2dline.h"
#include "uiseissel.h"
#include "uigeninput.h"
#include "uifileinput.h"
#include "uiseisioobjinfo.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "executor.h"
#include "survinfo.h"
#include "surv2dgeom.h"
#include "strmprov.h"
#include "oddirs.h"
#include "ioobj.h"

static const BufferStringSet emptylnms;


uiSeisDump2DGeom::uiSeisDump2DGeom( uiParent* p, const IOObj* ioobj )
    : uiDialog(p,uiDialog::Setup("Dump 2D line geometry to file",
				 "Specify parameters for 2D geometry dump",
				 "103.1.4"))
    , ctio(*mMkCtxtIOObj(SeisTrc))
{
    CallBack cb( mCB(this,uiSeisDump2DGeom,seisSel) );
    if ( ioobj )
    {
	ctio.setObj( ioobj->clone() );
	preFinalise().notify( cb );
    }
    seisfld = new uiSeisSel( this, ctio, uiSeisSel::Setup(Seis::Line) );
    seisfld->selectionDone.notify( cb );

    lnmsfld = new uiGenInput( this, "One line only",
			      StringListInpSpec(emptylnms) );
    lnmsfld->setWithCheck( true );
    lnmsfld->attach( alignedBelow, seisfld );

    incnrfld = new uiGenInput( this, "Start with trace number",
	    		       BoolInpSpec(true) );
    incnrfld->attach( alignedBelow, lnmsfld );

    BufferString txt( "Add Z value" ); txt += SI().getZUnitString(true);
    const float zval = SI().zRange(true).start * SI().zDomain().userFactor();
    zfld = new uiGenInput( this, txt, FloatInpSpec(zval) );
    zfld->setWithCheck( true );
    zfld->attach( alignedBelow, incnrfld );

    outfld = new uiFileInput( this, "Output file",
	    			uiFileInput::Setup().forread(false) );
    outfld->attach( alignedBelow, zfld );
}


uiSeisDump2DGeom::~uiSeisDump2DGeom()
{
    delete ctio.ioobj; delete &ctio;
}


void uiSeisDump2DGeom::seisSel( CallBacker* )
{
    seisfld->commitInput();
    BufferStringSet lnms;
    if ( ctio.ioobj )
    {
	uiSeisIOObjInfo oinf( *ctio.ioobj );
	oinf.ioObjInfo().getLineNames( lnms );
    }

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

    float zval = zfld->isChecked() ? zfld->getfValue() : mUdf(float);
    if ( !mIsUdf(zval) ) zval /= SI().zDomain().userFactor();
    const bool incnr = incnrfld->getBoolValue();
    LineKey lk;
    if ( lnmsfld->isChecked() )
	lk.setLineName( lnmsfld->text() );
    lk.setAttrName( seisfld->attrNm() );
    Seis2DLineSet ls( ctio.ioobj->fullUserExpr(true) );

    S2DPOS().setCurLineSet( ls.name() );
    PosInfo::Line2DData l2dd( lk.lineName() );
    S2DPOS().getGeometry( l2dd );
    l2dd.write( *sd.ostrm, true );

    sd.close();
    return true;
}
