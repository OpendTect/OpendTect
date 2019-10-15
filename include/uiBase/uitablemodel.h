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

class ODAbstractTableModel;
class ODTableView;

mExpClass(uiBase) uiTableModel
{
public:
    virtual			~uiTableModel();

    virtual int			nrRows() const		= 0;
    virtual int			nrCols() const		= 0;
    virtual BufferString	text(int row,int col) const	= 0;
    virtual uiString		headerText(int rowcol,OD::Orientation) const =0;

    ODAbstractTableModel*	getAbstractModel()	{ return odtablemodel_;}

protected:
				uiTableModel();

    ODAbstractTableModel*	odtablemodel_;
};


mExpClass(uiBase) uiTableView : public uiObject
{
public:
				uiTableView(uiParent*,const char* nm);
				~uiTableView();

    void			setModel(uiTableModel*);

protected:

    ODTableView&		mkView(uiParent*,const char*);

    uiTableModel*		tablemodel_;
    ODTableView*		odtableview_;
};
