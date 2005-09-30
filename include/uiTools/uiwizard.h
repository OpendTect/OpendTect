#ifndef uiwizard_h
#define uiwizard_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          March 2004
 RCS:           $Id: uiwizard.h,v 1.7 2005-09-30 14:12:58 cvskris Exp $
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
