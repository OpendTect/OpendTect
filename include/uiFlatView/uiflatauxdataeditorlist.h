#ifndef uiflatauxdataeditorlist_h
#define uiflatauxdataeditorlist_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          June 2007
 RCS:           $Id: uiflatauxdataeditorlist.h,v 1.4 2007-07-11 21:08:43 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "uimenuhandler.h"

class MenuHandler;
class uiListBox;

namespace FlatView { class AuxDataEditor; }


/*!A list with all auxdata in a FlatView::AuxDataEditor where the user
   can select which one should be active for new points. */

class uiFlatViewAuxDataEditorList : public uiGroup
{
public:
			uiFlatViewAuxDataEditorList(uiParent*);
			~uiFlatViewAuxDataEditorList();

    void		addEditor(FlatView::AuxDataEditor*);
    			/*!<Does not take over ownership. */
    void		removeEditor(FlatView::AuxDataEditor*);

    void		updateList(CallBacker* = 0);
    void		setSelection(const FlatView::AuxDataEditor*,int id);
    			//!<Set which data in the editor that should be active. 

    NotifierAccess&	selectionChange() { return change_; }
    void		getSelections(ObjectSet<FlatView::AuxDataEditor>&,
	    			      TypeSet<int>& ids );

    MenuHandler&	menuhandler() { return *uimenuhandler_; }

protected:

    void		rightClickedCB(CallBacker*);
    virtual void	listSelChangeCB(CallBacker*);
    int			findEditorIDPair( const FlatView::AuxDataEditor*,
					  int id) const;

    ObjectSet<FlatView::AuxDataEditor>		editors_;

    TypeSet<int>				listboxids_;    
    						//coupled, 1 per row in the list
    ObjectSet<FlatView::AuxDataEditor>		listboxeditors_;
    						//coupled, 1 per row in the list

    uiListBox*					listbox_;
    uiMenuHandler*				uimenuhandler_;

    Notifier<uiFlatViewAuxDataEditorList>	change_;
};

#endif
