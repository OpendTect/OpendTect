#ifndef uiTable_H
#define uiTable_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.h,v 1.1 2003-02-14 15:34:49 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
class PtrUserIDObjectSet;
class uiLabel;
class uiTableBody;

template <class T> class ObjectSet;
class BufferString;


class uiTable : public uiObject
{
friend class		i_tableMessenger;
public:

                        uiTable(uiParent* parnt=0, 
				  const char* nm="uiTable",
				  int nrows=0, int ncols=0);

    virtual 		~uiTable();


    void		setText( int row, int col, const char* );
    void		clearCell( int row, int col );

    const char*		text( int row, int col ) const;

    void		setNumRows(int);
    void		setNumCols(int);
    int			numRows() const;
    int			numCols() const;

    void		setColumnWidth( int col, int w );
    void		setRowHeight( int row, int h );

    void		setColumnStretchable( int col, bool stretch );
    void		setRowStretchable( int row, bool stretch );
    bool		isColumnStretchable( int col ) const;
    bool		isRowStretchable( int row ) const;

    void		insertRows( int row, int count = 1 );
    void		insertColumns( int col, int count = 1 );
    void		removeRow( int row );
    void		removeColumn( int col );

    void		setRowLabels( const char** labels );
    void		setColumnLabels( const char** labels );

    void		setRowLabels( const ObjectSet<BufferString>& labels );
    void		setColumnLabels( const ObjectSet<BufferString>& labels);

    int			lastRow() const { return lastrow_; }
    int			lastCol() const { return lastcol_; }

    Notifier<uiTable>	valueChanged;
    Notifier<uiTable>	clicked;
    Notifier<uiTable>	doubleClicked;

protected:

    int			lastrow_;
    int			lastcol_;

    mutable BufferString rettxt_;

private:

    uiTableBody*	body_;
    uiTableBody&	mkbody(uiParent*, const char*, int, int);

};



#endif
