#ifndef uiwelldisppropdlg_h
#define uiwelldisppropdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldisppropdlg.h,v 1.19 2012/06/19 08:46:17 cvsbruno Exp $
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
				uiWellDispPropDlg(uiParent*,Well::Data*,
						    bool is2ddisplay=false);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg>	applyAllReq;

    Well::Data*			wellData()		{ return wd_; }
    const Well::Data*		wellData() const	{ return wd_; }

    bool 			savedefault_;
 
protected:

    Well::Data*			wd_;
    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;
    bool			is2ddisplay_;

    virtual void		getFromScreen();
    virtual void		putToScreen();

    virtual void		setWDNotifiers(bool yn);

    virtual void		applyAllPush(CallBacker*);
    virtual void		propChg(CallBacker*);
    bool			rejectOK(CallBacker*);
    void			wdChg(CallBacker*);
    void			welldataDelNotify(CallBacker*);

public:
    void			disableWDNotifiers()
				{ setWDNotifiers( false ); }
};


mClass uiMultiWellDispPropDlg : public uiWellDispPropDlg
{
public:
				uiMultiWellDispPropDlg(uiParent*,
						ObjectSet<Well::Data>&,
						bool is2ddisplay);
protected:

    ObjectSet<Well::Data> 	wds_;
    uiLabeledComboBox*		wellselfld_;

    void			resetProps(int logidx);
    virtual void 		wellSelChg(CallBacker*);
    virtual void		setWDNotifiers(bool yn);
};


#endif
