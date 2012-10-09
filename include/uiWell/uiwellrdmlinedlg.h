#ifndef uiwellrdmlinedlg_h
#define uiwellrdmlinedlg_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Payraudeau
 Date:          October 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"
#include "bufstringset.h"

class MultiID;
class IOObj;
class CtxtIOObj;
class Coord;
class uiCheckBox;
class uiIOObjSel;
class uiTable;
class uiGenInput;
class uiListBox;
class uiToolButton;
class uiPushButton;
class uiWellPartServer;


mClass uiWellSelGrp : public uiGroup
{
public:
    			uiWellSelGrp(uiParent*,bool withpos=true);

    void 		getCoordinates(TypeSet<Coord>&);
    const TypeSet<MultiID>&	getSelWells() const { return selwellsids_; }
    void		setSelectedWells();

protected:

    void		fillListBox();
    
    void		createSelectButtons(uiGroup*);
    void                createMoveButtons(uiGroup*);
    void		createFields();
    void		attachFields(uiGroup*,uiGroup*);
    void		selButPush(CallBacker*);
    void		moveButPush(CallBacker*);
    int			getFirstEmptyRow();
    void		extendLine(TypeSet<Coord>&);
    void		ptsSel(CallBacker*);

    bool		withpos_;

    BufferStringSet	allwellsnames_;
    TypeSet<MultiID>	allwellsids_;
    TypeSet<MultiID>    selwellsids_;
    TypeSet<int>	selwellstypes_;

    uiListBox*		wellsbox_;
    uiTable*		selwellsbox_;
    
    uiGenInput*		onlytopfld_;

    uiToolButton*	toselect_;
    uiToolButton*	fromselect_;

    uiToolButton*	moveupward_;
    uiToolButton*	movedownward_;
};

/*! \brief: setup a dialog where the user can select throught which wells (s)he
  wants to make a random line path.
  Get coordinates from selected wells and send an event to allow the display of
  a fake random line on top of the survey; user can still modify choices, apply
  again and finally close the dlg.

*/

mClass uiWell2RandomLineDlg : public uiDialog
{
public:
    			uiWell2RandomLineDlg(uiParent*,uiWellPartServer*);
    			~uiWell2RandomLineDlg();

    void 		getCoordinates(TypeSet<Coord>&);
    const char*		getRandLineID() const;
    bool		dispOnCreation();

protected:
    
    void		createFields();
    void		attachFields();
    void		previewPush(CallBacker*);
    void		extendLine(TypeSet<Coord>&);
    bool		acceptOK(CallBacker*);

    uiGenInput*		extendfld_;
    uiIOObjSel* 	outfld_;
    uiCheckBox* 	dispfld_;

    CtxtIOObj&		outctio_;

    uiPushButton*	previewbutton_;

    uiWellSelGrp*	selgrp_;
    uiWellPartServer*	wellserv_;
};

#endif
