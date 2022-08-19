#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiflatviewmod.h"
#include "uigroup.h"
#include "uimenuhandler.h"

class MenuHandler;
class uiListBox;

namespace FlatView { class AuxDataEditor; }

/*!
\brief A list with all auxdata in a FlatView::AuxDataEditor where the user can
select which one should be active for new points.
*/

mExpClass(uiFlatView) uiFlatViewAuxDataEditorList : public uiGroup
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

    bool		isRectangleSelection() const;
    void		useRectangleSelection(bool yn);

    NotifierAccess&	selectionChange() { return change_; }
    void		getSelections(ObjectSet<FlatView::AuxDataEditor>&,
	    			      TypeSet<int>& ids );

    MenuHandler&	menuhandler() { return *uimenuhandler_; }

    NotifierAccess&	pointSelectionChanged() { return ptselchange_; }
    			/*!<Triggers when the polygonselectiontool has been
			    used in one of the editors */

protected:

    void		pointSelectionChangedCB(CallBacker*);
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
    Notifier<uiFlatViewAuxDataEditorList>	ptselchange_;

    bool					isrectangleselection_;
};
