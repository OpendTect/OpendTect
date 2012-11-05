#ifndef tablecommands_h
#define tablecommands_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Jaap Glas
 Date:          February 2009
 RCS:           $Id$
 ________________________________________________________________________

-*/

#include "uicmddrivermod.h"
#include "command.h"
#include "cmdcomposer.h"
#include "rowcol.h"
#include "uitable.h"


namespace CmdDrive
{

mStartDeclCmdClassNoActNoEntry( uiCmdDriver,Table, UiObjectCmd )
protected:
    enum		TableTag { RowTag=0,RowHead,CellTag,ColHead,ColTag };

    bool		parTableSelPre(const char* prefix,TableTag,
	    			const uiTable*,const BufferString& itemstr,
				int itemnr,TypeSet<RowCol>& itemrcs,
				bool ambicheck);

    RowCol		singleSelected(const uiTable*) const;
    bool		isSelected(const uiTable*,const RowCol&) const;

mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, TableClick, TableCmd )	mEndDeclCmdClass

mClass(uiCmdDriver) TableActivator: public Activator
{
public:
    			TableActivator(const uiTable&,const RowCol&,
				       const BufferStringSet& clicktags);
    void		actCB(CallBacker*);
protected:
    uiTable&		acttable_;
    RowCol		actrc_;

    BufferStringSet	actclicktags_;
};


mStartDeclCmdClass( uiCmdDriver, TableFill, TableCmd )	mEndDeclCmdClass

mClass(uiCmdDriver) TableFillActivator: public Activator
{
public:
    			TableFillActivator(const uiTable&,const RowCol&,
					   const char* txt);
    void		actCB(CallBacker*);
protected:
    uiTable&		acttable_;
    RowCol		actrc_;
    BufferString	acttxt_;
};


mStartDeclCmdClass( uiCmdDriver, TableSelect, TableCmd )	mEndDeclCmdClass

mClass(uiCmdDriver) TableSelectActivator: public Activator
{
public:
    			TableSelectActivator(const uiTable&,
					     const TypeSet<RowCol>&);
    void		actCB(CallBacker*);
protected:
    uiTable&		acttable_;

    const TypeSet<RowCol>& actselset_;		// Only access before trigger!
};


mStartDeclCmdClassNoActNoEntry( uiCmdDriver,TableQuestion, TableCmd )
    virtual bool	isUiObjChangeCommand() const	{ return false; }
    virtual bool	isVisualCommand() const		{ return false; }
mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, TableExec, TableQuestionCmd )	mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, TableMenu, TableCmd )		mEndDeclCmdClass


mStartDeclCmdClass( uiCmdDriver, NrTableRows, TableQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrTableCols, TableQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTableItemOn,TableQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTableRow, TableQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTableCol, TableQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, CurTableItem, TableQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableRow, TableQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableCol, TableQuestionCmd )mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableItem, TableQuestionCmd)mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, NrTableMenuItems, TableQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, IsTableMenuItemOn, TableQuestionCmd )
    mEndDeclCmdClass
mStartDeclCmdClass( uiCmdDriver, GetTableMenuItem, TableQuestionCmd )
    mEndDeclCmdClass


/* Ordered encoded set of table RowCols for which some boolean state
   (selected, checked, etc.) is true. Order is ascending with column
   as major and row as minor key. Two consecutive RowCols (row0,-step)
   and (row1,col) encode a series of rows in column col from row0 to
   row1 with a positive step step.
*/
mClass(uiCmdDriver) TableState
{
public:
    			TableState(const uiTable* uitable=0)
			    : table_( uitable )			{};

    bool		headInsert(const RowCol&);
    int			remove(const RowCol&,int startidx=0);
    int			indexOf(const RowCol&,int startidx=0) const;

    bool		equalToCurItemSel() const;
    bool		equalToClickedItem(const RowCol&) const;

    bool		storeCurItemSel(); 

    void		setTable(const uiTable* uit)	{ table_ = uit; }
    void		clear()				{ set_.erase(); }
    bool		setAll();		

    const TypeSet<RowCol>& getSet() const		{ return set_; }

protected:
    const uiTable*	table_;
    TypeSet<RowCol>	set_;
};


mStartDeclComposerClassWithInit( uiCmdDriver, Table, CmdComposer, uiTable )
public:
    virtual void	updateInternalState();
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

    TableState		selectedcells_;
    TableState		wasselectedcells_;
    TableState		isselectedcells_;

mEndDeclComposerClass


}; // namespace CmdDrive

#endif

