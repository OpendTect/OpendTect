#ifndef uiTable_H
#define uiTable_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.h,v 1.7 2003-04-23 12:33:49 arend Exp $
________________________________________________________________________

-*/

#include <uigroup.h>
#include <uimouse.h>
class PtrUserIDObjectSet;
class uiLabel;
class uiTableBody;
class UserInputObj;

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
			    : size_( -1, -1 )
			    , rowgrow_(false)
			    , colgrow_(false)
			    , rowdesc_("Row")
			    , coldesc_("Column")
			    , fillrow_(true)
			    , fillcol_(false)
			    , minrowhgt_( 1 )
			    , maxrowhgt_( 3 )
			    , mincolwdt_( uiObject::baseFldSize() )
			    , maxcolwdt_( 2.3 * uiObject::baseFldSize() )

	{}

	Setup& size( const Size& s )		{ size_ = s; return *this; }
	Setup& rowdesc( const char* s )		{ rowdesc_ = s; return *this; }
	Setup& coldesc( const char* s )		{ coldesc_ = s; return *this; }
	Setup& rowcangrow( bool s=true )	{ rowgrow_ = s; return *this; }
	Setup& colcangrow( bool s=true )	{ colgrow_ = s; return *this; }
	Setup& fillrow( bool s=true )		{ fillrow_ = s; return *this; }
	Setup& fillcol( bool s=true )		{ fillcol_ = s; return *this; }
	Setup& maxrowhgt( float s )		{ maxrowhgt_= s; return *this; }
	Setup& minrowhgt( float s )		{ minrowhgt_= s; return *this; }
	Setup& maxcolwdt( float s )		{ maxcolwdt_= s; return *this; }
	Setup& mincolwdt( float s )		{ mincolwdt_= s; return *this; }

	Size		size_;
	BufferString	rowdesc_;
	BufferString	coldesc_;
	bool		rowgrow_;
	bool		colgrow_;
	bool		fillrow_; //!< grow cell heights to fill up avail space
	bool		fillcol_; //!< grow cell widths to fill up avail space
	float		minrowhgt_; //!< expressed in multiples of font height
	float		maxrowhgt_; //!< expressed in multiples of font height
	float		mincolwdt_; //!< times average font width
	float		maxcolwdt_; //!< times average font width
    };

                        uiTable(uiParent*, const Setup&,const char* nm="Table");
    virtual 		~uiTable();


    const char*		text(const Pos&) const;
    void		setText(const Pos&,const char*);
    void		clearCell(const Pos&);
    void		setCurrentCell( const Pos& );


    int			nrRows() const;
    int			nrCols() const;
    void		setNrRows( int nr);
    void		setNrCols( int nr);

    Size		size() const	{ return Size( nrCols(), nrRows() ); }
    void		setSize( const Size& s)
			{
			    setNrCols( s.width() );
			    setNrRows( s.height() );
			}

    int			columnWidth( int col ) const;
    int			rowHeight( int row ) const;
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

    const char*		rowLabel(int) const;
    const char*		rowLabel( const Pos& p ) const
			    { return rowLabel(p.y()); }
    void		setRowLabel( int row, const char* label );
    void		setRowLabels( const char** labels );
    void		setRowLabels( const ObjectSet<BufferString>& labels );
    void		setRowLabel( const Pos& p, const char* label )
			    { setRowLabel( p.y(), label ); }

    const char*		columnLabel(int) const;
    const char*		columnLabel( const Pos& p ) const
			    { return columnLabel(p.x()); }
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

    UserInputObj*	mkUsrInputObj(const Pos&);
    void		delUsrInputObj(const Pos&);
    UserInputObj*	usrInputObj(const Pos&);
    const UserInputObj*	usrInputObj(const Pos& p ) const
			  { return const_cast<uiTable*>(this)->usrInputObj(p); }

    int			getIntValue( const Pos& p ) const;
    void		setValue( const Pos& p, int i );

protected:

    Pos			notifpos_;
    Pos			newpos_;

    mutable Setup	setup_;
    mutable BufferString rettxt_;

    void		clicked_(CallBacker*);
    void		rightClk();

    void		geometrySet_(CallBacker*);
    void		updateCellSizes(uiSize* sz=0);


private:

    uiTableBody*	body_;
    uiTableBody&	mkbody(uiParent*, const char*, int, int);

    uiSize		lastsz;

};

#endif
