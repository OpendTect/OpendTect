#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2017
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiMenu;
class uiColorSeqDisp;
class uiColorTableMan;


mExpClass(uiTools) uiColorSeqSel : public uiGroup
{ mODTextTranslationClass(uiColorSeqSel);
public:

				uiColorSeqSel(uiParent*,
					uiString lbl=uiString::emptyString());
				~uiColorSeqSel();

    const char*			seqName() const;
    void			setSeqName(const char*);
    bool			isFlipped() const;
    void			setFlipped(bool);

    Notifier<uiColorSeqSel>	seqChanged;
    Notifier<uiColorSeqSel>	menuReq;	//!< only when !usingBasicMenu()

    bool			usingBasicMenu() const	{ return usebasicmenu_;}
    void			setUseBasicMenu( bool yn )
				{ usebasicmenu_ = yn; }
    uiMenu*			getBasicMenu();	//!< starting point

    void			setCurrentAsDefault();
    void			showManageDlg();

protected:

    uiColorSeqDisp*		disp_;
    uiColorTableMan*		mandlg_;
    bool			usebasicmenu_;

    void			initDisp(CallBacker*);
    void			selectCB(CallBacker*);
    void			menuCB(CallBacker*);
    void			upCB(CallBacker*)	{ nextColTab(true); }
    void			downCB(CallBacker*)	{ nextColTab(false); }
    void			setAsDefaultCB(CallBacker*);
    void			manageCB(CallBacker*);

    void			nextColTab(bool prev);

};
