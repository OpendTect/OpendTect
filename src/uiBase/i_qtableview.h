#ifndef i_qtableview_h
#define i_qtableview_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

 Inspired by Qt's Frozen Column Example
 https://doc.qt.io/qt-5/qtwidgets-itemviews-frozencolumn-example.html
-*/

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


void setNrColumns( int nrcol )
{ nrcols_ = nrcol; }


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

private slots:
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

#endif
