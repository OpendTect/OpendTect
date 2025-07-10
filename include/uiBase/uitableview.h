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
#include "tablemodel.h"

class ODTableView;
class ODStyledItemDelegate;
class QByteArray;
class QSortFilterProxyModel;


mExpClass(uiBase) uiTableView : public uiObject
{
    friend class		i_tableViewMessenger;
public:
    enum SelectionBehavior	{ SelectItems, SelectRows, SelectColumns };
    enum SelectionMode		{ SingleSelection=1, ExtendedSelection=3,
				  NoSelection=0 };

				uiTableView(uiParent*,const char* nm);
				~uiTableView();
				mOD_DisableCopy(uiTableView)

    void			setModel(TableModel*);
    const TableModel*		getModel() const	{ return tablemodel_; }
    TableModel*			getModel()		{ return tablemodel_; }
    void			saveHorizontalHeaderState();
    void			resetHorizontalHeader();

    void			setNrFrozenColumns(int nrcols);
    void			setContextMenuEnabled(bool);

    void			setColumnStretchable(int,bool);
    void			resizeColumnsToContents();
    void			resizeColumnToContents(int column);
    void			setRowHeight(int row,int height);

    void			setSectionsMovable(bool);
    void			moveColumn(int from,int to);

    void			setSortingEnabled(bool);
    bool			isSortingEnabled() const;
    void			sortByColumn(int col,bool asc=true);

    void			setRowHidden(int row,bool);
    bool			isRowHidden(int row) const;
    void			getVisibleRows(TypeSet<int>&,
					       bool mappedtosource) const;
    void			setColumnHidden(int col,bool);
    bool			isColumnHidden(int col) const;
    void			getVisibleColumns(TypeSet<int>&,
					       bool mappedtosource) const;
    void			setHeaderVisible(OD::Orientation,bool yn);
    bool			isHeaderVisible(OD::Orientation) const;

    RowCol			mapFromSource(const RowCol&) const;
				// source model to filter model
    RowCol			mapToSource(const RowCol&) const;
				// filter model to source model

    void			setSelectionBehavior(SelectionBehavior);
    SelectionBehavior		getSelectionBehavior() const;
    void			setSelectionMode(SelectionMode);
    SelectionMode		getSelectionMode() const;
    void			clearSelection();
    int				maxNrOfSelections() const;

    bool			getSelectedRows(TypeSet<int>&) const;
    bool			getSelectedColumns(TypeSet<int>&) const;
    bool			getSelectedCells(TypeSet<RowCol>&,
						 bool mappedtosource) const;
    void			selectAll();
    void			setSelectedCells(const TypeSet<RowCol>&);
    void			setSelectedCells(const TypeSet<RowCol>&,
						 bool mapfromsource);
    void			setCellSelected(const RowCol&,bool yn,
						bool mapfromsource=true);
    bool			isCellSelected(const RowCol&,
					       bool mapfromsource=true) const;
    void			selectColumn(int col);
    void			selectRow(int row);
    void			removeSelection(const TypeSet<RowCol>&);

    void			setCurrentCell(const RowCol&,
					       bool noselection=false);
    const RowCol&		currentCell() const;

    void			setColumnValueType(int col,
						   TableModel::CellType,
						   char format='g',
						   int precision=6);
    void			setColumnWidth(int col,int width );

    TableModel::CellType	getCellType(int col) const;

    Notifier<uiTableView>			doubleClicked;
    Notifier<uiTableView>			rightClicked;
    Notifier<uiTableView>			selectionChanged;
    CNotifier<uiTableView,int>			columnClicked;
    CNotifier<uiTableView,int>			rowClicked;

protected:

    ODTableView&		mkView(uiParent*,const char*);
    ODStyledItemDelegate*	getColumnDelegate(int col,TableModel::CellType,
						  char format='g',
						  int precision=6);

    TableModel*			tablemodel_	= nullptr;
    ODTableView*		odtableview_;
    QSortFilterProxyModel*	qproxymodel_	= nullptr;
    QByteArray*			horizontalheaderstate_	= nullptr;

    ObjectSet<ODStyledItemDelegate>	columndelegates_;

    virtual ODStyledItemDelegate* createColumnDelegate(int col,
						       TableModel::CellType,
						       char format='g',
						       int precision=6);
};
