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
class TableModelEditRequest;

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
	CellData(float);
	CellData(double);
	CellData(const CellData&);
	~CellData();

	bool		getBoolValue() const;
	const char*	text() const;
	float		getFValue() const;
	double		getDValue() const;
	int		getIntValue() const;
	void		setDate(const char*);
	void		setISODateTime(const char*);

	CellData&	operator=(const CellData&);
	bool		operator==(const CellData&) const;
	bool		operator!=(const CellData&) const;

	QVariant&	qvar_;
    };

    enum ItemFlag		{ NoFlags=0, ItemSelectable=1, ItemEditable=2,
				  ItemDragEnabled=4, ItemDropEnabled=8,
				  ItemIsUserCheckable=16, ItemEnabled=32 };

    enum CellType		{ Bool, Text, NumI, NumF,
				  NumD, Color, Date, DateTime, Enum, Other };

    virtual			~TableModel();
				mOD_DisableCopy(TableModel)

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
    virtual char		getColumnFormatSpecifier(int col) const;
    virtual int			getColumnPrecision(int col) const;

    QAbstractTableModel*	getAbstractModel();
    void			beginReset();
    void			endReset();

    bool			collectEditRequests(
					    const TableModelEditRequest&,
					    TypeSet<TableModelEditRequest>&);
    bool			applyEditRequest(const TableModelEditRequest&,
					    bool useoldval);

    CNotifier<CallBacker,const TableModelEditRequest&>& editRequested();

protected:

				TableModel();

    void			rowBulkDataChanged(int row,
					    const TypeSet<int>& changedcols);

    ODAbstractTableModel*	odtablemodel_;
};


mExpClass(General) TableModelEditRequest
{
public:
			TableModelEditRequest(int row,int col,
				  const TableModel::CellData& oldval,
				  const TableModel::CellData& newval,int role);
    bool		operator==(const TableModelEditRequest&) const;
    bool		operator!=(const TableModelEditRequest&) const;

    int			    row_		= -1;
    int			    col_		= -1;
    TableModel::CellData    oldval_;
    TableModel::CellData    newval_;
    int			    role_		= 0;
    mutable bool	    handled_		= false;
};
