#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Wayne Mogg
 Date:		April 2022
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uibutton.h"

/*!\brief Toolbutton to display status */

class EnumDef;

mExpClass(uiTools) uiStatusButton : public uiPushButton
{ mODTextTranslationClass(uiStatusButton)
public:
    enum Status { Unknown, OK, Error };
    mDeclareEnumUtils(Status)

			uiStatusButton(uiParent*, const EnumDef&,
				       const char**, int defenum=0);
			~uiStatusButton();

    void		setValue(int, const uiPhraseSet& msg=uiPhraseSet());
    void		setMessage(const uiPhraseSet&);
    int			getValue() const;
protected:
    int			status_;
    uiPhraseSet		msg_;

    const EnumDef&	statusdef_;
    BufferStringSet	iconnames_;

    void		showmsgCB(CallBacker*);

};
