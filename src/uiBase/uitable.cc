/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.cc,v 1.7 2003-04-01 10:13:51 arend Exp $
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

#include <basictypes.h>
#include <uicombobox.h>


class Input
{
public:
			Input( UserInputObj* o, QWidget* w )
			    : obj( o ), widg( w )	{}
			~Input() 			{ delete obj; }

    UserInputObj*	obj;
    QWidget*		widg;
}; 


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

    }

    virtual 		~uiTableBody()
			    { deepErase(inputs); delete &messenger_; }

    void 		setLines( int prefNrLines )
			{ 
			    setNumRows( prefNrLines - 1 );
			    if ( prefNrLines > 1 )
			    {
				const int rowh = rowHeight( 0 );
				const int prefh = rowh * (prefNrLines-1) + 30;
				setPrefHeight( mMIN(prefh,200) );
			    }

			    int hs = stretch(true);
			    if( stretch(false) != 1 )
				setStretch( hs, ( nrTxtLines()== 1) ? 0 : 2 );
			}

//    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			    { return numRows() ? numRows()+1 : 7; }

    void		setRowLabels( const QStringList &labels )
			{

			    QHeader* leftHeader = verticalHeader();

			    int i = 0;
			    for( QStringList::ConstIterator it = labels.begin();
				  it != labels.end() && i < numRows(); ++i,++it
			       )
				leftHeader->setLabel( i, *it );
			}

    UserInputObj*	mkUsrInputObj( const uiTable::Pos& pos )
			{

			    uiComboBox* cbb = new uiComboBox(0);
			    QWidget* widg = cbb->body()->qwidget();

			    setCellWidget( pos.y(), pos.x(), widg );

			    inputs += new Input( cbb, widg );

			    return cbb;
			}


    UserInputObj*	usrInputObj( const uiTable::Pos& pos )
			{
			    QWidget* w = cellWidget( pos.y(), pos.x() );
			    if ( !w ) return 0;

			    for ( int idx=0; idx < inputs.size(); idx ++ )
			    {
				if ( inputs[idx]->widg == w )
				    return inputs[idx]->obj;
			    }
			    return 0;
			}

    void		delUsrInputObj( const uiTable::Pos& pos )
			{
			    QWidget* w = cellWidget( pos.y(), pos.x() );
			    if ( !w ) return;

			    Input* inp=0;
			    for ( int idx=0; idx < inputs.size(); idx ++ )
			    {
				if ( inputs[idx]->widg == w )
				    { inp = inputs[idx]; break; }
			    }

			    clearCellWidget( pos.y(), pos.x() );
			    if( inp )	{ inputs -= inp; delete inp; }
			}

protected:

    ObjectSet<Input>	inputs;


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

    setHSzPol( uiObject::smallvar );
    setVSzPol( uiObject::smallvar );

    setStretch( s.colgrow_ ? 2 : 1, s.rowgrow_ ? 2 : 1 );
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
{
    if ( usrInputObj(pos) )
	rettxt_ = usrInputObj(pos)->text();
    else
	rettxt_ = body_->text( pos.y(), pos.x() );
    return rettxt_;
}

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

UserInputObj* uiTable::mkUsrInputObj( const Pos& pos )
    { return body_->mkUsrInputObj(pos); }
void uiTable::delUsrInputObj( const Pos& pos )
    { body_->delUsrInputObj(pos); }
UserInputObj* uiTable::usrInputObj(const Pos& pos)
    { return body_->usrInputObj(pos); }

const char* uiTable::rowLabel( int nr ) const
{
    static BufferString ret;
    QHeader* topHeader = body_->verticalHeader();
    ret = topHeader->label( nr );
    return ret;
}


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


const char* uiTable::columnLabel( int nr ) const
{
    static BufferString ret;
    QHeader* topHeader = body_->horizontalHeader();
    ret = topHeader->label( nr );
    return ret;
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


int uiTable::getIntValue( const Pos& p ) const
{
    if ( usrInputObj(p) )
	return usrInputObj(p)->getIntValue();

    return convertTo<int>( text(p) );
}

void uiTable::setValue( const Pos& p, int i )
{
    if ( usrInputObj(p) )
	usrInputObj(p)->setValue(i);

    setText(p, convertTo<const char*>(i) );
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
