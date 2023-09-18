#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uimainwin.h"
#include "uitoolbutton.h"
#include "uistring.h"
class uiTextBrowser;


/*!\brief Non-modal window able to display local, handy info for the user.

  The text to display can be rich text, i.e. what uiTextBrowser accepts, see
  QTextBrowser.

  The window will manage itself (i.e. is DeleteOnClose).

 */

mExpClass(uiTools) uiOfferInfoWin : public uiMainWin
{ mODTextTranslationClass(uiOfferInfoWin);
public:
			uiOfferInfoWin(uiParent*,const uiString& captn,
					int initialnrlines=5);
			~uiOfferInfoWin();

    void		setText(const char*);

    uiTextBrowser*	uitb_;

};


/*!\brief Tool button with the 'i' popping up a uiOfferInfoWin if pushed.

  The toolbutton will be made insensitive or invisible if the info is empty.
  Setting the info to an empty string will remove any open info window.

 */


mExpClass(uiTools) uiOfferInfo : public uiToolButton
{ mODTextTranslationClass(uiOfferInfo);
public:
			uiOfferInfo(uiParent*,bool setinsens=true);
			~uiOfferInfo();

    void		setInfo(const char*,const uiString& newcaption=
				uiStrings::sEmptyString());

protected:

    BufferString	info_;
    uiString		caption_;
    bool		insens_;

    uiOfferInfoWin*	infowin_				= nullptr;

    void		updateWin();

    void		infoReq(CallBacker*);
    void		winClose(CallBacker*);
};
