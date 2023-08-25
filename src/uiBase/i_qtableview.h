#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

/*
  Inspired by Qt's Frozen Column Example
  https://doc.qt.io/qt-5/qtwidgets-itemviews-frozencolumn-example.html
*/

#include <uitableview.h>

#include <QTableView>
#include <QScrollBar>
#include <QHeaderView>

QT_BEGIN_NAMESPACE

class FrozenColumnsHelper : public QObject
{
Q_OBJECT
friend class ODTableView;

protected:
FrozenColumnsHelper( QTableView* mainview, QTableView* frozenview )
    : mainview_(mainview)
    , frozenview_(frozenview)
    , nrcols_(1)
{
    connect( mainview_->horizontalHeader(), &QHeaderView::sectionResized,
	     this, &FrozenColumnsHelper::columnResized );
    connect( mainview_->verticalHeader(), &QHeaderView::sectionResized,
	     this, &FrozenColumnsHelper::rowResized );

    connect( frozenview_->verticalScrollBar(), &QAbstractSlider::valueChanged,
	     mainview_->verticalScrollBar(), &QAbstractSlider::setValue );
    connect( mainview_->verticalScrollBar(), &QAbstractSlider::valueChanged,
	     frozenview_->verticalScrollBar(), &QAbstractSlider::setValue );
}


~FrozenColumnsHelper()
{}


void setNrColumns( int nrcol )
{
    nrcols_ = nrcol;
}


void updateGeom()
{
    const int posx =
	mainview_->verticalHeader()->width() + mainview_->frameWidth();
    const int posy = mainview_->frameWidth();
    int viewwidth = 0;
    for ( int col=0; col<nrcols_; col++ )
	viewwidth += mainview_->columnWidth( col );

    const int viewheight = mainview_->viewport()->height() +
			   mainview_->horizontalHeader()->height();
    frozenview_->setGeometry( posx, posy, viewwidth-1, viewheight );
}

    QTableView*		mainview_;	// main table
    QTableView*		frozenview_;	// frozen column(s)
    int			nrcols_;	// nr of frozen columns

private Q_SLOTS:
void columnResized( int col, int oldwidth, int newwidth )
{
    if ( col >= nrcols_ )
	return;

    frozenview_->setColumnWidth( col, newwidth );
    updateGeom();
}


void rowResized( int row, int oldheight, int newheight )
{
    frozenview_->setRowHeight( row, newheight );
}

};


class i_tableViewMessenger : public QObject
{
Q_OBJECT
friend class ODTableView;

protected:
i_tableViewMessenger( QTableView* sndr, uiTableView* rcvr )
    : sender_(sndr)
    , receiver_(rcvr )
{
    connect( sndr, &QAbstractItemView::doubleClicked,
	     this, &i_tableViewMessenger::doubleClicked,
	     Qt::QueuedConnection );
    connect( sndr->verticalHeader(), &QHeaderView::sectionClicked,
	    this, &i_tableViewMessenger::rowClicked );
    connect( sndr->horizontalHeader(), &QHeaderView::sectionClicked,
	    this, &i_tableViewMessenger::columnClicked );
    connect( sndr->verticalHeader(), &QHeaderView::sectionPressed,
	    this, &i_tableViewMessenger::rowPressed );
    connect( sndr->horizontalHeader(), &QHeaderView::sectionPressed,
	    this, &i_tableViewMessenger::columnPressed );
}


~i_tableViewMessenger()
{}

private:

    QTableView*		sender_;
    uiTableView*	receiver_;

private Q_SLOTS:

void handleSlot( const char* notifiernm, int row, int col,
		 Notifier<uiTableView>* notifier = nullptr )
{
    BufferString msg( notifiernm );
    msg.addSpace().add( row ).addSpace().add( col );
    const int refnr = receiver_->beginCmdRecEvent( msg );
    if ( notifier )
	notifier->trigger( *receiver_ );

    receiver_->endCmdRecEvent( refnr, msg );
}


void doubleClicked( const QModelIndex& index )
{
    const int row = index.row();
    const int col = index.column();
    receiver_->setCurrentCell( RowCol(row,col) );
    handleSlot( "doubleClicked", row, col, &receiver_->doubleClicked );
}


void rowClicked( int row )
{
    handleSlot( "rowClicked", row, -1 );
}


void columnClicked( int col )
{
    handleSlot( "columnClicked", -1, col );
}


void rowPressed( int row )
{
    handleSlot( "rowPressed", row, -1 );
}


void columnPressed( int col )
{
    handleSlot( "columnPressed", -1, col );
}

};

QT_END_NAMESPACE
