#ifndef uiwelldisppropdlg_h
#define uiwelldisppropdlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldisppropdlg.h,v 1.1 2008-12-05 15:20:05 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiTabStack;
class uiWellDispProperties;

namespace Well { class Data; class DisplayProperties; };


/*! \brief Dialog for well display properties. */

class uiWellDispPropDlg : public uiDialog
{
public:
				uiWellDispPropDlg(uiParent*,Well::Data&);
				~uiWellDispPropDlg();

protected:

    Well::Data&			wd_;
    Well::DisplayProperties&	props_;
    Well::DisplayProperties*	orgprops_;

    uiTabStack*			ts_;
    ObjectSet<uiWellDispProperties> propflds_;

    void			getFromScreen();
    void			putToScreen();

    void			applyPush(CallBacker*);
    bool			rejectOK(CallBacker*);
    bool			acceptOK(CallBacker*);
};


#endif
