#ifndef uiwelldisppropdlg_h
#define uiwelldisppropdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldisppropdlg.h,v 1.12 2010-04-23 10:02:13 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiTabStack;
class uiWellDispProperties;
class uiLabeledComboBox;

namespace Well { class Data; class DisplayProperties; class LogSet; };


/*! \brief Dialog for well display properties. */

mClass uiWellDispPropDlg : public uiDialog
{
public:
				uiWellDispPropDlg(uiParent*,Well::Data*);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg>	applyAllReq;

    Well::Data*			wellData()		{ return wd_; }
    const Well::Data*		wellData() const	{ return wd_; }

    bool 			savedefault_;
 
protected:

    Well::Data*			wd_;
    Well::DisplayProperties&	props_;

    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;

    void			getFromScreen();
    void			putToScreen();

    virtual void		applyAllPush(CallBacker*);
    virtual void		propChg(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			wdChg(CallBacker*);
    void			welldataDelNotify(CallBacker*);
};


mClass uiMultiWellDispPropDlg : public uiWellDispPropDlg
{
public:
				uiMultiWellDispPropDlg(uiParent*,
						ObjectSet<Well::Data>);
				~uiMultiWellDispPropDlg(){}


    Notifier<uiMultiWellDispPropDlg>	wdChged;

protected:

    ObjectSet<Well::Data> 	wds_;
    uiLabeledComboBox*		wellselfld_;

    void			resetProps(Well::DisplayProperties&,
					    const Well::LogSet*);
    virtual void 		wellSelChg(CallBacker*);
};


#endif
