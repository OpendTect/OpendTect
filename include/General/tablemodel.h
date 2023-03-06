#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "color.h"
#include "odcommonenums.h"
#include "pixmapdesc.h"


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

	CellData&	operator=(const CellData&);

	QVariant&	qvar_;
    };

    enum ItemFlag		{ NoFlags=0, ItemSelectable=1, ItemEditable=2,
				  ItemDragEnabled=4, ItemDropEnabled=8,
				  ItemIsUserCheckable=16, ItemEnabled=32 };

    enum CellType		{ Bool, Text, NumI, NumF,
				  NumD, Color, Date, Enum, Other };

    virtual			~TableModel();

    virtual int			nrRows() const				= 0;
    virtual int			nrCols() const				= 0;
    virtual int			flags(int row,int col) const		= 0;
    virtual void		setCellData(int row,int col,const CellData&) =0;
    virtual CellData		getCellData(int row,int col) const	= 0;

    virtual OD::Color		textColor(int row,int col) const	= 0;
    virtual OD::Color		cellColor(int row,int col) const	= 0;
    virtual PixmapDesc		pixmap(int row,int col) const		= 0;
    virtual void		setChecked(int row,int col,int val)	{}
    virtual int			isChecked(int row,int col) const  { return -1; }
    virtual uiString		headerText(int rowcol,OD::Orientation) const =0;
    virtual uiString		tooltip(int row,int col) const		= 0;
    virtual const EnumDef*	getEnumDef(int col) const
				{ return nullptr; }

    virtual CellType		getColumnCellType(int col) const;

    QAbstractTableModel*	getAbstractModel();
    void			beginReset();
    void			endReset();

protected:
				TableModel();

    ODAbstractTableModel*	odtablemodel_;
};
