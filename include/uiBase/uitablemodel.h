#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          October 2019
________________________________________________________________________

-*/


#include "uibasemod.h"
#include "uiobj.h"
#include "rowcol.h"

class ODAbstractTableModel;
class ODTableView;
class QSortFilterProxyModel;

mExpClass(uiBase) uiTableModel
{
public:
    virtual			~uiTableModel();

    virtual int			nrRows() const		= 0;
    virtual int			nrCols() const		= 0;
    virtual BufferString	text(int row,int col) const	= 0;
    virtual Color		color(int row,int col) const	= 0;
    virtual uiString		headerText(int rowcol,OD::Orientation) const =0;

    ODAbstractTableModel*	getAbstractModel()	{ return odtablemodel_;}

protected:
				uiTableModel();

    ODAbstractTableModel*	odtablemodel_;
};


mExpClass(uiBase) uiTableView : public uiObject
{
public:
    enum SelectionBehavior	{ SelectItems, SelectRows, SelectColumns };

				uiTableView(uiParent*,const char* nm);
				~uiTableView();

    void			setModel(uiTableModel*);

    void			setSortingEnabled(bool);
    bool			isSortingEnabled() const;
    void			sortByColumn(int col,bool asc=true);
    void			setRowHidden(int row,bool);
    bool			isRowHidden(int row) const;
    void			setColumnHidden(int col,bool);
    bool			iscolumnHidden(int col) const;

    RowCol			mapFromSource(const RowCol&) const;
				// source model to filter model
    RowCol			mapToSource(const RowCol&) const;
				// filter model to source model
    void			setSelectionBehavior(SelectionBehavior);

protected:

    ODTableView&		mkView(uiParent*,const char*);

    uiTableModel*		tablemodel_;
    ODTableView*		odtableview_;
    QSortFilterProxyModel*	qproxymodel_;
};
