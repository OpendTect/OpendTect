/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Jun 2002
 RCS:		$Id: uiseiscbvsimp.cc,v 1.1 2002-06-19 15:42:21 bert Exp $
________________________________________________________________________

-*/

#include "uiseiscbvsimp.h"
#include "seistrctr.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"

#include "uimsg.h"
#include "uifileinput.h"
#include "uiioobjsel.h"


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,"Import CBVS format file")
	, ctio_(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
{
    setTitleText( "Import CBVS cube " );

    inpfld = new uiFileInput( this, "(First) CBSV file name", GetDataDir(),
	   			true, "*.cbvs" );

    StringListInpSpec spec;
    spec.addString( "Input data cube" );
    spec.addString( "Generated attribute cube" );
    spec.addString( "Steering cube" );
    typefld = new uiGenInput( this, "Cube type", spec );
    typefld->attach( alignedBelow, inpfld );

    ctio_.ctxt.forread = false;
    ctio_.ctxt.trglobexpr = "CBVS";
    IOM().to( ctio_.ctxt.stdSelKey() );
    seissel = new uiIOObjSel( this, ctio_, "Cube name" );
}


uiSeisImpCBVS::~uiSeisImpCBVS()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    if ( !seissel->commitInput(false) )
    {
	uiMSG().error( "Please choose a name for the cube" );
	return false;
    }

    const int seltyp = typefld->getIntValue();
    int nrcomp = 1;

    if ( !seltyp )
	ctio_.ioobj->pars().removeWithKey( "Type" );
    else
	ctio_.ioobj->pars().set( "Type",
	    			 seltyp == 1 ? "Attribute" : "Steering" );


    return true;
}
