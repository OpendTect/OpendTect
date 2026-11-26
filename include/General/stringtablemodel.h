#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tablemodel.h"
#include "bufstringset.h"
#include "manobjectset.h"
#include "stringview.h"
#include "typeset.h"
#include "uistring.h"


static OD::Color sFirstColumnColor() { return OD::Color(235,235,235); }

/*!\brief One snapshot of the strings a StringTableModel shows.

  Leave a list empty for the default. types_ empty means every column is
  Text. A shorter types_ list leaves the remaining columns as Text. A
  DateTime cell is an ISO-8601 string such as "2026-11-23T09:22:00". icons_
  empty means no icons. rowcolors_ empty, or OD::Color::NoColor(),
  leaves the view background. rowlabels_ empty, or an empty string, makes
  that row's header row+1.

  rows_ owns its sets. The model takes them. Do not put pointers into
  the source object in rows_.

  A later column, such as a tooltip, is another field here. A source
  that leaves it empty keeps the previous behavior.

*/

mClass(General) StringTableContents
{
public:
    BufferStringSet			columns_;
    TypeSet<TableModel::CellType>	types_;
    ManagedObjectSet<BufferStringSet>	rows_;
    BufferStringSet			icons_;
    TypeSet<OD::Color>			rowcolors_;
    BufferStringSet			rowlabels_;
};


/*!\brief TableModel of string rows copied from a domain object T.

  T is the source of truth. It is not a TableModel. This class keeps an
  owning cache of one BufferStringSet per row and answers the view from
  that cache.

  T must publicly inherit CallBacker and provide these members. The names
  are fixed. A missing or mismatched member fails when the template is
  instantiated, at the call inside this header.

  void fillStringTable( StringTableContents& ) const;
      Fill one snapshot. The contents are empty on entry. Do not fire
      stringTableChanged from this function.

  Notifier<T> stringTableChanged;
      Construct it with this. Fire it after the data shown in the table
      changes. The model then copies the snapshot again. T must outlive
      the model. Do not delete T.

  Cells are read-only. Call setBaseData before the model is given to a
  view. The view reads the column count and column types at that moment.

  A DateTime column reports DateTime. The view then installs the date-time
  delegate, and the model parses the ISO string into a QDateTime. Every
  other column reports Color, so the color delegate paints the row
  background and any icon.

*/


template <class T>
mClass(General) StringTableModel : public TableModel
{
public:
			StringTableModel();
			~StringTableModel();
			mOD_DisableCopy(StringTableModel)

    void		setBaseData(T*);
    T*			baseData()		{ return basedata_; }
    const T*		baseData() const	{ return basedata_; }

    void		pullRows();

protected:
    int			nrRows() const override;
    int			nrCols() const override;
    int			flags(int row,int col) const override;
    void		setCellData(int row,int col,const CellData&) override;
    CellData		getCellData(int row,int col) const override;
    uiString		headerText(int rowcol,OD::Orientation) const override;
    CellType		getColumnCellType(int col) const override;
    OD::Color		cellColor(int row,int col) const override;
    PixmapDesc		pixmap(int row,int col) const override;
    void		setRowCheckable(bool) override;

    void		modelChangeCB(CallBacker*);
    void		unsetBaseData();
    int			nrDataCols() const;

    T*					basedata_	= nullptr;
    ManagedObjectSet<BufferStringSet>	rows_;
    BufferStringSet			collabels_;
    BufferStringSet			rowlabels_;
    BufferStringSet			iconnames_;
    TypeSet<int>			coltypes_;
    TypeSet<OD::Color>			rowcolors_;
    bool				rowcheckable_	= false;
};


template <class T> inline
StringTableModel<T>::StringTableModel()
    : TableModel()
{
}


template <class T> inline
StringTableModel<T>::~StringTableModel()
{
    detachAllNotifiers();
    unsetBaseData();
}


template <class T> inline
void StringTableModel<T>::unsetBaseData()
{
    if ( basedata_ )
	mDetachCB( basedata_->stringTableChanged,
		   StringTableModel::modelChangeCB );

    basedata_ = nullptr;
}


template <class T> inline
void StringTableModel<T>::setBaseData( T* data )
{
    unsetBaseData();
    basedata_ = data;
    if ( basedata_ )
	mAttachCB( basedata_->stringTableChanged,
		   StringTableModel::modelChangeCB );

    pullRows();
}


template <class T> inline
void StringTableModel<T>::pullRows()
{
    beginReset();
    rows_.setEmpty();
    collabels_.setEmpty();
    rowlabels_.setEmpty();
    iconnames_.setEmpty();
    coltypes_.setEmpty();
    rowcolors_.setEmpty();
    if ( basedata_ )
    {
	StringTableContents contents;
	basedata_->fillStringTable( contents );
	collabels_ = contents.columns_;
	rowlabels_ = contents.rowlabels_;
	iconnames_ = contents.icons_;
	rowcolors_ = contents.rowcolors_;
	while ( !contents.rows_.isEmpty() )
	    rows_.add( contents.rows_.removeAndTake(0) );

	const int nrdata = nrDataCols();
	for ( int idx=0; idx<nrdata; idx++ )
	{
	    const CellType tp = contents.types_.validIdx(idx)
				? contents.types_[idx] : Text;
	    coltypes_ += sCast(int,tp);
	}
    }

    endReset();
}


template <class T> inline
void StringTableModel<T>::modelChangeCB( CallBacker* )
{
    pullRows();
}


template <class T> inline
int StringTableModel<T>::nrRows() const
{
    return rows_.size();
}


template <class T> inline
int StringTableModel<T>::nrDataCols() const
{
    int nrcols = collabels_.size();
    for ( int idx=0; idx<rows_.size(); idx++ )
    {
	const BufferStringSet* row = rows_.get( idx );
	if ( row && row->size()>nrcols )
	    nrcols = row->size();
    }

    return nrcols;
}


template <class T> inline
int StringTableModel<T>::nrCols() const
{
    return nrDataCols();
}


template <class T> inline
int StringTableModel<T>::flags( int, int col ) const
{
    int fl = ItemSelectable | ItemEnabled;
    if ( rowcheckable_ && col==0 )
	fl |= TableModel::ItemIsUserCheckable;

    return fl;
}


template <class T> inline
void StringTableModel<T>::setRowCheckable( bool yn )
{
    rowcheckable_ = yn;
    TableModel::setRowCheckable( yn );
}


template <class T> inline
void StringTableModel<T>::setCellData( int, int, const CellData& )
{}


template <class T> inline
TableModel::CellData StringTableModel<T>::getCellData( int row, int col ) const
{
    if ( !rows_.validIdx(row) )
	return CellData();

    const BufferStringSet* cells = rows_.get( row );
    if ( !cells || col<0 || !cells->validIdx(col) )
	return CellData();

    const StringView txt( cells->get(col).buf() );
    if ( getColumnCellType(col) == DateTime )
    {
	CellData cd;
	if ( !txt.isEmpty() )
	    cd.setISODateTime( txt.buf() );

	return cd;
    }

    return CellData( txt.buf() );
}


template <class T> inline
uiString StringTableModel<T>::headerText( int rowcol,
					   OD::Orientation orient ) const
{
    if ( orient == OD::Horizontal )
    {
	if ( collabels_.validIdx(rowcol) )
	    return toUiString( collabels_.get(rowcol) );

	return uiString::empty();
    }

    if ( rowlabels_.validIdx(rowcol) && !rowlabels_.get(rowcol).isEmpty() )
	return toUiString( rowlabels_.get(rowcol) );

    return toUiString( rowcol + 1 );
}


template <class T> inline
TableModel::CellType StringTableModel<T>::getColumnCellType( int col ) const
{
    if ( col>=0 && coltypes_.validIdx(col) &&
	 sCast(CellType,coltypes_[col])==DateTime )
	return DateTime;

    return Color;
}


template <class T> inline
OD::Color StringTableModel<T>::cellColor( int row, int col ) const
{
    if ( rowcolors_.validIdx(row) &&
	 rowcolors_[row] != OD::Color::NoColor() )
	return rowcolors_[row];

    if ( col == 0 )
	return sFirstColumnColor();

    return OD::Color::NoColor();
}


template <class T> inline
PixmapDesc StringTableModel<T>::pixmap( int row, int col ) const
{
    if ( col != 0 )
	return PixmapDesc();

    if ( !iconnames_.validIdx(row) || iconnames_.get(row).isEmpty() )
	return PixmapDesc();

    return PixmapDesc( iconnames_.get(row).buf(), 0, 0, OD::Color::NoColor() );
}
