/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiscaler.cc,v 1.1 2002-05-30 15:03:13 nanne Exp $
________________________________________________________________________

-*/

#include "uiscaler.h"
#include "scaler.h"

#include "uigeninput.h"


static const char* scalestrs[] =
{
        "None",
	sLinScaler,
        0
};


uiScaler::uiScaler( uiParent* p, const char* lbl, bool linonly )
	: uiGroup(p,"Scale selector")
	, scaler(0)
	, basefld(0)
{
    StringListInpSpec spec(scalestrs);
    if ( !linonly )
    {
	spec.addString( sLogScaler );
	spec.addString( sExpScaler );
    }

    typefld = new uiGenInput( this, lbl, spec );
    typefld->valuechanged.notify( mCB(this,uiScaler,typeSel) );

    linearfld = new uiGenInput( this, "Shift/Factor", DoubleInpSpec(), 
				DoubleInpSpec() );
    linearfld->attach( alignedBelow, typefld );

    if ( !linonly )
    {
	basefld = new uiGenInput( this, "Base", BoolInpSpec("10","e") );
	basefld->attach( alignedBelow, typefld );
    }

    uiObj()->finalising.notify( mCB(this,uiScaler,doFinalise) );
    setHAlignObj( typefld->uiObj() );
}


uiScaler::~uiScaler()
{
    delete scaler;
}


void uiScaler::doFinalise( CallBacker* )
{
    typeSel(0);
}


Scaler* uiScaler::getScaler()
{
    delete scaler; scaler = 0;
    int typ = typefld->getIntValue();
    switch ( typ )
    {
    case 1: {
	double c = linearfld->getValue(0);
	double f = linearfld->getValue(1);
	if ( mIsUndefined(c) ) c = 0;
	if ( mIsUndefined(f) ) f = 1;
	scaler = new LinScaler( c, f );
	} break;
    case 2: {
	scaler = new LogScaler( basefld->getBoolValue() );
	} break;
    case 3: {
	scaler = new ExpScaler( basefld->getBoolValue() );
	} break;
    }

    return scaler;
}


void uiScaler::typeSel( CallBacker* )
{
    int typ = typefld->getIntValue();
    linearfld->display( typ == 1 );
    if ( basefld ) basefld->display( typ > 1 );
}

