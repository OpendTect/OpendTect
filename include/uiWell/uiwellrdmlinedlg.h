#ifndef uiwellrdmlinedlg_h
#define uiwellrdmlinedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Payraudeau
 Date:          October 2005
 RCS:           $Id: uiwellrdmlinedlg.h,v 1.1 2005-10-12 12:38:00 cvshelene Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "bufstringset.h"

class MultiID;
class IOObj;
class Coord;
class uiGenInput;
class uiListBox;
class uiToolButton;
class uiPushButton;

/*! \brief: setup a dialog where the user can select throught which wells (s)he
  wants to make a random line path.
  Get coordinates from selected wells and send an event to allow the display of
  a fake random line on top of the survey; user can still modify choices, apply
  again and finally close the dlg.

*/

class uiWell2RandomLineDlg : public uiDialog
{
public:
    			uiWell2RandomLineDlg(uiParent*);

protected:
    void		fillListBox();
    void		setSelectedWells();
    
    const TypeSet<MultiID>&	getSelWells() const { return selwellsids_; }
    void 		getCoordinates(TypeSet<Coord>&);

    void		createSelectButtons(uiGroup*);
    void                createMoveButtons(uiGroup*);
    void		createFields(uiGroup*);
    void		attachFields(uiGroup*,uiGroup*,uiGroup*);
    void		selButPush(CallBacker*);
    void		moveButPush(CallBacker*);
    void		previewPush(CallBacker*);

    BufferStringSet	allwellsnames_;
    TypeSet<MultiID>	allwellsids_;
    TypeSet<MultiID>    selwellsids_;

    uiListBox*		wellsbox_;
    uiListBox*		selectedwellsbox_;
    uiGenInput*		onlytopfld_;

    uiToolButton*	toselect_;
    uiToolButton*	fromselect_;

    uiToolButton*	moveupward_;
    uiToolButton*	movedownward_;
    
    uiPushButton*	previewbutton_;
};

#endif
