#ifndef uiwizard_h
#define uiwizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.h,v 1.3 2004-08-17 11:20:58 nanne Exp $
________________________________________________________________________

-*/


#include "uidialog.h"

class uiGroup;

class uiWizard : public uiDialog
{
public:
			uiWizard(uiParent*,uiDialog::Setup&);

    int			addPage(uiGroup*,bool disp=true);
    void		displayPage(int,bool yn=true);
    void		setRotateMode(bool);

    void		setCurrentPage(int);
    int			currentPageIdx() const		{ return pageidx; }

    void		approvePage(bool yn)		{ approved=yn; }

    int			firstPage() const;
    int			lastPage() const;
    int			nrPages() const			{ return pages.size(); }

    Notifier<uiWizard>	next;
    Notifier<uiWizard>	back;
    Notifier<uiWizard>	finish;
    Notifier<uiWizard>	cancel;

protected:

    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);

private:

    ObjectSet<uiGroup>	pages;
    BoolTypeSet		dodisplay;

    int			pageidx;
    bool		approved;
    bool		rotatemode;
    
    void		displayCurrentPage();
    void		handleButtonText();
    void		doFinalise(CallBacker*);
};

#endif
