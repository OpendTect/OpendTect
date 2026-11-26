#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uitableview.h"
#include "odcommonenums.h"
#include "typeset.h"

class uiCheckSelGrp;
class uiGenInput;
class TableModel;


/*!\brief uiGroup that shows a TableModel in a uiTableView.

  The model is not owned. It must outlive this group. Any TableModel can
  be passed, including StringTableModel<T>. For a StringTableModel, T
  fills one StringTableContents. Call setBaseData before the model is
  given to the group.

  Clicking a row, or the cell in its first column, selects that row.
  Setup::multiselection_ chooses one row or several rows. Setup::checkboxes_
  adds a checkbox on the first column, and only applies when several rows
  can be selected. With checkboxes, clicking a row selects that one row.
  Checked rows are the multi-selection. When the choice mode is
  ChooseAtLeastOne and no row is checked, the highlighted row is the
  selection, as in uiListBox. selectionChanged is triggered for a new
  highlight and for a check change. The returned row numbers follow
  the model. The filter hides other rows on screen
  and leaves the model unchanged. It is a case-insensitive glob:
  * and ? are wildcards, and the text is matched anywhere in a cell.

  The filter field is in the same group as the table. The table's right
  edge lines up with the filter, so the filter sits at the right of the
  row above the table. A button attached to the right of the filter
  stays on that row. When checkboxes are on, the check-selection strip
  (menu, check-all, open and save) sits at the left of that row.
  offerReadWriteSelection sets what the open and save buttons do.

  view() is the screen table. Its row numbers follow the screen.

*/

mExpClass(uiTools) uiStringTableViewGrp : public uiGroup
{ mODTextTranslationClass(uiStringTableViewGrp);
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup();
			~Setup();

	mDefSetupMembInit(bool,sorting,false)
	mDefSetupMembInit(bool,alternatingrowcolors,false)
	mDefSetupMembInit(bool,showverticalheader,true)
	mDefSetupMembInit(bool,multiselection,false)
	mDefSetupMembInit(bool,checkboxes,false)
	mDefSetupMembInit(OD::ChoiceMode,choicemode,OD::ChooseOnlyOne)
	mDefSetupMembInit(bool,compactrows,false)
    };

			uiStringTableViewGrp(uiParent*,TableModel*,
					     const Setup&,
					     const char* nm=nullptr);
			~uiStringTableViewGrp();
			mOD_DisableCopy(uiStringTableViewGrp)

    uiTableView&	view()			{ return *view_; }
    const uiTableView&	view() const		{ return *view_; }
    TableModel*		model()			{ return model_; }
    const TableModel*	model() const		{ return model_; }

    uiGroup&		filterGroup()		{ return *filtergrp_; }
    const uiGroup&	filterGroup() const	{ return *filtergrp_; }
    uiGenInput&		filterField()		{ return *filterfld_; }
    const uiGenInput&	filterField() const	{ return *filterfld_; }
    uiObject*		selectionGroupObj();

    int			selectedRow() const;
    void		getSelectedRows(TypeSet<int>&) const;
    bool		hasCheckBoxes() const;
    bool		isRowChecked(int row) const;
    void		setRowChecked(int row,bool yn);
    void		getCheckedRows(TypeSet<int>&) const;
    void		setCheckedRows(const TypeSet<int>&);
    void		offerReadWriteSelection(const CallBack& readcb,
						const CallBack& writecb);

    int			nrChosen() const;
    bool		isChosen(int row) const;
    void		setChosen(int row,bool yn=true);

    Notifier<uiStringTableViewGrp>	selectionChanged;

private:

    void		filterCB(CallBacker*);
    void		finalizeCB(CallBacker*);
    void		readSelCB(CallBacker*);
    void		saveSelCB(CallBacker*);
    void		selectionChangedCB(CallBacker*);
    void		checkStateCB(CallBacker*);
    void		updateSelButtons();
    void		menuCB(CallBacker*);
    void		aboutToShowCheckMenuCB(CallBacker*);
    void		chooseAllReqCB(CallBacker*);
    void		invertReqCB(CallBacker*);
    void		showCheckedOnlyReqCB(CallBacker*);

    uiGroup*		filtergrp_;
    uiTableView*	view_;
    TableModel*		model_;
    const Setup		setup_;
    uiGenInput*		filterfld_;
    uiCheckSelGrp*	checkselgrp_		= nullptr;
    CallBack		opencb_;
    CallBack		savecb_;
};
