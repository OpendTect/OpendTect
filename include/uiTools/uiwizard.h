#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class uiGroup;

mExpClass(uiTools) uiWizard : public uiDialog
{ mODTextTranslationClass(uiWizard)
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

    bool		acceptOK(CallBacker*) override;
    bool		rejectOK(CallBacker*) override;

private:

    ObjectSet<uiGroup>	pages;
    BoolTypeSet		dodisplay;

    int			pageidx;
    bool		rotatemode;

    bool		displayCurrentPage();
    void		updateButtonText();
    void		doFinalize(CallBacker*);
};
