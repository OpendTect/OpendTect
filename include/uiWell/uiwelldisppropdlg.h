#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiwellmod.h"
#include "uidialog.h"
#include "ptrman.h"

class uiTabStack;
class uiWellDispProperties;
class uiLabeledComboBox;

namespace Well { class Data; class DisplayProperties; };

/*!
\brief Well display properties dialog box.
*/

mExpClass(uiWell) uiWellDispPropDlg : public uiDialog
{mODTextTranslationClass(uiWellDispPropDlg)
public:
				uiWellDispPropDlg(uiParent*,
					const Well::Data&,
					OD::Color,bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,const MultiID&,
					OD::Color,bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,
					const Well::Data&,
					bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,const MultiID&,
						  bool is2ddisplay=false);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg>	applyAllReq;
    Notifier<uiWellDispPropDlg> applyTabReq;
    Notifier<uiWellDispPropDlg> resetAllReq;

    enum TabType { LeftLog=0, CenterLog=1, RightLog=2, Marker=3, Track=4 };
    Well::Data*			wellData()		{ return wd_; }
    const Well::Data*		wellData() const	{ return wd_; }

    TabType			currentTab() const;
    bool			is2D() const;
    void			updateLogs();

protected:

    RefMan<Well::Data>		wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;
    OD::Color			bkcol_;

    virtual void		getFromScreen();
    virtual void		putToScreen();

    virtual void		setWDNotifiers(bool yn);

    virtual void		applyAllPush(CallBacker*);
    virtual void		applyTabPush(CallBacker*);
    virtual void		resetAllPush(CallBacker*);
    virtual void		propChg(CallBacker*);
    void			markersChgd(CallBacker*);
    bool			acceptOK(CallBacker*);
    virtual void		resetCB(CallBacker*);
    void			wdChg(CallBacker*);
    void			welldataDelNotify(CallBacker*);
    void			tabSel(CallBacker*);
    void			init();
};


/*!
\brief Multi Well display properties dialog box.
*/

mExpClass(uiWell) uiMultiWellDispPropDlg : public uiWellDispPropDlg
{mODTextTranslationClass(uiMultiWellDispPropDlg)
public:
				uiMultiWellDispPropDlg(uiParent*,
						const ObjectSet<Well::Data>&,
						bool is2ddisplay);
				~uiMultiWellDispPropDlg();

protected:

    ObjectSet<Well::Data>	wds_;
    uiLabeledComboBox*		wellselfld_ = nullptr;

    void			resetProps(int,int);
    virtual void 		wellSelChg(CallBacker*);
    virtual void		setWDNotifiers(bool yn);
    void			onClose(CallBacker*);
    virtual void		applyTabPush(CallBacker*);
    virtual void		resetAllPush(CallBacker*);
};

