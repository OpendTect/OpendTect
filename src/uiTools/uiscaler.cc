/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiscaler.cc,v 1.5 2002-08-02 10:27:23 bert Exp $
________________________________________________________________________

-*/

#include "uiscaler.h"
#include "scaler.h"

#include "uigeninput.h"
#include "uibutton.h"


static const char* scalestrs[] =
	{ sLinScaler, sLogScaler, sExpScaler, 0 };


uiScaler::uiScaler( uiParent* p, const char* lbl, bool linonly )
	: uiGroup(p,"Scale selector")
	, basefld(0)
	, typefld(0)
{
    if ( !lbl ) lbl = linonly ? "Scale values: " : "Scale values";

    if ( !linonly )
    {
	StringListInpSpec spec(scalestrs);
	typefld = new uiGenInput( this, "", spec );
	typefld->valuechanged.notify( mCB(this,uiScaler,typeSel) );
    }

    ynfld = new uiCheckBox( this, lbl );
    ynfld->activated.notify( mCB(this,uiScaler,typeSel) );
    linearfld = new uiGenInput( this, "Shift/Factor", DoubleInpSpec(), 
				DoubleInpSpec() );
    if ( !typefld )
	ynfld->attach( leftOf, linearfld );
    else
    {
	ynfld->attach( leftOf, typefld );
	linearfld->attach( alignedBelow, typefld );
	basefld = new uiGenInput( this, "Base", BoolInpSpec("10","e") );
	basefld->attach( alignedBelow, typefld );
    }

    uiObj()->finalising.notify( mCB(this,uiScaler,doFinalise) );
    setHAlignObj( linearfld->uiObj() );
}


void uiScaler::doFinalise( CallBacker* )
{
    typeSel(0);
}


Scaler* uiScaler::getScaler() const
{
    if ( !ynfld->isChecked() ) return 0;

    int typ = typefld ? typefld->getIntValue() : 0;
    Scaler* scaler = 0;
    switch ( typ )
    {
    case 0: {
	double c = linearfld->getValue(0);
	double f = linearfld->getValue(1);
	if ( mIsUndefined(c) ) c = 0;
	if ( mIsUndefined(f) ) f = 1;
	scaler = new LinScaler( c, f );
	} break;
    case 1: {
	scaler = new LogScaler( basefld->getBoolValue() );
	} break;
    case 2: {
	scaler = new ExpScaler( basefld->getBoolValue() );
	} break;
    }

    if ( scaler->isEmpty() )
	{ delete scaler; scaler = 0; }
    return scaler;
}


void uiScaler::setInput( const Scaler& sc )
{
    const char* typ = sc.type();
    int typnr = 0;
    if ( !strcmp(typ,sLinScaler) )
    {
	const LinScaler& lsc = (const LinScaler&)sc;
	linearfld->setValue( lsc.constant, 0 );
	linearfld->setValue( lsc.factor, 1 );
    }
    else
    {
	if ( !typefld ) return;

	if ( !strcmp(typ,sLogScaler) )
	{
	    typnr = 1;
	    basefld->setValue( ((const LogScaler&)sc).ten );
	}
	else if ( !strcmp(typ,sExpScaler) )
	{
	    typnr = 2;
	    basefld->setValue( ((const ExpScaler&)sc).ten );
	}
	else return;
    }

    if ( typefld )
	typefld->setValue( typnr );

    typeSel(0);
}


void uiScaler::typeSel( CallBacker* )
{
    int typ = typefld ? typefld->getIntValue() : 0;
    if ( !ynfld->isChecked() ) typ = -1;

    if ( typefld )
	linearfld->display( typ == 0 );
    else
	linearfld->setSensitive( typ == 0 );
    if ( basefld ) basefld->display( typ > 0 );
}
