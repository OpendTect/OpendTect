/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.cc,v 1.4 2003-03-12 16:22:25 arend Exp $
________________________________________________________________________

-*/

#include <uitable.h>
#include <uifont.h>
#include <uidobjset.h>
#include <uilabel.h>
#include <uiobjbody.h>
#include <uimenu.h>


#include <qsize.h> 
#include <sets.h> 
#include <i_qtable.h>


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
	setLines( nrows + 1 );
	setNumCols(ncols);

	setStretch( 2, ( nrTxtLines()== 1) ? 0 : 2 );
	setHSzPol( uiObject::medvar );
    }

    virtual 		~uiTableBody()		{ delete &messenger_; }

    void 		setLines( int prefNrLines )
			{ 
			    setNumRows( prefNrLines - 1 );
			    if ( prefNrLines > 1 )
			    {
				const int rowh = rowHeight( 0 );
				const int prefh = rowh * (prefNrLines-1) + 30;
				setPrefHeight( mMIN(prefh,200) );
			    }

			    if( stretch(true) == 2  && stretch(false) != 1 )
				setStretch( 2, ( nrTxtLines()== 1) ? 0 : 2 );
			}

//    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			    { return numRows() ? numRows()+1 : 7; }

void setRowLabels( const QStringList &labels )
{

    QHeader* leftHeader = verticalHeader();

    int i = 0;
    for ( QStringList::ConstIterator it = labels.begin();
          it != labels.end() && i < numRows(); ++i, ++it )
        leftHeader->setLabel( i, *it );
}


private:

    i_tableMessenger&	messenger_;

};


uiTable::uiTable( uiParent* p, const Setup& s, const char* nm )
    : uiObject( p, nm, mkbody(p,nm,s.size_.height(),s.size_.width()) )
    , setup_( s )
    , valueChanged( this )
    , clicked( this )
    , doubleClicked( this )
    , rowInserted( this )
    , colInserted( this )
{
    clicked.notify( mCB(this,uiTable,clicked_) );
}


uiTableBody& uiTable::mkbody( uiParent* p, const char* nm, int nr, int nc)
{
    body_ = new uiTableBody(*this,p,nm,nr,nc);
    return *body_;
}


uiTable::~uiTable() {}


void uiTable::setText( const Pos& pos, const char* txt )
    { body_->setText( pos.y(), pos.x(), txt ); }

void uiTable::clearCell( const Pos& pos )
    { body_->clearCell( pos.y(), pos.x() ); }

void uiTable::setCurrentCell( const Pos& pos )
    { body_->setCurrentCell( pos.y(), pos.x() ); }

const char* uiTable::text( const Pos& pos ) const
    { rettxt_ = body_->text( pos.y(), pos.x() ); return rettxt_; }

int uiTable::nrRows() const
    { return  body_->numRows(); }
void uiTable::setNrRows( int nr )
    { body_->setLines( nr + 1 ); }

int uiTable::nrCols() const
    { return body_->numCols(); }
void uiTable::setNrCols( int nr )
    { body_->setNumCols( nr ); }


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


void uiTable::setRowLabel( int row, const char* label )
{
    QHeader* topHeader = body_->verticalHeader();
    topHeader->setLabel( row, label );
}


void uiTable::setRowLabels( const char** labels )
{
    const char* pt_cur = *labels;

    int nlabels=0;
    while ( pt_cur ) { nlabels++; pt_cur++; }
    body_->setLines( nlabels + 1 );

    pt_cur = *labels;
    int idx=0;
    while ( pt_cur ) 
	setRowLabel( idx++, pt_cur++ );
}


void uiTable::setRowLabels( const ObjectSet<BufferString>& labels )
{
    body_->setLines( labels.size() + 1 );

    for ( int i=0; i<labels.size(); i++ )
        setRowLabel( i, *labels[i] );
}


void uiTable::setColumnLabel( int col, const char* label )
{
    QHeader* topHeader = body_->horizontalHeader();
    topHeader->setLabel( col, label );
}


void uiTable::setColumnLabels( const char** labels )
{
    const char* pt_cur = *labels;

    int nlabels=0;
    while ( pt_cur ) { nlabels++; pt_cur++; }
    body_->setNumCols( nlabels );

    pt_cur = *labels;
    int idx=0;
    while ( pt_cur ) 
	setColumnLabel( idx++, pt_cur++ );
}


void uiTable::setColumnLabels( const ObjectSet<BufferString>& labels )
{
    body_->setNumCols( labels.size() );

    for ( int i=0; i<labels.size(); i++ )
        setColumnLabel( i, *labels[i] );
}


void uiTable::clicked_( CallBacker* cb )
{
    mCBCapsuleUnpack(const uiMouseEvent&,ev,cb);

    if( ev.buttonState() & uiMouseEvent::RightButton )
	rightClk();
}


void uiTable::rightClk()
{
    if ( !setup_.rowgrow_  && !setup_.colgrow_  )
	return;

    uiPopupMenu* mnu = new uiPopupMenu( parent(), "Action" );
    BufferString itmtxt;

    int inscolbef = 0;
    int delcol = 0;
    int inscolaft = 0;
    if ( setup_.colgrow_ )
    {
	itmtxt = "Insert "; itmtxt += setup_.coldesc_; itmtxt += " before";
	inscolbef = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Remove "; itmtxt += setup_.coldesc_;
	delcol = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Insert "; itmtxt += setup_.coldesc_; itmtxt += " after";
	inscolaft = mnu->insertItem( new uiMenuItem( itmtxt ) );
    }

    int insrowbef = 0;
    int delrow = 0;
    int insrowaft = 0;
    if ( setup_.rowgrow_ )
    {
	itmtxt = "Insert "; itmtxt += setup_.rowdesc_; itmtxt += " before";
	insrowbef = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Remove "; itmtxt += setup_.rowdesc_;
	delrow = mnu->insertItem( new uiMenuItem( itmtxt ) );
	itmtxt = "Insert "; itmtxt += setup_.rowdesc_; itmtxt += " after";
	insrowaft = mnu->insertItem( new uiMenuItem( itmtxt ) );
    }

    int ret = mnu->exec();
    if ( !ret ) return;

    Pos cur = notifiedPos();

    if( ret == inscolbef || ret == inscolaft )
    {
	const int offset = (ret == inscolbef) ? 0 : 1;
	newpos_ = Pos( cur.x() + offset, cur.y() );
	insertColumns( newpos_ );

	BufferString label( newpos_.x() );
	setColumnLabel( newpos_, label );

	colInserted.trigger();
    }
    else if ( ret == delcol )
    {
	removeColumn( cur.x() );
    }
    else if ( ret == insrowbef || ret == insrowaft  )
    {
	const int offset = (ret == insrowbef) ? 0 : 1;
	newpos_ = Pos( cur.x(), cur.y() + offset );
	insertRows( newpos_ );

	BufferString label( newpos_.y() );
	setRowLabel( newpos_, label );

	rowInserted.trigger();
    }
    else if ( ret == delrow )
    {
	removeRow( cur.y() );
    }

    setCurrentCell( newpos_ );
}
