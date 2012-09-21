/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uibuttonstateedit.h"
#include "uigeninput.h"
#include "keyenum.h"
#include "keystrs.h"

uiButtonStateEdit::uiButtonStateEdit( uiParent* p, const char* label,
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

    BufferStringSet set;
    for ( int idx=0; idx<states_.size(); idx++ )
	set.add( createName(states_[idx]).buf() );

    combobox_ = new uiGenInput( this, label, StringListInpSpec( set ) );
    combobox_->setText( createName(initialstate).buf() );
    setHAlignObj( combobox_ );
}


int uiButtonStateEdit::getState() const
{
    return states_[combobox_->getIntValue()];
}


BufferString uiButtonStateEdit::createName( int status )
{
    BufferString res;
    if ( !status )
	res = sKey::None();
    else
    {
	bool first = true;
	if ( status & OD::ShiftButton )
	{
	    res += "Shift";
	    first = false;
	}

	if ( status & OD::ControlButton )
	{
	    if ( !first ) res += "-";
	    res += "Control";
	    first = false;
	}

	if ( status & OD::AltButton )
	{
	    if ( !first ) res += "-";
	    res += "Alt";
	    first = false;
	}
    }

    return res;
}

