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
class uiMultiWellDispPropDlg;

namespace Well { class Data; class DisplayProperties; };


/*!
\brief Well display properties dialog box.
*/

mExpClass(uiWell) uiWellDispPropDlg : public uiDialog
{mODTextTranslationClass(uiWellDispPropDlg)
public:
				uiWellDispPropDlg(uiParent*,Well::Data*,Color,
						  bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,const MultiID&,
						  Color,bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,Well::Data*,
						  bool is2ddisplay=false);
				uiWellDispPropDlg(uiParent*,const MultiID&,
						  bool is2ddisplay=false);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg>	applyAllReq;
    Notifier<uiWellDispPropDlg>& applyTabReq();
    Notifier<uiWellDispPropDlg>& resetAllReq();

    enum TabType { LeftLog=0, CenterLog=1, RightLog=2, Marker=3, Track=4 };
    Well::Data*			wellData()		{ return wd_; }
    const Well::Data*		wellData() const	{ return wd_; }

    TabType			currentTab() const;
    bool			is2D() const;
    bool 			savedefault_;
    void			updateLogs();

protected:

    Well::Data*			wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;

    Color&	    backGroundColor();
    const Color&	backGroundColor() const;

    virtual void		getFromScreen();
    virtual void		putToScreen();

    virtual void		setWDNotifiers(bool yn);

    virtual void		applyAllPush(CallBacker*);
    void		applyTabPush(CallBacker*);
    void		resetAllPush(CallBacker*);
    virtual void		onClose(CallBacker*);
    virtual void		propChg(CallBacker*);
    void			markersChgd(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			resetCB(CallBacker*);
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
						ObjectSet<Well::Data>&,
						bool is2ddisplay);
				~uiMultiWellDispPropDlg();

protected:

    ObjectSet<Well::Data> 	wds_;
    uiLabeledComboBox*		wellselfld_;

    mDeprecatedDef void			resetProps(int logidx);
    void			resetProps(int wellidx,int logidx);
    virtual void 		wellSelChg(CallBacker*);
    virtual void		setWDNotifiers(bool yn);
    void			onClose(CallBacker*);
    void		applyMWTabPush(CallBacker*);
    void		resetMWAllPush(CallBacker*);

    friend class uiWellDispPropDlg;
};


