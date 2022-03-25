/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiscaler.h"
#include "scaler.h"

#include "uigeninput.h"
#include "uibutton.h"


static const char* scalestrs[] =
	{ sLinScaler, sLogScaler, sExpScaler, 0 };


uiScaler::uiScaler( uiParent* p, const uiString& txt, bool linonly )
	: uiGroup(p,"Scale selector")
	, basefld(0)
	, typefld(0)
{
    uiString lbl = txt;


    if ( lbl.isEmpty() ) lbl = linonly ? tr("Scale values: ")
				       : tr("Scale values");

    if ( !linonly )
    {
	StringListInpSpec spec(scalestrs);
	typefld = new uiGenInput( this, uiStrings::sEmptyString(), spec );
	typefld->valuechanged.notify( mCB(this,uiScaler,typeSel) );
    }

    ynfld = new uiCheckBox( this, lbl );
    ynfld->activated.notify( mCB(this,uiScaler,typeSel) );

    DoubleInpSpec dis;
    linearfld = new uiGenInput( this, tr("Shift/Factor"),
				DoubleInpSpec().setName("Shift"),
				DoubleInpSpec().setName("Factor") );

    if ( !typefld )
	ynfld->attach( leftOf, linearfld );
    else
    {
	ynfld->attach( leftOf, typefld );
	linearfld->attach( alignedBelow, typefld );
	basefld = new uiGenInput( this, tr("Base"), BoolInpSpec(true,
				  toUiString("10"),toUiString("e")));
	basefld->attach( alignedBelow, typefld );
    }

    preFinalize().notify( mCB(this,uiScaler,doFinalize) );
    setHAlignObj( linearfld );
}


void uiScaler::doFinalize( CallBacker* )
{
    typeSel(0);
}


void uiScaler::setUnscaled()
{
    if ( typefld ) typefld->setValue( 0 );
    linearfld->setValue( 0, 0 );
    linearfld->setValue( 1, 1 );
    ynfld->setChecked( false );
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
	double c = linearfld->isUndef(0) ? 0 : linearfld->getDValue(0);
	double f = linearfld->isUndef(1) ? 1 : linearfld->getDValue(1);
	if ( mIsUdf(c) ) c = 0;
	if ( mIsUdf(f) ) f = 1;
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
    ynfld->setChecked( !sc.isEmpty() );

    const FixedString typ = sc.type();
    int typnr = 0;
    if ( typ == sLinScaler )
    {
	const LinScaler& lsc = (const LinScaler&)sc;
	linearfld->setValue( lsc.constant, 0 );
	linearfld->setValue( lsc.factor, 1 );
    }
    else
    {
	if ( !typefld ) return;

	if ( typ == sLogScaler )
	{
	    typnr = 1;
	    basefld->setValue( ((const LogScaler&)sc).ten_ );
	}
	else if ( typ == sExpScaler )
	{
	    typnr = 2;
	    basefld->setValue( ((const ExpScaler&)sc).ten_ );
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
    {
	typefld->setSensitive( ynfld->isChecked() );
	linearfld->display( typ == 0 );
    }
    else
	linearfld->setSensitive( typ == 0 );

    if ( basefld ) basefld->display( typ > 0 );
}


void uiScaler::fillPar( IOPar& iop ) const
{
    Scaler* scl = getScaler();
    if ( !scl )
	iop.removeWithKey( sKey::Scale() );
    else
    {
	BufferString scalerstr( 1024, false );
	scl->put( scalerstr.getCStr(), scalerstr.bufSize() );
	iop.set( sKey::Scale(), scalerstr.buf() );
	delete scl;
    }
}


void uiScaler::usePar( const IOPar& iop )
{
    const FixedString res = iop.find( sKey::Scale() );
    if ( res.isEmpty() )
	return;

    Scaler* scl = Scaler::get( res );
    if ( !scl )
	return;

    setInput( *scl );
    delete scl;
}
