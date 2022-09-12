#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "rowcol.h"

class TableModel;
class ODTableView;
class ODStyledItemDelegate;
class QByteArray;
class QSortFilterProxyModel;
class QVariant;


mExpClass(uiBase) uiTableView : public uiObject
{
    friend class		i_tableViewMessenger;
public:
    enum SelectionBehavior	{ SelectItems, SelectRows, SelectColumns };
    enum SelectionMode		{ SingleSelection=1, ExtendedSelection=3,
				  NoSelection=0 };
    enum CellType		{ Bool, Text, NumI, NumF,
				  NumD, Color, Date, Enum, Other };

				uiTableView(uiParent*,const char* nm);
				~uiTableView();

    void			setModel(TableModel*);
    void			saveHorizontalHeaderState();
    void			resetHorizontalHeader();

    void			setSectionsMovable(bool);
    void			setSortingEnabled(bool);
    bool			isSortingEnabled() const;
    void			sortByColumn(int col,bool asc=true);
    void			setRowHidden(int row,bool);
    bool			isRowHidden(int row) const;
    void			setColumnHidden(int col,bool);
    bool			isColumnHidden(int col) const;
    void			setNrFrozenColumns(int nrcols);

    RowCol			mapFromSource(const RowCol&) const;
				// source model to filter model
    RowCol			mapToSource(const RowCol&) const;
				// filter model to source model
    void			setSelectionBehavior(SelectionBehavior);
    void			setSelectionMode(SelectionMode);
    void			clearSelection();
    bool			getSelectedRows(TypeSet<int>&) const;
    bool			getSelectedColumns(TypeSet<int>&) const;
    bool			getSelectedCells(TypeSet<RowCol>&) const;
    void			selectAll();
    void			setSelectedCells(const TypeSet<RowCol>&);
    void			removeSelection(const TypeSet<RowCol>&);
    const RowCol&		currentCell() const;
    void			setCurrentCell(const RowCol&);

    void			setColumnValueType(int col,CellType);
    void			setColumnWidth(int col,int width );

    CellType			getCellType(int col) const;

    Notifier<uiTableView>	doubleClicked;

protected:

    ODTableView&		mkView(uiParent*,const char*);
    ODStyledItemDelegate*	getColumnDelegate(int col,CellType);

    TableModel*			tablemodel_	= nullptr;
    ODTableView*		odtableview_;
    QSortFilterProxyModel*	qproxymodel_	= nullptr;
    QByteArray*			horizontalheaderstate_	= nullptr;

    ObjectSet<ODStyledItemDelegate>	columndelegates_;

    virtual ODStyledItemDelegate*	createColumnDelegate(int col,CellType);

    void			doubleClickedCB(CallBacker*);
};
