#ifndef uiwizard_h
#define uiwizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.h,v 1.1 2004-03-22 16:15:55 nanne Exp $
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

    void		setCurrentPage(int);
    int			currentPageIdx() const		{ return pageidx; }

    int			firstPage() const;
    int			lastPage() const;

    Notifier<uiWizard>	nextpage;
    Notifier<uiWizard>	prevpage;
    Notifier<uiWizard>	finished;
    Notifier<uiWizard>	cancelled;

protected:

    ObjectSet<uiGroup>	pages;
    BoolTypeSet		dodisplay;

    int			pageidx;
    void		displayCurrentPage();
    void		handleButtonText();

    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);
};

#endif
