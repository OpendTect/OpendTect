#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2006
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

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
			~uiCompoundParSel();

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

    virtual uiString		getSummary() const = 0;
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
				 const uiString& buttxt=uiString::empty());

    void		setChecked(bool);
    bool		isChecked() const;

    Notifier<uiCheckedCompoundParSel> checked;
    void		setSummary(const uiString& smmry)
						{ summary_ = smmry; return; }

protected:

    uiCheckBox*		cbox_;
    bool		mkinvis_;
    uiString		summary_;

    void		checkCB(CallBacker*);

    virtual uiString	getSummary() const { return summary_; }

};
