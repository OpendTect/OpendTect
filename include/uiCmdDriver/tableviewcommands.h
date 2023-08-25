#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"
#include "rowcol.h"
#include "uitableview.h"



namespace CmdDrive
{

mStartDeclCmdClassNoActNoEntry( uiCmdDriver,TableView, UiObjectCmd )
protected:
    enum		TableViewTag { RowTag=0,RowHead,CellTag,ColHead,ColTag };

    bool		parTableSelPre(const char* prefix,TableViewTag,
				const uiTableView*,const BufferString& itemstr,
				int itemnr,TypeSet<RowCol>& itemrcs,
				bool ambicheck);

    RowCol		singleSelected(const uiTableView*) const;
    bool		isSelected(const uiTableView*,const RowCol&) const;

mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, TableViewClick, TableViewCmd ) mEndDeclCmdClass

mExpClass(uiCmdDriver) TableViewActivator: public Activator
{
public:
			TableViewActivator(const uiTableView&,const RowCol&,
				       const BufferStringSet& clicktags);
    void		actCB(CallBacker*) override;
protected:
    uiTableView&	acttable_;
    RowCol		actrc_;

    BufferStringSet	actclicktags_;
};


mStartDeclCmdClass( uiCmdDriver, TableViewFill, TableViewCmd )	mEndDeclCmdClass

mExpClass(uiCmdDriver) TableViewFillActivator: public Activator
{
public:
			TableViewFillActivator(const uiTableView&,const RowCol&,
					   const char* txt);
    void		actCB(CallBacker*) override;
protected:
    uiTableView&	acttable_;
    RowCol		actrc_;
    BufferString	acttxt_;
};


mStartDeclCmdClass( uiCmdDriver, TableViewSelect, TableViewCmd )	mEndDeclCmdClass

mExpClass(uiCmdDriver) TableViewSelectActivator: public Activator
{
public:
			TableViewSelectActivator(const uiTableView&,
					     const TypeSet<RowCol>&);
    void		actCB(CallBacker*) override;
protected:
    uiTableView&	acttable_;

    const TypeSet<RowCol>& actselset_;		// Only access before trigger!
};


mStartDeclCmdClassNoActNoEntry( uiCmdDriver,TableViewQuestion, TableViewCmd )
    bool	isUiObjChangeCommand() const override	{ return false; }
    bool	isVisualCommand() const override	{ return false; }
mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, TableViewExec, TableViewQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, TableViewMenu, TableViewCmd )		mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, NrTableViewRows, TableViewQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrTableViewCols, TableViewQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTableViewItemOn,TableViewQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTableViewRow, TableViewQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTableViewCol, TableViewQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTableViewItem, TableViewQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableViewRow, TableViewQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableViewCol, TableViewQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableViewItem, TableViewQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrTableViewMenuItems, TableViewQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTableViewMenuItemOn, TableViewQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableViewMenuItem, TableViewQuestionCmd )
    mEndDeclCmdClass


/* Ordered encoded set of table RowCols for which some boolean state
   (selected, checked, etc.) is true. Order is ascending with column
   as major and row as minor key. Two consecutive RowCols (row0,-step)
   and (row1,col) encode a series of rows in column col from row0 to
   row1 with a positive step step.
*/
mExpClass(uiCmdDriver) TableViewState
{
public:
			TableViewState(const uiTableView* uitable=0)
			    : table_( uitable )			{};

    bool		headInsert(const RowCol&);
    int			remove(const RowCol&,int startidx=0);
    int			indexOf(const RowCol&,int startidx=0) const;

    bool		equalToCurItemSel() const;
    bool		equalToClickedItem(const RowCol&) const;

    bool		storeCurItemSel(); 

    void		setTableView(const uiTableView* uit)	{ table_ = uit; }
    void		clear()				{ set_.erase(); }
    bool		setAll();		

    const TypeSet<RowCol>& getSet() const		{ return set_; }

protected:
    const uiTableView*	table_;
    TypeSet<RowCol>	set_;
};


mStartDeclComposerClassWithInit( uiCmdDriver, TableView, CmdComposer, uiTableView )
public:
    void		updateInternalState() override;
    static void		getExecPrefix(CmdRecEvent&,const RowCol&);

protected:
    void		reInit();
    void		storeTableState();
    void		labelStoredStateOld();
    void		labelStoredStateNew();

    void		writeTableSelect();
    int			writeTableSelect(bool differential,
					 bool virtually=false);
    void		writeTableSelect(const RowCol& firstrc,
					 const RowCol& lastrc,
					 int blockstate,bool clear);
    void		writeTableFill();
    void		writeTableMenu(const CmdRecEvent&);
    void		writeTableClick();

    int			stagenr_;
    RowCol		clickedrc_;
    bool		leftclicked_;
    bool		ctrlclicked_;
    bool		selchanged_;
    bool		tablecmdsflushed_;

    TableViewState		selectedcells_;
    TableViewState		wasselectedcells_;
    TableViewState		isselectedcells_;

mEndDeclComposerClass


} // namespace CmdDrive
