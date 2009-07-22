#ifndef uiwelldisppropdlg_h
#define uiwelldisppropdlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldisppropdlg.h,v 1.6 2009-07-22 16:01:24 cvsbert Exp $
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
};


#endif
