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

class uiLabeledComboBox;
class uiPushButton;
class uiTabStack;
class uiWellDispProperties;

namespace Well { class Data; class DisplayProperties; };
namespace WellCorr { class uiWellPropDlg; }


/*!
\brief Well display properties dialog box.
*/

mExpClass(uiWell) uiWellDispPropDlg : public uiDialog
{mODTextTranslationClass(uiWellDispPropDlg)
public:
    mDeprecatedDef		uiWellDispPropDlg(uiParent*,Well::Data*,
						  bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,const MultiID&,
						  bool is2ddisplay,
						  Color bgcolor);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg> applyAllReq; //Deprecated
    Notifier<uiWellDispPropDlg>& saveReq();
    Notifier<uiWellDispPropDlg>& applyTabReq();
    Notifier<uiWellDispPropDlg>& resetAllReq();

    enum TabType { LeftLog=0, CenterLog=1, RightLog=2, Marker=3, Track=4 };

    Well::Data*			wellData()		{ return wd_; }
    const Well::Data*		wellData() const	{ return wd_; }

    TabType			currentTab() const;
    bool			is2D() const		{ return is2ddisplay_; }
    bool			savedefault_;	//Deprecated
    mDeprecatedDef void		updateLogs();
    bool			needsSave() const;
    void			setNeedsSave(bool yn);

protected:
				uiWellDispPropDlg(uiParent*,Well::Data*,
						  bool is2ddisplay,
						  Color bgcolor);

    Well::Data*			wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;
    Color&			backGroundColor();
    const Color&		backGroundColor() const;

    void			initDlg(Color);
    virtual void		getFromScreen();
    virtual void		putToScreen();

    virtual void		setWDNotifiers(bool yn);

    void			postFinaliseCB(CallBacker*);

    virtual void		applyAllPush(CallBacker*);
    virtual void		onClose(CallBacker*);
    virtual void		propChg(CallBacker*);
    void			markersChgd(CallBacker*);
    void			logsChgd(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			wdChg(CallBacker*);
    void			welldataDelNotify(CallBacker*);
    void			tabSel(CallBacker*);

    uiPushButton*		applyTabButton() const;
    void			applyTabPush(CallBacker*);
    uiPushButton*		resetAllButton() const;
    void			resetAllPush(CallBacker*);
    void			saveAsDefaultCB(CallBacker*);
				//As we cannot override the virtual fn:
    uiButton*			saveButton() const;
    void			acceptOKCB(CallBacker*);
    void			resetCB(CallBacker*);

    friend class WellCorr::uiWellPropDlg;

};


/*!
\brief Multi Well display properties dialog box.
*/

mExpClass(uiWell) uiMultiWellDispPropDlg : public uiWellDispPropDlg
{mODTextTranslationClass(uiMultiWellDispPropDlg)
public:
	mDeprecatedDef		uiMultiWellDispPropDlg(uiParent*,
						ObjectSet<Well::Data>&,
						bool is2ddisplay);
				uiMultiWellDispPropDlg(uiParent*,
						const ObjectSet<Well::Data>&,
						bool is2ddisplay,
						Color bgcolor);
				~uiMultiWellDispPropDlg();

protected:

    ObjectSet<Well::Data>	wds_;
    uiLabeledComboBox*		wellselfld_;

    mDeprecatedDef void		resetProps(int logidx);
    void			resetProps(int wellidx,int logidx);
    virtual void		wellSelChg(CallBacker*);
    virtual void		setWDNotifiers(bool yn);
    void			onClose(CallBacker*);
    void			applyMWTabPush(CallBacker*);
    void			resetMWAllPush(CallBacker*);

    friend class uiWellDispPropDlg;

};


