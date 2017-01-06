#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          May 2006
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uistrings.h"

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

			uiCompoundParSel(uiParent*,const uiString& seltxt,
					 OD::StdActionType t=OD::Select);
			uiCompoundParSel(uiParent*,const uiString& seltxt,
					 const uiString& buttxt,
					 const char* icid=0);


    void		setSelText(const uiString&);
    void		setSelIcon(const char*);
    void		updateSummary()			{ updSummary(0); }

    Notifier<uiCompoundParSel>	butPush;

protected:

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

			uiCheckedCompoundParSel(uiParent*,
				 const uiString& seltxt,
				 bool mkinvisible, // if not, makes insensitive
				 const uiString& buttxt=
				 uiStrings::sEmptyString());

    void		setChecked(bool);
    bool		isChecked() const;

    Notifier<uiCheckedCompoundParSel> checked;
    void		setSummary(const BufferString& smmry)
						{ summary_ = smmry; return; }

protected:

    uiCheckBox*		cbox_;
    bool		mkinvis_;
    BufferString	summary_;

    void		checkCB(CallBacker*);

    virtual BufferString getSummary() const { return summary_; }

};
