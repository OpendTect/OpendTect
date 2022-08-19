#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
