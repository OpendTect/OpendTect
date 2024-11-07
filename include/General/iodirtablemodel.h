#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "tablemodel.h"


class IODir;
class IODirEntryList;
class IOObjContext;


/*\brief TableModel of IOObj objects for a selected context

*/


mExpClass(General) IODirTableModel : public TableModel
{
public:
			IODirTableModel(const IOObjContext&);
			~IODirTableModel();

    void		setFilter(const char*);

protected:
    int			nrRows() const override;
    int			nrCols() const override;
    int			flags(int row,int col) const override;
    void		setCellData(int row,int col,const CellData&) override;
    CellData		getCellData(int row,int col) const override;

    OD::Color		textColor(int row,int col) const override;
    OD::Color		cellColor(int row,int col) const override;
    PixmapDesc		pixmap(int row,int col) const override;
    void		setChecked(int row,int col,int val) override;
    int			isChecked(int row,int col) const override;
    uiString		headerText(int rowcol,OD::Orientation) const override;
    uiString		tooltip(int row,int col) const override;
    const EnumDef*	getEnumDef(int col) const override;
    CellType		getColumnCellType(int col) const override;

    IODirEntryList*	iodirentrylist_;

private:
    IODir*		iodir_;
};
