#ifndef uiwelldisppropdlg_h
#define uiwelldisppropdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldisppropdlg.h,v 1.4 2009-01-08 09:16:21 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiTabStack;
class uiWellDispProperties;

namespace Well { class Data; class DisplayProperties; };


/*! \brief Dialog for well display properties. */

mClass uiWellDispPropDlg : public uiDialog
{
public:
				uiWellDispPropDlg(uiParent*,Well::Data&);
				~uiWellDispPropDlg();

    Notifier<uiWellDispPropDlg>	applyAllReq;

    Well::Data&			wellData()		{ return wd_; }
    const Well::Data&		wellData() const	{ return wd_; }

    bool 			savedefault_;
 
protected:

    Well::Data&			wd_;
    Well::DisplayProperties&	props_;
    Well::DisplayProperties*	orgprops_;

    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;


    void			getFromScreen();
    void			putToScreen();

    void			propChg(CallBacker*);
    void			wdChg(CallBacker*);
    void			applyAllPush(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
};


#endif
