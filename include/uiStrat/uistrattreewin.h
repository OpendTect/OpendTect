#ifndef uistratwin_h
#define uistratwin_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          July 2007
 RCS:           $Id: uistrattreewin.h,v 1.9 2007-09-05 15:31:52 cvshelene Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"

class uiLabeledListBox;
class uiListViewItem;
class uiMenuItem;
class uiStratLevelDlg;
class uiStratRefTree;
namespace Strat { class RefTree; class Level; }


/*!\brief Main window for Stratigraphy display: holds the reference tree
  and the units description view */

class uiStratTreeWin : public uiMainWin
{
public:

			uiStratTreeWin(uiParent*);
			~uiStratTreeWin();

protected:

    uiStratRefTree*		uitree_;
    uiLabeledListBox*	 	lvllistfld_;
    uiMenuItem*			expandmnuitem_;
    uiMenuItem*			editmnuitem_;
    uiMenuItem*			savemnuitem_;
    uiMenuItem*			saveasmnuitem_;
    uiMenuItem*			openmnuitem_;
    uiMenuItem*			resetmnuitem_;

    void			createMenus();
    void			createGroups();

    void			fillLvlList();
    void			prepTreeForSave();
    void			updateTreeLevels();
    BufferString		getCodeFromLVIt(const uiListViewItem*) const;
    void			fillTmpTreeFromListview();
    void			editLevel(bool);
    void			fillInLvlPars(Strat::Level*,
	    				      const uiStratLevelDlg&,bool);

    void			editCB(CallBacker*);
    void			openCB(CallBacker*);
    void			resetCB(CallBacker*);
    void			saveCB(CallBacker*);
    void			saveAsCB(CallBacker*);
    void			selLvlChgCB(CallBacker*);
    void			rClickLvlCB(CallBacker*);
    void			setExpCB(CallBacker*);
    void			unitSelCB(CallBacker*);

    Strat::RefTree*		tmptree_;
};


#endif
