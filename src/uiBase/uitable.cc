/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.cc,v 1.1 2003-02-14 15:34:42 arend Exp $
________________________________________________________________________

-*/

#include <uitable.h>
#include <uifont.h>
#include <uidobjset.h>
#include <uilabel.h>
#include <uiobjbody.h>

#include <i_qtable.h>

#include <qsize.h> 
#include <sets.h> 



class uiTableBody : public uiObjBodyImpl<uiTable,QTable>
{

public:

                        uiTableBody(uiTable& handle, 
				  uiParent* parnt=0, 
				  const char* nm="uiTableBody",
				  int nrows=0,
				  int ncols=0)
    : uiObjBodyImpl<uiTable,QTable>( handle, parnt, nm )
    , messenger_ (*new i_tableMessenger(this, &handle))
    {
	setNumRows(nrows);
	setNumCols(ncols);

	setStretch( 2, ( nrTxtLines()== 1) ? 0 : 2 );
	setHSzPol( uiObject::medvar );
    }

    virtual 		~uiTableBody()		{ delete &messenger_; }

    void 		setLines( int prefNrLines )
			{ 
			    setNumRows( prefNrLines - 1 );

			    if( stretch(true) == 2  && stretch(false) != 1 )
				setStretch( 2, ( nrTxtLines()== 1) ? 0 : 2 );
			}

//    virtual uiSize	minimumSize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			    { return numRows() ? numRows()+1 : 7; }

private:

    i_tableMessenger&	messenger_;

};


uiTable::uiTable( uiParent* p, const char* nm, int nr, int nc)
    : uiObject( p, nm, mkbody(p,nm,nr,nc) )
    , valueChanged( this )
    , clicked( this )
    , doubleClicked( this )
    , lastrow_( -1 )
    , lastcol_( -1 )
{}


uiTableBody& uiTable::mkbody( uiParent* p, const char* nm, int nr, int nc)
{
    body_ = new uiTableBody(*this,p,nm,nr,nc);
    return *body_;
}


uiTable::~uiTable() {}



void uiTable::setText( int row, int col, const char* txt )
    { body_->setText( row, col, txt ); }

void uiTable::clearCell( int row, int col )
    { body_->clearCell( row, col ); }

const char* uiTable::text( int row, int col ) const
    { rettxt_ = body_->text( row, col ); return rettxt_; }

void uiTable::setNumRows(int nr)
    { body_->setLines( nr + 1 ); }

void uiTable::setNumCols(int nc)
    { body_->setNumCols( nc ); }

int uiTable::numRows() const
    { return body_->numRows(); }
int uiTable::numCols() const
    { return body_->numCols(); }

void uiTable::setColumnWidth( int col, int w )
    { body_->setColumnWidth( col, w ); }
void uiTable::setRowHeight( int row, int h )
    { body_->setRowHeight( row, h ); }

void uiTable::setColumnStretchable( int col, bool stretch )
    { body_->setColumnStretchable( col, stretch ); }
void uiTable::setRowStretchable( int row, bool stretch )
    { body_->setRowStretchable( row, stretch ); }
bool uiTable::isColumnStretchable( int col ) const
    { return body_->isColumnStretchable(col); }
bool uiTable::isRowStretchable( int row ) const
    { return body_->isRowStretchable(row); }

void uiTable::insertRows( int row, int count )
    { body_->insertRows( row, count ); }
void uiTable::insertColumns( int col, int count)
    { body_->insertColumns( col, count ); }
void uiTable::removeRow( int row )
    { body_->removeRow( row ); }
void uiTable::removeColumn( int col )
    { body_->removeColumn( col ); }

void uiTable::setRowLabels( const char** labels )
{
    QStringList labls;
    const char* pt_cur = *labels;
    while ( pt_cur )
        labls += pt_cur++;

    setNumRows( labls.size() );
    body_->setRowLabels( labls );
}

void uiTable::setColumnLabels( const char** labels )
{
    QStringList labls;
    const char* pt_cur = *labels;
    while ( pt_cur )
        labls += pt_cur++;

    setNumCols( labls.size() );
    body_->setColumnLabels( labls );
}


void uiTable::setRowLabels( const ObjectSet<BufferString>& labels )
{
    setNumRows( labels.size() );

    QStringList labls;
    for ( int idx=0; idx < labels.size(); idx++ )
        labls += (const char*) *labels[idx];

    body_->setRowLabels( labls );
}


void uiTable::setColumnLabels( const ObjectSet<BufferString>& labels )
{
    setNumCols( labels.size() );

    QStringList labls;
    for ( int idx=0; idx < labels.size(); idx++ )
        labls += (const char*) *labels[idx];

    body_->setColumnLabels( labls );
}

/*
uiSize uiTableBody::minimumSize() const
{ 
    int totHeight = fontHgt() * prefnrlines;
    int totWidth  = fontWdt( true ) * fieldWdt;

    return uiSize ( totWidth , totHeight, true );
}
*/

