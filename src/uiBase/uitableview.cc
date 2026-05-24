/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitableview.h"
#include "i_qtableview.h"

#include "uiclipboard.h"
#include "uiobjbodyimpl.h"
#include "uipixmap.h"
#include "uiicon.h"

#include "tablemodel.h"

#include "q_uiimpl.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QComboBox>
#include <QColor>
#include <QDate>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMargins>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QUndoCommand>

#include "hiddenparam.h"

class ODStyledItemDelegate : public QStyledItemDelegate
{
public:
ODStyledItemDelegate( QObject* parent )
    : QStyledItemDelegate(parent)
{}

ODStyledItemDelegate( TableModel::CellType typ )
    : celltype_(typ)
{}

void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    QStyleOptionViewItem myoption = option;
    if ( option.state & QStyle::State_Selected )
	myoption.font.setBold( true );
    else
	myoption.font.setBold( false );
    initStyleOption( &myoption, index );

    // Give cells their original background color
    QVariant background = index.data( Qt::BackgroundRole );
    if ( background.canConvert<QBrush>() )
	painter->fillRect( option.rect, background.value<QBrush>() );
    else if ( background.canConvert<QString>() )
    {
	const QColor bgcol( background.toString() );
	if ( bgcol.isValid() )
	    painter->fillRect( option.rect, bgcol );
    }

    QStyledItemDelegate::paint( painter, myoption, index );
}

TableModel::CellType cellType() const
{
    return celltype_;
}

private:

    TableModel::CellType	celltype_	= TableModel::Text;

}; // class ODStyledItemDelegate


class DecorationItemDelegate : public ODStyledItemDelegate
{
public:
DecorationItemDelegate()
    : ODStyledItemDelegate(TableModel::Color)
{}

static const int sXPosPadding = 2;
static const int sPmScalePadding = 10;

const uiPixmap* createPixmap( const QVariant& qvar, QRect rect ) const
{
    const QString qstr = qvar.toString();
    const BufferString desc( qstr );
    if ( desc.isEmpty() )
	return nullptr;

    PixmapDesc pd;
    pd.fromString( desc );
    if ( !pd.isValid() )
	return nullptr;

    auto* pm = new uiPixmap( pd );
    BufferString pmsrc = pm->source();
    if ( pm && pmsrc != PixmapDesc::sKeySingleColorSrc() )
	pm->scaleToHeight( rect.height() - sPmScalePadding );

    return pm;
}


void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    QVariant color = index.data( Qt::BackgroundRole );
    if ( color.canConvert<QColor>() )
	painter->fillRect( option.rect, color.value<QColor>() );
    else if ( color.canConvert<QString>() )
    {
	const QColor bgcol( color.toString() );
	if ( bgcol.isValid() )
	    painter->fillRect( option.rect, bgcol );
    }

    const QVariant qvar = index.data( Qt::DecorationRole );
    ConstPtrMan<uiPixmap> pm = createPixmap( qvar, option.rect );
    ODStyledItemDelegate::paint( painter, option, index );
    if ( pm )
    {
	const QPixmap* qpm = pm->qpixmap();
	const int qpmwidth = qpm->rect().width();
	const int qpmheight = qpm->rect().height();
	const int xpos = option.rect.left() + sXPosPadding;
	const int ypos = option.rect.center().y() - qpmheight/2;
	painter->drawPixmap( xpos, ypos, qpmwidth, qpmheight, *qpm );
    }
}

}; // class DecorationItemDelegate


class DoubleItemDelegate : public ODStyledItemDelegate
{
public:
DoubleItemDelegate( TableModel::CellType tp,
		    char specifier, int precision )
    : ODStyledItemDelegate(tp)
    , specifier_(specifier)
    , precision_(precision)
{}


QString displayText( const QVariant& val, const QLocale& locale ) const override
{
    bool ok;
    const double dval = val.toDouble( &ok );
    if ( !ok )
	return QStyledItemDelegate::displayText( val, locale );

    QString ret;
    toUiString( dval, 0, specifier_, precision_ ).fillQString( ret );
    return ret;
}

   int		precision_;
   char		specifier_;

}; // class DoubleItemDelegate


class TextItemDelegate : public ODStyledItemDelegate
{
public:
TextItemDelegate()
    : ODStyledItemDelegate(TableModel::Text)
{}


QWidget* createEditor( QWidget* prnt,
		       const QStyleOptionViewItem&,
		       const QModelIndex& ) const override
{
    return new QLineEdit( prnt );
}


void setEditorData( QWidget* editor, const QModelIndex& index ) const override
{
    QString value = index.model()->data(index,Qt::DisplayRole).toString();
    QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
    lineedit->setText( value );
}


void setModelData( QWidget* editor, QAbstractItemModel* model,
		   const QModelIndex& index ) const override
{
    QLineEdit* lineedit = static_cast<QLineEdit*>(editor);
    QString txt = lineedit->text();
    model->setData( index, txt, Qt::EditRole );
}

}; // class TextItemDelegate


class EnumItemDelegate : public ODStyledItemDelegate
{
public:
EnumItemDelegate(const EnumDef* enumdef)
    : ODStyledItemDelegate(TableModel::Enum)
    , enumdef_(enumdef)
{}


static const int sXPosPadding = 2;

void paint( QPainter* painter, const QStyleOptionViewItem& option,
	    const QModelIndex& index ) const override
{
    if ( !enumdef_ )
    {
	pErrMsg("Model must supply an EnumDef for columns with CellType Enum");
	QStyledItemDelegate::paint( painter, option, index );
	return;
    }

    const int fontheight = option.fontMetrics.height();
    QRect rect = option.rect;
    const int ymargin = (rect.height() - fontheight) / 2;
    rect -= QMargins( sXPosPadding, ymargin, sXPosPadding, ymargin );

    const QVariant qvar = index.data( Qt::DisplayRole );
    const int enumidx = enumdef_->indexOf( qvar.toInt() );
    const char* iconfnm = enumdef_->getIconFileForIndex( enumidx );
    Qt::Alignment textalignment = Qt::AlignCenter;
    if ( iconfnm && *iconfnm )
    {
	uiIcon icon( iconfnm );
	QRect iconrect = rect;
	iconrect.setWidth( rect.height() );
	rect.adjust( rect.height()+sXPosPadding, 0, 0, 0 );
	textalignment = Qt::AlignLeft;
	icon.qicon().paint( painter, iconrect, Qt::AlignCenter );
    }

    uiString label = enumdef_->getUiStringForIndex( enumidx );
    QString labeltxt;
    painter->drawText( rect, textalignment, label.fillQString(labeltxt) );
}


QWidget* createEditor( QWidget* prnt,
		       const QStyleOptionViewItem&,
		       const QModelIndex& ) const override
{
    auto* qcb = new QComboBox( prnt );
    if ( !enumdef_ )
	return qcb;

    for ( int idx=0; idx<enumdef_->size(); idx++ )
    {
	qcb->addItem( toQString(enumdef_->getUiStringForIndex(idx)) );
	const char* iconnm = enumdef_->getIconFileForIndex( idx );
	if ( !iconnm )
	    continue;

	uiIcon icon( iconnm );
	qcb->setItemIcon( idx, icon.qicon() );
    }

    return qcb;
}


void setEditorData( QWidget* editor, const QModelIndex& index ) const override
{
    if ( !enumdef_ )
	return;

    const QVariant qvar = index.data( Qt::DisplayRole );
    const int enumidx = enumdef_->indexOf( qvar.toInt() );
    QComboBox* qcb = static_cast<QComboBox*>(editor);
    qcb->setCurrentIndex( enumidx );
}


void setModelData( QWidget* editor, QAbstractItemModel* model,
		   const QModelIndex& index ) const override
{
    if ( !enumdef_ )
	return;

    QComboBox* qcb = static_cast<QComboBox*>(editor);
    const int enumval = enumdef_->getEnumValForIndex( qcb->currentIndex() );
    model->setData( index, enumval, Qt::EditRole );
}

protected:

    const EnumDef*	enumdef_;

}; // class EnumItemDelegate


class DateItemDelegate : public ODStyledItemDelegate
{
public:
DateItemDelegate()
    : ODStyledItemDelegate(TableModel::Date)
{}


QString displayText( const QVariant& value,
		     const QLocale& locale ) const override
{
    return locale.toString( value.toDate(),
			    locale.dateFormat(QLocale::ShortFormat) );
}

}; // class DateItemDelegate


class DateTimeItemDelegate : public ODStyledItemDelegate
{
public:
DateTimeItemDelegate()
    : ODStyledItemDelegate(TableModel::DateTime)
{}


QString displayText( const QVariant& value,
		     const QLocale& locale ) const override
{
    QDateTime qdt = value.toDateTime().toLocalTime();
    return locale.toString( qdt,
			    locale.dateTimeFormat(QLocale::ShortFormat) );
}

}; // class DateTimeItemDelegate


class ODTableView : public uiObjBodyImpl<uiTableView,QTableView>
{
public:
		     ODTableView( uiTableView&,uiParent*,const char* nm);
		     ~ODTableView();

    void	    currentChanged(const QModelIndex& current,
				   const QModelIndex& previous) override;
    void	    selectionChanged(const QItemSelection& selected,
				     const QItemSelection& deselected) override;
    void	    setModel(QAbstractItemModel*) override;
    void	    init();
    void	    initFrozenView();
    void	    updateColumns();
    void	    setNrFrozenColumns(int nrcols);
    void	    setSortEnabled(bool yn);
    void	    setContextMenuEnabled(bool yn);
    bool	    setSourceDataDirect(const TableModelEditRequest&,
					bool useoldval);
    bool	    setSourceDataWithUndo(const TableModelEditRequest& req);
    void	    setCurrentCell(const RowCol&,bool noselection);
    void	    scrollTo(const QModelIndex&,ScrollHint) override;
    void	    pushUndoCommand(QUndoCommand*);
    void	    clearUndoStack();
    void	    undo();
    void	    redo();
    void	    markUndoBaseline();
    bool	    canUndo() const;
    bool	    canRedo() const;

    RowCol	    notifcell_;

protected:

    bool	    eventFilter(QObject*,QEvent*) override;
    void	    keyPressEvent(QKeyEvent*) override;
    void	    resizeEvent(QResizeEvent*) override;
    QModelIndex     moveCursor(CursorAction,Qt::KeyboardModifiers) override;
    void	    enableCustomContextMenu();
    void	    setContextMenuPolicyToDefault();

    QTableView*			frozenview_;
    int				nrfrozencols_	= 1;
    FrozenColumnsHelper*	helper_;
    i_tableViewMessenger&	messenger_;
    QUndoStack			undostack_;
    int				undobaselineidx_ = 0;
    bool			iscellundoactive_ = false;

};


class ODUndoCommand : public QUndoCommand
{
public:
    ODUndoCommand( ODTableView* view, QUndoCommand* parent = nullptr )
	: QUndoCommand(parent), view_(view)
    {}

protected:

    ODTableView*	    view_;

};

static HiddenParam<uiTableView,char> hp_enableundo_(1);
static HiddenParam<uiTableView,ODUndoCommand*> hp_activeundogroup_(nullptr);
static HiddenParam<uiTableView,Notifier<uiTableView>*> hp_undoredohappened_(
								    nullptr );


class CellEditUndoCommand : public ODUndoCommand
{
public:
CellEditUndoCommand( ODTableView* view,
		     const TypeSet<TableModelEditRequest>& reqs,
		     QUndoCommand* parent = nullptr )
    : ODUndoCommand(view,parent)
    , reqs_(reqs)
{
    setText( "Edit cell" );
}

void undo() override
{
    if ( !view_ )
	return;

    for ( int idx=reqs_.size()-1; idx>=0; idx-- )
	view_->setSourceDataDirect( reqs_.get(idx), true );
}

void redo() override
{
    if ( !view_ )
	return;

    for ( const auto& req : reqs_ )
	view_->setSourceDataDirect( req, false );
}

private:
    TypeSet<TableModelEditRequest> reqs_;
};


class RowDisplayUndoCommand : public ODUndoCommand
{
public:
RowDisplayUndoCommand( ODTableView* view, int row,
		   bool dohide, ODUndoCommand* parent )
    : ODUndoCommand( view, parent )
    , row_(row)
    , dohide_(dohide)
{
    setText( dohide ? "Hide row" : "Show row" );
}

void undo() override
{
    if ( view_ )
	view_->setRowHidden( row_, !dohide_ );
}

void redo() override
{
    if ( view_ )
	view_->setRowHidden( row_, dohide_ );
}

private:

    int		    row_;
    bool	    dohide_;
};


class ColDisplayUndoCommand : public ODUndoCommand
{
public:
ColDisplayUndoCommand( ODTableView* view, int col,
		   bool dohide, ODUndoCommand* parent )
    : ODUndoCommand( view, parent )
    , col_(col)
    , dohide_(dohide)
{
    setText( dohide ? "Hide column" : "Show column" );
}

void undo() override
{
    if ( view_ )
	view_->setColumnHidden( col_, !dohide_ );
}

void redo() override
{
    if ( view_ )
	view_->setColumnHidden( col_, dohide_ );
}

private:

    int		    col_;
    bool	    dohide_;
};


ODTableView::ODTableView( uiTableView& hndl, uiParent* p, const char* nm )
    : uiObjBodyImpl<uiTableView,QTableView>(hndl,p,nm)
    , messenger_(*new i_tableViewMessenger(this,&hndl))
{
    frozenview_ = new QTableView( this );
    helper_ = new FrozenColumnsHelper( this, frozenview_ );
    installEventFilter( this );
    if ( viewport() )
	viewport()->installEventFilter( this );
    if ( window() )
	window()->installEventFilter( this );
}


ODTableView::~ODTableView()
{
    if ( window() )
	window()->removeEventFilter( this );
    if ( viewport() )
	viewport()->removeEventFilter( this );
    removeEventFilter( this );
    delete helper_;
    delete frozenview_;
    delete &messenger_;
}


void ODTableView::currentChanged( const QModelIndex& current,
				  const QModelIndex& previous )
{
    QTableView::currentChanged( current, previous );
}


void ODTableView::selectionChanged( const QItemSelection& selected,
				    const QItemSelection& deselected )
{
    QTableView::selectionChanged( selected, deselected );
    handle_.selectionChanged.trigger();
}


void ODTableView::setModel( QAbstractItemModel* tblmodel )
{
    QTableView::setModel( tblmodel );
    frozenview_->setModel( model() );
    frozenview_->setSelectionModel( selectionModel() );
}


void ODTableView::init()
{
    setStyleSheet( "selection-background-color: rgba(50, 50, 50, 50);"
		   "selection-color: black;" );
    if ( horizontalHeader() )
	horizontalHeader()->setDefaultAlignment(
		Qt::AlignCenter | Qt::Alignment(Qt::TextWordWrap) );

    setHorizontalScrollMode( ScrollPerPixel );
    initFrozenView();
}


void ODTableView::initFrozenView()
{
    viewport()->stackUnder( frozenview_ );

    frozenview_->setStyleSheet( styleSheet() );
    frozenview_->setFrameStyle( QFrame::NoFrame );
    frozenview_->setFocusPolicy( Qt::NoFocus );

    frozenview_->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    frozenview_->setHorizontalScrollMode( horizontalScrollMode() );
    frozenview_->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    frozenview_->setVerticalScrollMode( verticalScrollMode() );

    frozenview_->verticalHeader()->hide();
    frozenview_->horizontalHeader()->setSectionResizeMode( QHeaderView::Fixed );

    updateColumns();
}


void ODTableView::updateColumns()
{
    for ( int col=0; col<nrfrozencols_; col++ )
	frozenview_->setColumnWidth( col, columnWidth(col) );

    for ( int col=0; col<model()->columnCount(); col++ )
	frozenview_->setColumnHidden( col, col>=nrfrozencols_ );

    helper_->updateGeom();
}


void ODTableView::setNrFrozenColumns( int nrcols )
{
    nrfrozencols_ = nrcols;
    helper_->setNrColumns( nrcols );
    updateColumns();
    if ( nrcols>0 )
	frozenview_->show();
    else
	frozenview_->hide();
}


void ODTableView::setSortEnabled( bool yn )
{
    setSortingEnabled( yn );
    frozenview_->setSortingEnabled( yn );
}


void ODTableView::setContextMenuEnabled( bool yn )
{
    yn ? enableCustomContextMenu() : setContextMenuPolicyToDefault();
}


bool ODTableView::setSourceDataDirect( const TableModelEditRequest& req,
				       bool useoldval )
{
    if ( !handle_.getModel() || req.row_<0 || req.col_<0 )
	return false;

    iscellundoactive_ = true;
    const bool ret = handle_.getModel()->applyEditRequest( req, useoldval );
    iscellundoactive_ = false;
    return ret;
}


void ODTableView::setCurrentCell( const RowCol& rc, bool noselection )
{
    notifcell_ = rc;
    const QModelIndex idx( model()->index(rc.row(),rc.col()) );
    if ( noselection )
	selectionModel()->setCurrentIndex( idx, QItemSelectionModel::NoUpdate );
    else
	setCurrentIndex( idx );
}


void ODTableView::pushUndoCommand( QUndoCommand* command )
{
    undostack_.push( command );
}


void ODTableView::clearUndoStack()
{
    undostack_.clear();
    undobaselineidx_ = 0;
}


void ODTableView::undo()
{
    if ( canUndo() )
	undostack_.undo();
}


void ODTableView::redo()
{
    undostack_.redo();
}


void ODTableView::markUndoBaseline()
{
    undobaselineidx_ = undostack_.index();
}


bool ODTableView::canUndo() const
{
    return undostack_.index() > undobaselineidx_;
}


bool ODTableView::canRedo() const
{
    return undostack_.canRedo();
}


bool ODTableView::eventFilter( QObject* obj, QEvent* ev )
{
    if ( !ev )
	return QObject::eventFilter( obj, ev );

    const bool isshcoverride = ev->type() == QEvent::ShortcutOverride;
    const bool iskeypress = ev->type() == QEvent::KeyPress;
    if ( !isshcoverride && !iskeypress )
	return QObject::eventFilter( obj, ev );

    auto* keyev = static_cast<QKeyEvent*>( ev );
    const bool isundo = keyev->matches( QKeySequence::Undo );
    const bool isredo = keyev->matches( QKeySequence::Redo );
    if ( !isundo && !isredo )
	return QObject::eventFilter( obj, ev );

    if ( isshcoverride )
    {
	keyev->accept();
	return true;
    }

    QWidget* focuswdgt = QApplication::focusWidget();
    QWidget* focuswindow = focuswdgt ? focuswdgt->window() : nullptr;
    if ( focuswindow != window() )
	return QObject::eventFilter( obj, ev );

    if ( isundo )
	undo();
    else
	redo();

    handle_.undoRedoHappened().trigger();
    keyev->accept();
    return true;
}


void ODTableView::keyPressEvent( QKeyEvent* ev )
{
    if ( !ev )
	return;

    if ( ev->key() == Qt::Key_Escape )
    {
	clearSelection();
	setFocus( Qt::OtherFocusReason );
	ev->setAccepted( true );
    }
    else if ( ev->matches(QKeySequence::Copy) )
    {
	BufferString text;
	QItemSelectionRange range = selectionModel()->selection().first();
	for ( int row = range.top(); row <= range.bottom(); row++ )
	{
	    for ( int col = range.left(); col <= range.right(); col++ )
	    {
		text.add( model()->index(row,col).data().toString() );
		text.addTab();
	    }

	    text.addNewLine();
	}

	uiClipboard::setText( text );
	ev->setAccepted( true );
    }
    else if ( ev->matches(QKeySequence::Undo) )
    {
	undo();
	handle_.undoRedoHappened().trigger();
	ev->setAccepted( true );
    }
    else if ( ev->matches(QKeySequence::Redo) )
    {
	redo();
	handle_.undoRedoHappened().trigger();
	ev->setAccepted( true );
    }
    else
	QTableView::keyPressEvent( ev );
}


void ODTableView::resizeEvent( QResizeEvent* event )
{
    QTableView::resizeEvent( event );
    helper_->updateGeom();
}


void ODTableView::scrollTo( const QModelIndex& index, ScrollHint hint )
{
    if ( index.column() > nrfrozencols_-1 )
	QTableView::scrollTo( index, hint );
}


QModelIndex ODTableView::moveCursor( CursorAction act,
				     Qt::KeyboardModifiers modif )
{
    QModelIndex current = QTableView::moveCursor( act, modif );
    const int mainviewx0 = visualRect(current).topLeft().x();
    int frozenwidth = 0;
    for ( int col=0; col<nrfrozencols_; col++ )
	frozenwidth += frozenview_->columnWidth( col );

    if ( act==MoveLeft && current.column()>0 && mainviewx0<frozenwidth )
    {
	const int newvalue =
		horizontalScrollBar()->value() + mainviewx0 - frozenwidth;
	horizontalScrollBar()->setValue( newvalue );
    }

    return current;
}


void ODTableView::enableCustomContextMenu()
{
    if ( contextMenuPolicy() == Qt::CustomContextMenu )
	return;

    setContextMenuPolicy( Qt::CustomContextMenu );
    horizontalHeader()->setContextMenuPolicy( Qt::CustomContextMenu );
    verticalHeader()->setContextMenuPolicy( Qt::CustomContextMenu );
    frozenview_->setContextMenuPolicy( Qt::CustomContextMenu );
    frozenview_->horizontalHeader()
	       ->setContextMenuPolicy( Qt::CustomContextMenu );
}


void ODTableView::setContextMenuPolicyToDefault()
{
    if ( contextMenuPolicy() == Qt::DefaultContextMenu )
	return;

    setContextMenuPolicy( Qt::DefaultContextMenu );
    horizontalHeader()->setContextMenuPolicy( Qt::DefaultContextMenu );
    verticalHeader()->setContextMenuPolicy( Qt::DefaultContextMenu );
    frozenview_->setContextMenuPolicy( Qt::DefaultContextMenu );
    frozenview_->horizontalHeader()
	       ->setContextMenuPolicy( Qt::DefaultContextMenu );
}


bool ODTableView::setSourceDataWithUndo( const TableModelEditRequest& req )
{
    if ( !handle_.getModel() )
	return false;

    if ( req.role_!=Qt::EditRole || !handle_.isUndoEnabled() ||
	 iscellundoactive_ || req.row_ < 0 || req.col_ < 0 )
	return false;

    if ( handle_.getModel()->getColumnCellType(req.col_) == TableModel::Color )
	return false;

    if ( req.oldval_ == req.newval_ )
	return true;

    TypeSet<TableModelEditRequest> relatedreqs;
    iscellundoactive_ = true;
    const bool collected = handle_.getModel()->collectEditRequests( req,
								 relatedreqs );
    iscellundoactive_ = false;
    if ( !collected )
	return false;

    if ( relatedreqs.isEmpty() )
	return true;

    iscellundoactive_ = true;
    for ( int idx=relatedreqs.size()-1; idx>=0; idx-- )
	handle_.getModel()->applyEditRequest( relatedreqs.get(idx), true );

    iscellundoactive_ = false;
    pushUndoCommand( new CellEditUndoCommand(this,relatedreqs,nullptr) );
    return true;
}


uiTableView::uiTableView( uiParent* p, const char* nm )
    : uiObject(p,nm,mkView(p,nm))
    , doubleClicked(this)
    , rightClicked(this)
    , selectionChanged(this)
    , columnClicked(this)
    , rowClicked(this)
{
    hp_enableundo_.setParam( this, 1 );
    hp_activeundogroup_.setParam( this, nullptr );
    hp_undoredohappened_.setParam( this, new Notifier<uiTableView>(this) );
    columndelegates_.setNullAllowed( true );
}


uiTableView::~uiTableView()
{
    detachAllNotifiers();
    hp_enableundo_.removeParam( this );
    hp_activeundogroup_.removeParam( this );
    hp_undoredohappened_.removeAndDeleteParam( this );
    if ( tablemodel_ )
	mDetachCB( tablemodel_->editRequested(), uiTableView::editRequestCB );

    delete horizontalheaderstate_;
    deepErase( columndelegates_ );
}


ODTableView& uiTableView::mkView( uiParent* p, const char* nm )
{
    odtableview_ = new ODTableView( *this, p, nm );
    return *odtableview_;
}


void uiTableView::setModel( TableModel* mdl )
{
    if ( tablemodel_ )
	mDetachCB( tablemodel_->editRequested(), uiTableView::editRequestCB );

    tablemodel_ = mdl;
    if ( !tablemodel_ )
	return;

    deleteAndNullPtr( horizontalheaderstate_ );
    delete qproxymodel_;
    qproxymodel_ = new QSortFilterProxyModel();
    qproxymodel_->setSourceModel( tablemodel_->getAbstractModel() );
    mAttachCB( tablemodel_->editRequested(), uiTableView::editRequestCB );
    odtableview_->setModel( qproxymodel_ );
    odtableview_->init();

    for ( int idx=0; idx<mdl->nrCols(); idx++ )
    {
	const char format = mdl->getColumnFormatSpecifier( idx );
	const int precision = mdl->getColumnPrecision( idx );
	setColumnValueType( idx, mdl->getColumnCellType(idx),
			    format, precision );
    }
}


void uiTableView::editRequestCB( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack( const TableModelEditRequest&, req, cb );
    req.handled_ = odtableview_ && odtableview_->setSourceDataWithUndo( req );
}


void uiTableView::setNrFrozenColumns( int nrcols )
{
    odtableview_->setNrFrozenColumns( nrcols );
}


void uiTableView::setContextMenuEnabled( bool yn )
{
    odtableview_->setContextMenuEnabled( yn );
}


void uiTableView::saveHorizontalHeaderState()
{
    deleteAndNullPtr( horizontalheaderstate_ );
    QHeaderView* horhdr = odtableview_->horizontalHeader();
    if ( horhdr )
	horizontalheaderstate_ = new QByteArray( horhdr->saveState() );
}


void uiTableView::resetHorizontalHeader()
{
    if ( !horizontalheaderstate_ )
	return;

    QHeaderView* horhdr = odtableview_->horizontalHeader();
    if ( horhdr )
	horhdr->restoreState( *horizontalheaderstate_ );

    odtableview_->clearSelection();
    odtableview_->clearFocus();
}


void uiTableView::setColumnStretchable( int col, bool yn )
{
    QHeaderView* header = odtableview_->horizontalHeader();
    QHeaderView::ResizeMode mode = yn ? QHeaderView::Stretch
				      : QHeaderView::Interactive ;

    const int idx = header->logicalIndex( col );
    if ( idx >= 0 )
	header->setSectionResizeMode( idx, mode );
    else
	header->setSectionResizeMode( mode );
}


void uiTableView::resizeColumnsToContents()
{
    odtableview_->resizeColumnsToContents();
}


void uiTableView::resizeColumnToContents( int column )
{
    odtableview_->resizeColumnToContents( column );
}


void uiTableView::setRowHeight( int row, int height )
{
    if ( row >= 0 )
	odtableview_->setRowHeight( row, height );
    else if ( row == -1 )
    {
	for ( int idx=0; idx<tablemodel_->nrRows(); idx++ )
	    odtableview_->setRowHeight( idx, height );
    }

    odtableview_->setRowHeight( row, height );
}


void uiTableView::setRowHeight( int height )
{
    auto* header = odtableview_->verticalHeader();
    if ( !header )
	return;

    header->setDefaultSectionSize( height );
    header->setSectionResizeMode( QHeaderView::Fixed );
}


void uiTableView::setSectionsMovable( bool yn )
{
    odtableview_->horizontalHeader()->setSectionsMovable( yn );
}


void uiTableView::setSortingEnabled( bool yn )
{
    odtableview_->setSortEnabled( yn );
}


bool uiTableView::isSortingEnabled() const
{
    return odtableview_->isSortingEnabled();
}


void uiTableView::sortByColumn( int col, bool asc )
{
    odtableview_->sortByColumn( col,
			asc ? Qt::AscendingOrder : Qt::DescendingOrder );
}


void uiTableView::enableUndo( bool yn )
{
    hp_enableundo_.setParam( this, yn ? 1 : 0 );
}


bool uiTableView::isUndoEnabled() const
{
    return hp_enableundo_.getParam( this ) != 0;
}


Notifier<uiTableView>& uiTableView::undoRedoHappened()
{
    auto* notif = hp_undoredohappened_.getParam( this );
    if ( !notif )
    {
	notif = new Notifier<uiTableView>( this );
	hp_undoredohappened_.setParam( this, notif );
    }

    return *notif;
}


void uiTableView::clearUndo()
{
    odtableview_->clearUndoStack();
}


void uiTableView::markUndoBaseline()
{
    odtableview_->markUndoBaseline();
}


void uiTableView::beginUndoGroup()
{
    if ( !isUndoEnabled() || hp_activeundogroup_.getParam(this) )
	return;

    hp_activeundogroup_.setParam( this,	
				  new ODUndoCommand(odtableview_,nullptr));
}


void uiTableView::endUndoGroup()
{
    ODUndoCommand* activeundogroup = hp_activeundogroup_.getParam( this );
    if ( !activeundogroup )
	return;

    ODUndoCommand* finishedgroup = activeundogroup;
    hp_activeundogroup_.setParam( this, nullptr );
    if ( finishedgroup->childCount() )
	odtableview_->pushUndoCommand( finishedgroup );
    else
	delete finishedgroup;
}


void uiTableView::undo()
{
    odtableview_->undo();

    undoRedoHappened().trigger();
}


void uiTableView::redo()
{
    odtableview_->redo();

    undoRedoHappened().trigger();
}


void uiTableView::scrollTo( const RowCol& rc )
{
    const QModelIndex sourceidx =
	tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
    const QModelIndex proxyidx = qproxymodel_->mapFromSource( sourceidx );
    if ( !proxyidx.isValid() )
	return;

    odtableview_->scrollTo( proxyidx, QAbstractItemView::PositionAtCenter );
}


void uiTableView::doHideRow( int row, bool yn, ODUndoCommand* parent )
{
    if ( odtableview_->isRowHidden(row) == yn )
	return;

    if ( !isUndoEnabled() )
    {
	odtableview_->setRowHidden( row, yn );
	return;
    }

    ODUndoCommand* undoparent = parent ? parent
				       : hp_activeundogroup_.getParam(this);
    auto* cmd = new RowDisplayUndoCommand( odtableview_, row, yn, undoparent );
    if ( undoparent )
	cmd->redo();
    else
	odtableview_->pushUndoCommand( cmd );
}


void uiTableView::setRowHidden( int row, bool yn )
{
    doHideRow( row, yn, nullptr );
}


void uiTableView::setRowsHidden( const TypeSet<int>& rows, bool yn )
{
    if ( rows.isEmpty() )
	return;

    if ( hp_activeundogroup_.getParam(this) )
    {
	for ( int idx=0; idx<rows.size(); idx++ )
	    doHideRow( rows[idx], yn, nullptr );

	return;
    }

    auto* undocommand = new ODUndoCommand( odtableview_, nullptr );
    for ( int idx=0; idx<rows.size(); idx++ )
	doHideRow( rows[idx], yn, undocommand );

    odtableview_->pushUndoCommand( undocommand );
}


bool uiTableView::isRowHidden( int row ) const
{
    return odtableview_->isRowHidden( row );
}


void uiTableView::doHideColumn( int col, bool yn, ODUndoCommand* parent )
{
    if ( odtableview_->isColumnHidden(col) == yn )
	return;

    if ( !isUndoEnabled() )
    {
	odtableview_->setColumnHidden( col, yn );
	return;
    }

    ODUndoCommand* undoparent = parent ? parent
				       : hp_activeundogroup_.getParam(this);
    auto* cmd = new ColDisplayUndoCommand( odtableview_, col, yn, undoparent );
    if ( undoparent )
	cmd->redo();
    else
	odtableview_->pushUndoCommand( cmd );
}


void uiTableView::setColumnHidden( int col, bool yn )
{
    doHideColumn( col, yn, nullptr );
}


void uiTableView::setColumnsHidden( const TypeSet<int>& cols, bool yn )
{
    if ( cols.isEmpty() )
	return;

    if ( hp_activeundogroup_.getParam(this) )
    {
	for ( int idx=0; idx<cols.size(); idx++ )
	    doHideColumn( cols[idx], yn, nullptr );

	return;
    }

    auto* undocommand = new ODUndoCommand( odtableview_, nullptr );
    for ( int idx=0; idx<cols.size(); idx++ )
	doHideColumn( cols[idx], yn, undocommand );

    odtableview_->pushUndoCommand( undocommand );
}


bool uiTableView::isColumnHidden( int col ) const
{
    return odtableview_->isColumnHidden( col );
}


void uiTableView::getVisibleRows( TypeSet<int>& rows,
				  bool mappedtosource ) const
{
    for ( int idx=0; idx<tablemodel_->nrRows(); idx++ )
    {
	if ( isRowHidden(idx) )
	    continue;

	if ( mappedtosource )
	{
	    const RowCol rc = mapToSource( RowCol(idx,0) );
	    rows += rc.row();
	}
	else
	    rows += idx;
    }
}


void uiTableView::getVisibleColumns( TypeSet<int>& cols,
				     bool mappedtosource ) const
{
    for ( int idx=0; idx<tablemodel_->nrCols(); idx++ )
    {
	if ( isColumnHidden(idx) )
	    continue;

	if ( mappedtosource )
	{
	    const RowCol rc = mapToSource( RowCol(0,idx) );
	    cols += rc.col();
	}
	else
	    cols += idx;
    }
}


void uiTableView::setRowsAndColsHidden( const TypeSet<int>& rows,
					const TypeSet<int>& cols, bool yn )
{
    if ( rows.isEmpty() && cols.isEmpty() )
	return;

    ODUndoCommand* activeundogroup = hp_activeundogroup_.getParam( this );
    auto* parent = activeundogroup ? activeundogroup
				   : new ODUndoCommand( odtableview_, nullptr );
    for ( const auto& row : rows )
	doHideRow( row, yn, parent );

    for ( const auto& col : cols )
	doHideColumn( col, yn, parent );

    if ( !activeundogroup )
	odtableview_->pushUndoCommand( parent );
}


void uiTableView::setRowsVisibility( const TypeSet<int>& rowstoshow,
				     const TypeSet<int>& rowstohide )
{
    if ( rowstoshow.isEmpty() && rowstohide.isEmpty() )
	return;

    if ( !isUndoEnabled() )
    {
	for ( const auto& row : rowstoshow )
	    odtableview_->setRowHidden( row, false );

	for ( const auto& row : rowstohide )
	    odtableview_->setRowHidden( row, true );

	return;
    }

    ODUndoCommand* activeundogroup = hp_activeundogroup_.getParam( this );
    auto* parent = activeundogroup ? activeundogroup
				   : new ODUndoCommand( odtableview_, nullptr );
    for ( const auto& row : rowstoshow )
	doHideRow( row, false, parent );

    for ( const auto& row : rowstohide )
	doHideRow( row, true, parent );

    if ( !activeundogroup )
	odtableview_->pushUndoCommand( parent );
}


void uiTableView::setColumnsVisibility( const TypeSet<int>& colstoshow,
					const TypeSet<int>& colstohide )
{
    if ( colstoshow.isEmpty() && colstohide.isEmpty() )
	return;

    if ( !isUndoEnabled() )
    {
	for ( const auto& col : colstoshow )
	    odtableview_->setColumnHidden( col, false );

	for ( const auto& col : colstohide )
	    odtableview_->setColumnHidden( col, true );

	return;
    }

    ODUndoCommand* activeundogroup = hp_activeundogroup_.getParam( this );
    auto* parent = activeundogroup ? activeundogroup
				   : new ODUndoCommand( odtableview_, nullptr );
    for ( const auto& col : colstoshow )
	doHideColumn( col, false, parent );

    for ( const auto& col : colstohide )
	doHideColumn( col, true, parent );

    if ( !activeundogroup )
	odtableview_->pushUndoCommand( parent );
}


void uiTableView::setHeaderVisible( OD::Orientation odor, bool yn )
{
    if ( odor==OD::Horizontal && odtableview_->horizontalHeader() )
	odtableview_->horizontalHeader()->setVisible( yn );
    else if ( odor==OD::Vertical && odtableview_->verticalHeader() )
	odtableview_->verticalHeader()->setVisible( yn );
}


bool uiTableView::isHeaderVisible( OD::Orientation odor ) const
{
    if ( odor == OD::Horizontal )
	return odtableview_->horizontalHeader() &&
	       odtableview_->horizontalHeader()->isVisible();

    return odtableview_->verticalHeader() &&
	       odtableview_->verticalHeader()->isVisible();
}


RowCol uiTableView::mapFromSource( const RowCol& rc ) const
{
    QModelIndex sourceidx =
	tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
    QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
    return RowCol( qmi.row(), qmi.column() );
}


RowCol uiTableView::mapToSource( const RowCol& rc ) const
{
    QModelIndex proxyidx = qproxymodel_->index( rc.row(), rc.col() );
    QModelIndex qmi = qproxymodel_->mapToSource( proxyidx );
    return RowCol( qmi.row(), qmi.column() );
}


void uiTableView::setSelectionBehavior( SelectionBehavior sb )
{
    odtableview_->setSelectionBehavior(
		sCast(QAbstractItemView::SelectionBehavior,sCast(int,sb)) );
}


uiTableView::SelectionBehavior uiTableView::getSelectionBehavior() const
{
    QAbstractItemView::SelectionBehavior sb = odtableview_->selectionBehavior();
    return sCast(uiTableView::SelectionBehavior,sb);
}


void uiTableView::setSelectionMode( SelectionMode sm )
{
    odtableview_->setSelectionMode(
		    sCast(QAbstractItemView::SelectionMode,sCast(int,sm)) );
}


uiTableView::SelectionMode uiTableView::getSelectionMode() const
{
    QAbstractItemView::SelectionMode sm = odtableview_->selectionMode();
    return sCast(uiTableView::SelectionMode,sm);
}


void uiTableView::clearSelection()
{
    odtableview_->clearSelection();
}


int uiTableView::maxNrOfSelections() const
{
    if ( getSelectionMode()==NoSelection )
	return 0;
    if ( getSelectionMode()==SingleSelection )
	return 1;
    if ( getSelectionBehavior()==SelectRows )
	return tablemodel_->nrRows();
    if ( getSelectionBehavior()==SelectColumns )
	return tablemodel_->nrCols();

    return tablemodel_->nrRows() * tablemodel_->nrCols();
}


bool uiTableView::getSelectedRows( TypeSet<int>& rows ) const
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return false;

    QModelIndexList selection = selmdl->selectedRows();
    for ( int idx=0; idx<selection.size(); idx++ )
    {
	const int selrow = selection[idx].row();
	if ( !isRowHidden(selrow) )
	    rows += selrow;
    }

    return rows.size();
}


bool uiTableView::getSelectedColumns( TypeSet<int>& cols ) const
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return false;

    QModelIndexList selection = selmdl->selectedColumns();
    for ( int idx=0; idx<selection.size(); idx++ )
    {
	const int selcol = selection[idx].column();
	if ( !isColumnHidden(selcol) )
	    cols += selcol;
    }

    return cols.size();
}


bool uiTableView::getSelectedCells( TypeSet<RowCol>& rcs,
				    bool mappedtosource ) const
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return false;

    QModelIndexList selection = selmdl->selectedIndexes();
    for ( int idx=0; idx<selection.size(); idx++ )
    {
	const int selrow = selection[idx].row();
	const int selcol = selection[idx].column();
	if ( isRowHidden(selrow) || isColumnHidden(selcol) )
	    continue;

	const RowCol rc( selrow, selcol );
	if ( mappedtosource )
	    rcs += mapToSource( rc );
	else
	    rcs += rc;
    }

    return rcs.size();
}


void uiTableView::setSelectedCells( const TypeSet<RowCol>& rcs )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    for ( const auto& rc : rcs )
    {
	const QModelIndex idx =
		tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
	selmdl->select( idx, QItemSelectionModel::Select );
    }
}


void uiTableView::setSelectedCells( const TypeSet<RowCol>& rcs,
				    bool mapfromsource )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    for ( const auto& rc : rcs )
    {
	const QModelIndex sourceidx =
		tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
	if ( !mapfromsource )
	{
	    selmdl->select( sourceidx, QItemSelectionModel::Select );
	    continue;
	}

	const QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
	selmdl->select( qmi, QItemSelectionModel::Select );
    }
}


void uiTableView::setCellSelected( const RowCol& rc, bool yn,
				   bool mapfromsource )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    const QModelIndex sourceidx =
		tablemodel_->getAbstractModel()->index( rc.row(), rc.col() );
    if ( !mapfromsource )
	selmdl->select( sourceidx, QItemSelectionModel::Select );
    else
    {
	const QModelIndex qmi = qproxymodel_->mapFromSource( sourceidx );
	selmdl->select( qmi, QItemSelectionModel::Select );
    }
}


bool uiTableView::isCellSelected( const RowCol& rc, bool mapfromsource ) const
{
    const QItemSelectionModel* selmdl = odtableview_->selectionModel();
    const QAbstractItemModel* model = selmdl ? selmdl->model() : nullptr;
    if ( !model )
	return false;

    QModelIndex idx = odtableview_->rootIndex();
    idx = model->index( rc.row(), rc.col(), idx );
    return selmdl->isSelected( idx );
}


void uiTableView::selectColumn( int col )
{
    odtableview_->selectColumn( col );
}


void uiTableView::selectRow( int row )
{
    odtableview_->selectRow( row );
}


void uiTableView::removeSelection( const TypeSet<RowCol>& rcs )
{
    QItemSelectionModel* selmdl = odtableview_->selectionModel();
    if ( !selmdl->hasSelection() )
	return;

    for ( const auto& rc : rcs )
    {
	const QModelIndex idx = tablemodel_->getAbstractModel()
					   ->index( rc.row(), rc.col() );
	selmdl->select( idx, QItemSelectionModel::Deselect );
    }
}


void uiTableView::selectAll()
{
    odtableview_->selectAll();
}


void uiTableView::setColumnValueType( int col, TableModel::CellType tp,
				      char format, int precision )
{
    ODStyledItemDelegate* coldelegate = getColumnDelegate( col, tp,
							   format, precision );
    if ( coldelegate )
	odtableview_->setItemDelegateForColumn( col, coldelegate );
}


ODStyledItemDelegate*
	uiTableView::getColumnDelegate( int col, TableModel::CellType tp,
					char format, int precision )
{
    if ( columndelegates_.validIdx(col) && columndelegates_[col] &&
	 columndelegates_[col]->cellType() == tp )
	return columndelegates_[col];

    while ( columndelegates_.size() <= col )
	columndelegates_ += nullptr;

    ODStyledItemDelegate* res = createColumnDelegate( col, tp,
						      format, precision );
    delete columndelegates_.replace( col, res );
    return res;
}


ODStyledItemDelegate*
	uiTableView::createColumnDelegate( int col, TableModel::CellType tp,
					   char format, int precision )
{
    if ( tp==TableModel::NumD || tp==TableModel::NumF )
	return new DoubleItemDelegate( tp, format, precision );
    if ( tp==TableModel::Text )
	return new TextItemDelegate;
    if ( tp==TableModel::Enum )
	return new EnumItemDelegate( tablemodel_ ? tablemodel_->getEnumDef(col)
						 : nullptr );
    if ( tp==TableModel::Color )
	return new DecorationItemDelegate;
    if ( tp==TableModel::Date )
	return new DateItemDelegate;
    if ( tp==TableModel::DateTime )
	return new DateTimeItemDelegate;

    return nullptr;
}


void uiTableView::setColumnWidth( int col, int wdth )
{
    odtableview_->setColumnWidth( col, wdth );
}


const RowCol& uiTableView::currentCell() const
{
    return odtableview_->notifcell_;
}


void uiTableView::setCurrentCell( const RowCol& rc, bool noselection )
{
    return odtableview_->setCurrentCell( rc, noselection );
}


TableModel::CellType uiTableView::getCellType( int col ) const
{
    if ( columndelegates_.validIdx(col) && columndelegates_[col] )
	return columndelegates_[col]->cellType();

    return TableModel::Other;
}


void uiTableView::moveColumn( int from, int to )
{
    const QModelIndex fromidx
		= tablemodel_->getAbstractModel()->index( 0, from );
    if ( !fromidx.isValid() )
    {
	pErrMsg("Invalid \"from\" column");
	return;
    }

    const QModelIndex toidx
		= tablemodel_->getAbstractModel()->index( 0, to );
    if ( !toidx.isValid() )
    {
	pErrMsg("Invalid \"to\" column");
	return;
    }

    QHeaderView *headerView = odtableview_->horizontalHeader();
    headerView->moveSection(from,to);
}
