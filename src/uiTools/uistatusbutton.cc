/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2022
________________________________________________________________________

-*/

#include "uistatusbutton.h"
#include "uitoolbar.h"


uiStatusButton::uiStatusButton( uiParent* p, const EnumDef& enumdef,
				const char** iconnms, int defenum )
    : uiPushButton(p, uiString::empty(), false)
    , statusdef_(enumdef)
    , iconnames_(iconnms)
{
    setValue( defenum );
}


uiStatusButton::~uiStatusButton()
{
}


void uiStatusButton::setValue( int status )
{
    int idx = statusdef_.indexOf( status );
    if ( !iconnames_.validIdx(idx) )
	return;
    setIcon( iconnames_.get(idx) );
    setToolTip( statusdef_.getUiStringForIndex(idx) );
    setPrefWidth( prefVNrPics() );
    status_ = status;
}


int uiStatusButton::getValue() const
{
    return status_;
}
