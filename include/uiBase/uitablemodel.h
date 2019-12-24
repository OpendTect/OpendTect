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
class QVariant;

mExpClass(uiBase) uiTableModel
{
public:
    mExpClass(uiBase)	CellData
    {
    public:
	CellData();
	CellData(const char*);
	CellData(int);
	CellData(float,int nrdec);
	CellData(double,int nrdec);
	CellData(const CellData&);
	~CellData();

	QVariant*	qvar_;
    };

    enum ItemFlag		{ NoFlags=0, ItemSelectable=1, ItemEditable=2,
				  ItemDragEnabled=4, ItemDropEnabled=8,
				  ItemCheckable=16, ItemEnabled=32 };
    virtual			~uiTableModel();

    virtual int			nrRows() const		= 0;
    virtual int			nrCols() const		= 0;
    virtual int			flags(int row,int col) const	= 0;
    virtual CellData		text(int row,int col) const	= 0;
    virtual Color		textColor(int row,int col) const = 0;
    virtual Color		color(int row,int col) const	= 0;
    virtual uiString		headerText(int rowcol,OD::Orientation) const =0;
    virtual uiString		tooltip(int row,int col) const	= 0;

    ODAbstractTableModel*	getAbstractModel()	{ return odtablemodel_;}
    void			beginReset();
    void			endReset();

protected:
				uiTableModel();

    ODAbstractTableModel*	odtablemodel_;
};


mExpClass(uiBase) uiTableView : public uiObject
{
public:
    enum SelectionBehavior	{ SelectItems, SelectRows, SelectColumns };
    enum SelectionMode		{ SingleSelection=1, ExtendedSelection=3,
				  NoSelection=0 };
    enum CellType		{ Text, NumI, NumF, NumD };

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
    void			setSelectionMode(SelectionMode);

    void			setColumnValueType(int col,CellType);

protected:

    ODTableView&		mkView(uiParent*,const char*);

    uiTableModel*		tablemodel_;
    ODTableView*		odtableview_;
    QSortFilterProxyModel*	qproxymodel_;
};
