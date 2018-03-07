#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Dec 2001
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uimainwin.h"
#include "uitoolbutton.h"
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

    void		setInfo(const char*,const uiString& newcaption=
						     uiString::empty());

protected:

    BufferString	info_;
    uiString		caption_;
    bool		insens_;

    uiOfferInfoWin*	infowin_;

    void		updateWin();

    void		infoReq(CallBacker*);
    void		winClose(CallBacker*);

};
