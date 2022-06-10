#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2019
________________________________________________________________________

-*/

#include "generalmod.h"
#include "odcommonenums.h"
#include "rowcol.h"
#include "color.h"


class QAbstractTableModel;
class ODAbstractTableModel;
class QByteArray;
class QSortFilterProxyModel;
class QVariant;

mExpClass(General) TableModel
{
public:
    mExpClass(General) CellData
    {
    public:
	CellData();
	CellData(const QVariant&);
	CellData(const QString&);
	CellData(const char*);
	CellData(bool);
	CellData(int);
	CellData(float,int nrdec);
	CellData(double,int nrdec);
	CellData(const CellData&);
	~CellData();

	bool		getBoolValue() const;
	const char*	text() const;
	float		getFValue() const;
	double		getDValue() const;
	int		getIntValue() const;

	QVariant&	qvar_;
    };

    enum ItemFlag		{ NoFlags=0, ItemSelectable=1, ItemEditable=2,
				  ItemDragEnabled=4, ItemDropEnabled=8,
				  ItemIsUserCheckable=16, ItemEnabled=32 };
    virtual			~TableModel();

    virtual int			nrRows() const				= 0;
    virtual int			nrCols() const				= 0;
    virtual int			flags(int row,int col) const		= 0;
    virtual void		setCellData(int row,int col,const CellData&) =0;
    virtual CellData		getCellData(int row,int col) const	= 0;

    virtual OD::Color		textColor(int row,int col) const	= 0;
    virtual OD::Color		cellColor(int row,int col) const	= 0;
    virtual BufferStringSet	pixmap(int row,int col) const		= 0;
    virtual void		setChecked(int row,int col,int val)	{}
    virtual int			isChecked(int row,int col) const  { return -1; }
    virtual uiString		headerText(int rowcol,OD::Orientation) const =0;
    virtual uiString		tooltip(int row,int col) const		= 0;
    virtual const EnumDef*	getEnumDef(int col) const
				{ return nullptr; }

    QAbstractTableModel*	getAbstractModel();
    void			beginReset();
    void			endReset();

protected:
				TableModel();

    ODAbstractTableModel*	odtablemodel_;
};
