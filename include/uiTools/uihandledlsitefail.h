#ifndef uihandledlsitefail_h
#define uihandledlsitefail_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2011
 RCS:           $Id: uihandledlsitefail.h,v 1.2 2012-07-18 09:04:34 cvsranojay Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstring.h"
class ODDLSite;
class BufferStringSet;
class uiComboBox;
class uiPushButton;
class uiSlider;


/*!Presents Handles a download site failure. Presents options to user.

  If the user cancels (i.e. if ( !dlg.go() )), then the user wants to give up.
  If you have set isfatal, that means that the program will be exited!

  The list_of_sites should be alternatives (which may contain the ODDLSite).

 */


mClass uiHandleDLSiteFail : public uiDialog
{
public:

    			uiHandleDLSiteFail(uiParent*,const ODDLSite&,
				bool isfatal,
				const BufferStringSet* list_of_sites=0);

    const char*		site() const		{ return site_; }
    float		timeout() const;

protected:

    BufferString	site_;
    const bool		isfatal_;

    uiComboBox*		dlsitefld_;
    uiPushButton*	proxybut_;
    uiSlider*		timeoutfld_;

    void		proxyButCB(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

};


#endif
