/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiscaler.h"
#include "keystrs.h"
#include "scaler.h"

#include "uigeninput.h"
#include "uibutton.h"
#include "uicombobox.h"



uiScaler::uiScaler( uiParent* p, const uiString& txt, bool linonly )
	: uiGroup(p,"Scale selector")
{
    uiString lbltxt = txt;
    if ( txt.isEmpty() )
	lbltxt = tr("Scale values");

    if ( !linonly )
    {
	uiStringSet typs( uiStrings::sLinear(), uiStrings::sLogarithmic(),
			  uiStrings::sExponential() );
	typefld_ = new uiComboBox( this, "Scaling Type" );
	typefld_->addItems( typs );
	typefld_->selectionChanged.notify( mCB(this,uiScaler,typeSel) );
    }

    ynfld_ = new uiCheckBox( this, lbltxt );
    ynfld_->activated.notify( mCB(this,uiScaler,typeSel) );

    DoubleInpSpec dis;
    linearfld_ = new uiGenInput( this, tr("Shift/Factor"),
				DoubleInpSpec().setName("Shift"),
				DoubleInpSpec().setName("Factor") );

    if ( !typefld_ )
	ynfld_->attach( leftOf, linearfld_ );
    else
    {
	ynfld_->attach( leftOf, typefld_ );
	linearfld_->attach( alignedBelow, typefld_ );
	basefld_ = new uiGenInput( this, uiStrings::sBase(true),
		BoolInpSpec(true,toUiString("10"),toUiString("e")));
	basefld_->attach( alignedBelow, typefld_ );
    }

    preFinalise().notify( mCB(this,uiScaler,doFinalise) );
    setHAlignObj( linearfld_ );
}


void uiScaler::doFinalise( CallBacker* )
{
    typeSel(0);
}


void uiScaler::setUnscaled()
{
    if ( typefld_ )
	typefld_->setCurrentItem( 0 );
    linearfld_->setValue( 0, 0 );
    linearfld_->setValue( 1, 1 );
    ynfld_->setChecked( false );
    typeSel(0);
}


Scaler* uiScaler::getScaler() const
{
    if ( !ynfld_->isChecked() )
	return 0;

    int typ = typefld_ ? typefld_->currentItem() : 0;
    Scaler* scaler = 0;
    switch ( typ )
    {
    case 0: {
	double c = linearfld_->isUndef(0) ? 0 : linearfld_->getDValue(0);
	double f = linearfld_->isUndef(1) ? 1 : linearfld_->getDValue(1);
	if ( mIsUdf(c) ) c = 0;
	if ( mIsUdf(f) ) f = 1;
	scaler = new LinScaler( c, f );
    } break;
    case 1: {
	scaler = new LogScaler( basefld_->getBoolValue() );
    } break;
    case 2: {
	scaler = new ExpScaler( basefld_->getBoolValue() );
    } break;
    }

    if ( scaler->isEmpty() )
	{ delete scaler; scaler = 0; }
    return scaler;
}


void uiScaler::setInput( const Scaler& sc )
{
    ynfld_->setChecked( !sc.isEmpty() );

    const FixedString typ = sc.type();
    int typnr = 0;
    if ( typ == sLinScaler )
    {
	const LinScaler& lsc = (const LinScaler&)sc;
	linearfld_->setValue( lsc.constant_, 0 );
	linearfld_->setValue( lsc.factor_, 1 );
    }
    else
    {
	if ( !typefld_ )
	    return;

	if ( typ == sLogScaler )
	{
	    typnr = 1;
	    basefld_->setValue( ((const LogScaler&)sc).ten_ );
	}
	else if ( typ == sExpScaler )
	{
	    typnr = 2;
	    basefld_->setValue( ((const ExpScaler&)sc).ten_ );
	}
	else return;
    }

    if ( typefld_ )
	typefld_->setCurrentItem( typnr );

    typeSel(0);
}


void uiScaler::typeSel( CallBacker* )
{
    auto typ = typefld_ ? typefld_->currentItem() : 0;
    if ( !ynfld_->isChecked() )
	typ = -1;

    if ( typefld_ )
    {
	typefld_->setSensitive( ynfld_->isChecked() );
	linearfld_->display( typ == 0 );
    }
    else
	linearfld_->setSensitive( typ == 0 );

    if ( basefld_ )
	basefld_->display( typ > 0 );
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
