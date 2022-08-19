/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistatusbutton.h"
#include "uimsg.h"
#include "uitoolbar.h"


uiStatusButton::uiStatusButton( uiParent* p, const EnumDef& enumdef,
				const char** iconnms, int defenum )
    : uiPushButton(p, uiString::empty(), false)
    , statusdef_(enumdef)
    , iconnames_(iconnms)
{
    setValue( defenum );
    mAttachCB(activated, uiStatusButton::showmsgCB);
}


uiStatusButton::~uiStatusButton()
{
    detachAllNotifiers();
}


void uiStatusButton::setValue( int status, const uiPhraseSet& msg )
{
    int idx = statusdef_.indexOf( status );
    if ( !iconnames_.validIdx(idx) )
	return;
    setIcon( iconnames_.get(idx) );
    setToolTip( statusdef_.getUiStringForIndex(idx) );
    setPrefWidth( prefVNrPics() );
    status_ = status;
    msg_ = msg;
}


void uiStatusButton::setMessage( const uiPhraseSet& msg )
{
    msg_ = msg;
}


int uiStatusButton::getValue() const
{
    return status_;
}


void uiStatusButton::showmsgCB( CallBacker* )
{
    if ( msg_.isEmpty() )
	return;

    uiMSG().message( msg_.cat() );
}
