#ifndef uiTable_H
#define uiTable_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.h,v 1.2 2003-03-12 16:23:19 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
#include <uimouse.h>
class PtrUserIDObjectSet;
class uiLabel;
class uiTableBody;

template <class T> class ObjectSet;
class BufferString;


class uiTable : public uiObject
{
friend class		i_tableMessenger;
public:

    typedef Point<int>	Pos;
    typedef Size2D<int>	Size;

    class Setup
    {
    public:

			Setup()
			    : rowgrow_(false)
			    , colgrow_(false)
			    , rowdesc_("Row")
			    , coldesc_("Column")	{}

	Setup& size( const Size& s )		{ size_ = s; return *this; }
	Setup& rowdesc( const char* s )		{ rowdesc_ = s; return *this; }
	Setup& coldesc( const char* s )		{ coldesc_ = s; return *this; }
	Setup& rowcangrow( bool s=true )	{ rowgrow_ = s; return *this; }
	Setup& colcangrow( bool s=true )	{ colgrow_ = s; return *this; }

	Size		size_;
	BufferString	rowdesc_;
	BufferString	coldesc_;
	bool		rowgrow_;
	bool		colgrow_;
    };

                        uiTable(uiParent*, const Setup&,const char* nm="Table");
    virtual 		~uiTable();


    const char*		text(const Pos&) const;
    void		setText(const Pos&,const char*);
    void		clearCell(const Pos&);
    void		setCurrentCell( const Pos& );

    int			nrRows() const;
    void		setNrRows( int nr);
    int			nrCols() const;
    void		setNrCols( int nr);

    Size		size() const	{ return Size( nrCols(), nrRows() ); }
    void		setSize( const Size& s)
			{
			    setNrCols( s.width() );
			    setNrRows( s.height() );
			}


    void		setColumnWidth( int col, int w );
    void		setRowHeight( int row, int h );

    void		setColumnStretchable( int col, bool stretch );
    void		setRowStretchable( int row, bool stretch );
    bool		isColumnStretchable( int col ) const;
    bool		isRowStretchable( int row ) const;

    void		insertRows( int row, int count = 1 );
    void		insertRows( const Pos& p, int count = 1 )
			    { insertRows( p.y(), count ); }
    void		insertColumns( int col, int count = 1 );
    void		insertColumns( const Pos& p, int count = 1 )
			    { insertColumns( p.x(), count ); }
    void		removeRow( int row );
    void		removeRow( const Pos& p )
			    { removeRow( p.y() ); }
    void		removeColumn( int col );
    void		removeColumn( const Pos& p )
			    { removeColumn( p.x() ); }

    void		setRowLabel( int row, const char* label );
    void		setRowLabels( const char** labels );
    void		setRowLabels( const ObjectSet<BufferString>& labels );
    void		setRowLabel( const Pos& p, const char* label )
			    { setRowLabel( p.y(), label ); }

    void		setColumnLabel( int col, const char* label );
    void		setColumnLabels( const char** labels );
    void		setColumnLabels( const ObjectSet<BufferString>& labels);
    void		setColumnLabel( const Pos& p, const char* label )
			    { setColumnLabel( p.x(), label ); }

    Setup&		setup() 		{ return setup_; }
    const Setup&	setup() const		{ return setup_; }

    Pos			notifiedPos() const	{ return notifpos_; }
    Notifier<uiTable>	valueChanged;
    CNotifier<uiTable,const uiMouseEvent&>	clicked;
    Notifier<uiTable>	doubleClicked;

    Pos			newPos() const		{ return newpos_; }
    Notifier<uiTable>	rowInserted;
    Notifier<uiTable>	colInserted;

protected:

    Pos			notifpos_;
    Pos			newpos_;

    mutable Setup	setup_;
    mutable BufferString rettxt_;

    void		clicked_(CallBacker*);
    void		rightClk();

private:

    uiTableBody*	body_;
    uiTableBody&	mkbody(uiParent*, const char*, int, int);

};

#endif
