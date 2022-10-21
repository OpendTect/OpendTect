#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "uigroup.h"
#include "uistrings.h"
#include "odcommonenums.h"

class uiGenInput;
class uiButton;
class uiCheckBox;


/*!\brief Single-line element allowing multi-parameter to be set via a dialog.

  Most useful for options that are not often actually changed by user.
  After the button push trigger, the summary is displayed in the text field.

 */

mExpClass(uiTools) uiCompoundParSel : public uiGroup
{
public:
    virtual		~uiCompoundParSel();

    void		setSelText(const uiString&);
    void		setSelIcon(const char*);
    void		updateSummary()			{ updSummary(0); }

    Notifier<uiCompoundParSel>	butPush;

protected:
			uiCompoundParSel(uiParent*,const uiString& seltxt,
					 OD::StdActionType t=OD::Select);
			uiCompoundParSel(uiParent*,const uiString& seltxt,
					 const uiString& buttxt,
					 const char* icid=0);

    virtual BufferString	getSummary() const= 0;
    void			doSel(CallBacker*);
    void			updSummary(CallBacker*);

    uiGenInput*			txtfld_;
    uiButton*			selbut_;

private:

    void			crTextFld(const uiString&);
    void			finishCreation(const uiString&,const uiString&);

};


/*!\brief CompoundParSel providing something that is optional

  Basically, a CompoundParSel with a check box.
 */

mExpClass(uiTools) uiCheckedCompoundParSel : public uiCompoundParSel
{
public:
			~uiCheckedCompoundParSel();

    void		setChecked(bool);
    bool		isChecked() const;

    Notifier<uiCheckedCompoundParSel> checked;

protected:
			uiCheckedCompoundParSel(uiParent*,
				 const uiString& seltxt,
				 bool mkinvisible, // if not, makes insensitive
				 const uiString& buttxt=
				 uiStrings::sEmptyString());

    uiCheckBox*		cbox_;
    bool		mkinvis_;

    void		checkCB(CallBacker*);
};
