#ifndef uiwizard_h
#define uiwizard_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.h,v 1.10 2012-08-03 13:01:16 cvskris Exp $
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uidialog.h"

class uiGroup;

mClass(uiTools) uiWizard : public uiDialog
{
public:
			uiWizard(uiParent*,uiDialog::Setup&);

    int			addPage(uiGroup*,bool disp=true);
    void		displayPage(int,bool yn=true);
    void		setRotateMode(bool);

    void		setCurrentPage(int);
    int			currentPageIdx() const		{ return pageidx; }

    int			firstPage() const;
    int			lastPage() const;
    int			nrPages() const			{ return pages.size(); }

protected:

    virtual bool	preparePage(int) { return true; }
    virtual bool	leavePage(int, bool next ) { return true; }
    virtual void	isStarting() {}
    virtual bool	isClosing(bool iscancel ) { return true; }
    virtual void	reset() {}
    			/*!<Is called when the wizartd starts again. */

    bool		acceptOK(CallBacker*);
    bool		rejectOK(CallBacker*);

private:

    ObjectSet<uiGroup>	pages;
    BoolTypeSet		dodisplay;

    int			pageidx;
    bool		rotatemode;
    
    bool		displayCurrentPage();
    void		updateButtonText();
    void		doFinalise(CallBacker*);
};

#endif

