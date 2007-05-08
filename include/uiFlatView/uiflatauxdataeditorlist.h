#ifndef uiflatauxdataeditorlist_h
#define uiflatauxdataeditorlist_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          April 2007
 RCS:           $Id: uiflatauxdataeditorlist.h,v 1.1 2007-05-08 18:40:04 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiListBox;
class uiButton;
namespace FlatView { class AuxDataEditor; }


/*!A list with all auxdata in a FlatView::AuxDataEditor where the user
   can select which one should be active for new points. */

class uiFlatViewAuxDataEditorList : public uiGroup
{
public:
		uiFlatViewAuxDataEditorList(uiParent*,FlatView::AuxDataEditor*);
		~uiFlatViewAuxDataEditorList();

    void	setEditor(FlatView::AuxDataEditor*);
    void	updateList(CallBacker* = 0);
    void	setSelection(int id);
    		/*!<Set which data in the editor that should be active. */

    NotifierAccess*	addNotifier();
    NotifierAccess*	removeNotifier();

    void		enableAdd(bool yn);

protected:
    void			listSelChangeCB(CallBacker*);

    uiListBox*			listbox_;
    uiButton*			addbutton_;
    uiButton*			removebutton_;

    FlatView::AuxDataEditor*	editor_;
    TypeSet<int>		ids_;
};

#endif
