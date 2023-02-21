/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibuttonstateedit.h"
#include "uigeninput.h"
#include "keyenum.h"

uiButtonStateEdit::uiButtonStateEdit( uiParent* p, const uiString& label,
				      int initialstate )
    : uiGroup( p )
{
    states_ += 0;
    states_ += (int) OD::ShiftButton;
    states_ += (int) OD::ControlButton;
    states_ += (int) OD::AltButton;
    states_ += (int) OD::ShiftButton | OD::ControlButton;
    states_ += (int) OD::ShiftButton | OD::AltButton;
    states_ += (int) OD::ControlButton | OD::AltButton;
    states_ += (int) OD::ShiftButton | OD::ControlButton | OD::AltButton;

    uiStringSet set;
    for ( int idx=0; idx<states_.size(); idx++ )
	set.add( createName(states_[idx]) );

    combobox_ = new uiGenInput( this, label, StringListInpSpec( set ) );
    combobox_->setText( createName(initialstate).getFullString() );
    setHAlignObj( combobox_ );
}


uiButtonStateEdit::~uiButtonStateEdit()
{}


int uiButtonStateEdit::getState() const
{
    return states_[combobox_->getIntValue()];
}


uiString uiButtonStateEdit::createName( int status )
{
    uiString res;
    if ( !status )
	res = uiStrings::sNone();
    else
    {
	bool first = true;
	if ( status & OD::ShiftButton )
	{
	    res = tr("Shift");
	    first = false;
	}

	if ( status & OD::ControlButton )
	{
	    const uiString control = tr("Control");
	    if ( !first )
		res = toUiString( "%1-%2").arg( res ).arg( control );
	    else
		res = control;
	    first = false;
	}

	if ( status & OD::AltButton )
	{
	    const uiString alt = tr("Alt");

	    if ( !first )
		res = toUiString( "%1-%2").arg( res ).arg( alt );
	    else
		res = alt;

	    first = false;
	}
    }

    return res;
}
