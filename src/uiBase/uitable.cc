/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/02/2003
 RCS:           $Id: uitable.cc,v 1.16 2004-03-19 14:27:35 nanne Exp $
________________________________________________________________________

-*/

#include "uitable.h"
#include "i_qtable.h"

#include "uifont.h"
#include "uimenu.h"
#include "pixmap.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "uicombobox.h"
#include "basictypes.h"
#include "bufstringset.h"

#include <qcolor.h>


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

                        uiTableBody( uiTable& handle, uiParent* parnt, 
				     const char* nm, int nrows, int ncols)
			    : uiObjBodyImpl<uiTable,QTable>( handle, parnt, nm )
			    , messenger_ (*new i_tableMessenger(this, &handle))
			{
			    if ( nrows >= 0 ) setLines( nrows + 1 );
			    if ( ncols >= 0 ) setNumCols( ncols );
			}

    virtual 		~uiTableBody()
			    { deepErase( inputs ); delete &messenger_; }

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
			    if ( stretch(false) != 1 )
				setStretch( hs, ( nrTxtLines()== 1) ? 0 : 2 );
			}

    virtual int 	nrTxtLines() const
			    { return numRows() >= 0 ? numRows()+1 : 7; }

    void		setRowLabels( const QStringList &labels )
			{

			    QHeader* leftHeader = verticalHeader();

			    int i = 0;
			    for( QStringList::ConstIterator it = labels.begin();
				  it != labels.end() && i < numRows(); ++i,++it
			       )
				leftHeader->setLabel( i, *it );
			}


    UserInputObj*	mkUsrInputObj( const uiTable::RowCol& rc )
			{

			    uiComboBox* cbb = new uiComboBox(0);
			    QWidget* widg = cbb->body()->qwidget();

			    setCellWidget( rc.row, rc.col, widg );

			    inputs += new Input( cbb, widg );

			    return cbb;
			}


    UserInputObj*	usrInputObj( const uiTable::RowCol& rc )
			{
			    QWidget* w = cellWidget( rc.row, rc.col );
			    if ( !w ) return 0;

			    for ( int idx=0; idx < inputs.size(); idx ++ )
			    {
				if ( inputs[idx]->widg == w )
				    return inputs[idx]->obj;
			    }
			    return 0;
			}


    void		delUsrInputObj( const uiTable::RowCol& rc )
			{
			    QWidget* w = cellWidget( rc.row, rc.col );
			    if ( !w ) return;

			    Input* inp=0;
			    for ( int idx=0; idx < inputs.size(); idx ++ )
			    {
				if ( inputs[idx]->widg == w )
				    { inp = inputs[idx]; break; }
			    }

			    clearCellWidget( rc.row, rc.col );
			    if ( inp )	{ inputs -= inp; delete inp; }
			}

protected:

    ObjectSet<Input>	inputs;


private:

    i_tableMessenger&	messenger_;

};


uiTable::uiTable( uiParent* p, const Setup& s, const char* nm )
    : uiObject(p,nm,mkbody(p,nm,s.size_.height(),s.size_.width()))
    , setup_(s)
    , valueChanged(this)
    , clicked(this)
    , rightClicked(this)
    , leftClicked(this)
    , doubleClicked(this)
    , rowInserted(this)
    , colInserted(this)
    , rowDeleted(this)
    , colDeleted(this)
{
    clicked.notify( mCB(this,uiTable,clicked_) );
    setGeometry.notify( mCB(this,uiTable,geometrySet_) );

    setHSzPol( uiObject::medvar );
    setVSzPol( uiObject::smallvar );

//    setStretch( s.colgrow_ ? 2 : 1, s.rowgrow_ ? 2 : 1 );

    setSelectionMode( s.selmode_ );
    if ( s.defrowlbl_ )
	setDefaultRowLabels();
    if ( s.defcollbl_ )
	setDefaultColLabels();
}


uiTableBody& uiTable::mkbody( uiParent* p, const char* nm, int nr, int nc)
{
    body_ = new uiTableBody(*this,p,nm,nr,nc);
    return *body_;
}


uiTable::~uiTable() {}


void uiTable::setDefaultRowLabels()
{
    const int nrrows = nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	BufferString lbl( setup_.rowdesc_ ); lbl += " ";
	lbl += idx+1;
	setRowLabel( idx, lbl );
    }
}


void uiTable::setDefaultColLabels()
{
    const int nrcols = nrCols();
    for ( int idx=0; idx<nrcols; idx++ )
    {
	BufferString lbl( setup_.coldesc_ ); lbl += " ";
	lbl += idx+1;
	setColumnLabel( idx, lbl );
    }
}

#define updateRow() if ( setup_.defrowlbl_ ) setDefaultRowLabels();
#define updateCol() if ( setup_.defcollbl_ ) setDefaultColLabels();

int  uiTable::columnWidth( int c ) const	{ return body_->columnWidth(c);}
int  uiTable::rowHeight( int r ) const		{ return body_->rowHeight(r);}
void uiTable::setColumnWidth(int col, int w)	{ body_->setColumnWidth(col,w);}
void uiTable::setRowHeight( int row, int h )	{ body_->setRowHeight(row,h); }

void uiTable::insertRows(int r, int cnt)	
{ body_->insertRows( r, cnt ); updateRow() }

void uiTable::insertColumns(int c,int cnt)	
{ body_->insertColumns(c,cnt); updateCol() }

void uiTable::removeRow( int row )		
{ body_->removeRow( row ); updateRow() }

void uiTable::removeColumn( int col )
{ body_->removeColumn( col );  updateCol() }

void uiTable::setNrRows( int nr )
{ body_->setLines( nr + 1 ); updateRow() }

void uiTable::setNrCols( int nr )
{ body_->setNumCols( nr );  updateCol() }

int  uiTable::nrRows() const			{ return  body_->numRows(); }
int  uiTable::nrCols() const			{ return body_->numCols(); }

void uiTable::setText( const RowCol& rc, const char* txt )
    { body_->setText( rc.row, rc.col, txt ); }


void uiTable::clearCell( const RowCol& rc )
    { body_->clearCell( rc.row, rc.col ); }


void uiTable::setCurrentCell( const RowCol& rc )
    { body_->setCurrentCell( rc.row, rc.col ); }


const char* uiTable::text( const RowCol& rc ) const
{
    if ( usrInputObj(rc) )
	rettxt_ = usrInputObj(rc)->text();
    else
	rettxt_ = body_->text( rc.row, rc.col );
    return rettxt_;
}


void uiTable::setColumnReadOnly( int col, bool yn )
    { body_->setColumnReadOnly( col, yn ); }


void uiTable::setRowReadOnly( int row, bool yn )
    { body_->setRowReadOnly( row, yn ); }


bool uiTable::isColumnReadOnly( int col ) const
    { return body_->isColumnReadOnly(col); }


bool uiTable::isRowReadOnly( int row ) const
    { return body_->isRowReadOnly(row); }


void uiTable::setColumnStretchable( int col, bool stretch )
    { body_->setColumnStretchable( col, stretch ); }


void uiTable::setRowStretchable( int row, bool stretch )
    { body_->setRowStretchable( row, stretch ); }


bool uiTable::isColumnStretchable( int col ) const
    { return body_->isColumnStretchable(col); }


bool uiTable::isRowStretchable( int row ) const
    { return body_->isRowStretchable(row); }


UserInputObj* uiTable::mkUsrInputObj( const RowCol& rc )
    { return body_->mkUsrInputObj(rc); }


void uiTable::delUsrInputObj( const RowCol& rc )
    { body_->delUsrInputObj(rc); }


UserInputObj* uiTable::usrInputObj( const RowCol& rc )
    { return body_->usrInputObj(rc); }


void uiTable::setPixmap( const RowCol& rc, const ioPixmap& pm )
{
    body_->setPixmap( rc.row, rc.col, *pm.Pixmap() );
}


void uiTable::setColor( const RowCol& rc, const Color& col )
{
    QRect rect = body_->cellRect( rc.row, rc.col );
    QPixmap pix( rect.width(), rect.height() );
    QColor qcol( col.r(), col.g(), col.b() );
    pix.fill( qcol );
    body_->setPixmap( rc.row, rc.col, pix );
    body_->setFocus();
    body_->setText( rc.row, rc.col, qcol.name() );
}


const Color uiTable::getColor( const RowCol& rc ) const
{
    BufferString coltxt = text(rc);
    if ( !*coltxt ) coltxt = "white";
	
    QColor qcol( coltxt.buf() );
    return Color( qcol.red(), qcol.green(), qcol.blue() );
}


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

    //setRowStretchable( row, true );
}


void uiTable::setRowLabels( const char** labels )
{
    int nlabels=0;
    while ( labels[nlabels] )
	nlabels++;

    body_->setLines( nlabels + 1 );
    for ( int idx=0; idx<nlabels; idx++ )
	setRowLabel( idx, labels[idx] );
}


void uiTable::setRowLabels( const BufferStringSet& labels )
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

    //setColumnStretchable( col, true );
}


void uiTable::setColumnLabels( const char** labels )
{
    int nlabels=0;
    while ( labels[nlabels] )
	nlabels++;

    body_->setNumCols( nlabels );
    for ( int idx=0; idx<nlabels; idx++ )
	setColumnLabel( idx, labels[idx] );
}


void uiTable::setColumnLabels( const BufferStringSet& labels )
{
    body_->setNumCols( labels.size() );

    for ( int i=0; i<labels.size(); i++ )
        setColumnLabel( i, *labels[i] );
}


int uiTable::getIntValue( const RowCol& rc ) const
{
    if ( usrInputObj(rc) )
	return usrInputObj(rc)->getIntValue();

    return convertTo<int>( text(rc) );
}


double uiTable::getValue( const RowCol& rc ) const
{
    if ( usrInputObj(rc) )
	return usrInputObj(rc)->getValue();

    return convertTo<double>( text(rc) );
}


float uiTable::getfValue( const RowCol& rc ) const
{
    if ( usrInputObj(rc) )
	return usrInputObj(rc)->getfValue();

    return convertTo<float>( text(rc) );
}


void uiTable::setValue( const RowCol& rc, int i )
{
    if ( usrInputObj(rc) )
	usrInputObj(rc)->setValue(i);

    setText( rc, convertTo<const char*>(i) );
}


void uiTable::setValue( const RowCol& rc, float f )
{
    if ( usrInputObj(rc) )
	usrInputObj(rc)->setValue(f);

    setText( rc, convertTo<const char*>(f) );
}


void uiTable::setValue( const RowCol& rc, double d )
{
    if ( usrInputObj(rc) )
	usrInputObj(rc)->setValue(d);

    setText( rc, convertTo<const char*>(d) );
}


void uiTable::setSelectionMode( SelectionMode m )
{
    switch ( m ) 
    {
	case Single : body_->setSelectionMode( QTable::Single ); break;
	case Multi : body_->setSelectionMode( QTable::Multi ); break;
	case SingleRow : body_->setSelectionMode( QTable::SingleRow ); break;
	case MultiRow : body_->setSelectionMode( QTable::MultiRow ); break;
	default :  body_->setSelectionMode( QTable::NoSelection ); break;
    }

}


void uiTable::clicked_( CallBacker* cb )
{
    mCBCapsuleUnpack(const uiMouseEvent&,ev,cb);

    if ( ev.buttonState() & uiMouseEvent::RightButton )
	{ rightClk(); rightClicked.trigger(); return; }

    if ( ev.buttonState() & uiMouseEvent::LeftButton )
	leftClicked.trigger();

    if ( setup_.snglclkedit_ )
	editCell( notifcell_, false );
}


void uiTable::editCell( const RowCol& rc, bool replace )
{
    body_->editCell( rc.row, rc.col, replace );
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

    RowCol cur = notifiedCell();

    if ( ret == inscolbef || ret == inscolaft )
    {
	const int offset = (ret == inscolbef) ? 0 : 1;
	newcell_ = RowCol( cur.row, cur.col + offset );
	insertColumns( newcell_ );

	if ( !setup_.defcollbl_ )
	{
	    BufferString label( newcell_.col );
	    setColumnLabel( newcell_, label );
	}

	colInserted.trigger();
    }
    else if ( ret == delcol )
    {
	removeColumn( cur.col );
	colDeleted.trigger();
    }
    else if ( ret == insrowbef || ret == insrowaft  )
    {
	const int offset = (ret == insrowbef) ? 0 : 1;
	newcell_ = RowCol( cur.row + offset, cur.col );
	insertRows( newcell_ );

	if ( !setup_.defrowlbl_ )
	{
	    BufferString label( newcell_.row );
	    setRowLabel( newcell_, label );
	}

	rowInserted.trigger();
    }
    else if ( ret == delrow )
    {
	removeRow( cur.row );
	rowDeleted.trigger();
    }

    setCurrentCell( newcell_ );
    updateCellSizes();
}


void uiTable::geometrySet_( CallBacker* cb )
{
//    if ( !mainwin() ||  mainwin()->poppedUp() ) return;
    mCBCapsuleUnpack(uiRect&,sz,cb);

    uiSize size = sz.getsize();
    updateCellSizes( &size );
}


void uiTable::updateCellSizes( uiSize* size )
{
    if ( size ) lastsz = *size;
    else	size = &lastsz;

    int nc = nrCols();
    if ( nc && setup_.fillrow_ )
    {
	int width = size->hNrPics();
	int availwdt = width - body_->verticalHeader()->frameSize().width()
			 - 2*body_->frameWidth();

	int colwdt = availwdt / nc;

	const int minwdt = (int)(setup_.mincolwdt_ * (float)font()->avgWidth());
	const int maxwdt = (int)(setup_.maxcolwdt_ * (float)font()->avgWidth());

	if ( colwdt < minwdt ) colwdt = minwdt;
	if ( colwdt > maxwdt ) colwdt = maxwdt;

	for ( int idx=0; idx < nc; idx ++ )
	{
	    if ( idx < nc-1 )
		setColumnWidth( idx, colwdt );
	    else 
	    {
		int wdt = availwdt;
		if ( wdt < minwdt )	wdt = minwdt;
		if ( wdt > maxwdt )	wdt = maxwdt;

		setColumnWidth( idx, wdt );
	    }
	    availwdt -= colwdt;
	}
    }

    int nr = nrRows();
    if ( nr && setup_.fillcol_ )
    {
	int height = size->vNrPics();
	int availhgt = height - body_->horizontalHeader()->frameSize().height()
			 - 2*body_->frameWidth();

	int rowhgt =  availhgt / nr;
	const float fonthgt = (float)font()->height();

	const int minhgt = (int)(setup_.minrowhgt_ * fonthgt);
	const int maxhgt = (int)(setup_.maxrowhgt_ * fonthgt);

	if ( rowhgt < minhgt ) rowhgt = minhgt;
	if ( rowhgt > maxhgt ) rowhgt = maxhgt; 

	for ( int idx=0; idx < nr; idx ++ )
	{
	    if ( idx < nr-1 )
		setRowHeight( idx, rowhgt );
	    else
	    {
		int hgt = availhgt;
		if ( hgt < minhgt ) hgt = minhgt;
		if ( hgt > maxhgt ) hgt = maxhgt;

		setRowHeight( idx, hgt );
	    }
	    availhgt -= rowhgt;
	}
    }

}
