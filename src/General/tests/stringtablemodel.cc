/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "stringtablemodel.h"

#include <QDateTime>
#include <QVariant>


class StringTableStub : public CallBacker
{
public:

StringTableStub()
    : stringTableChanged(this)
{}

void fillStringTable( StringTableContents& contents ) const
{
    contents.columns_ = collabels_;
    contents.rowlabels_ = rowlabels_;
    contents.icons_ = iconnames_;
    contents.rowcolors_ = rowcolors_;
    for ( int idx=0; idx<coltypes_.size(); idx++ )
	contents.types_ += sCast(TableModel::CellType,coltypes_[idx]);

    for ( const auto* src : storage_ )
	contents.rows_.add( new BufferStringSet(*src) );
}

void addRow( const char* a )
{
    auto* row = new BufferStringSet;
    row->add( a );
    storage_.add( row );
}

void addRow( const char* a, const char* b )
{
    auto* row = new BufferStringSet;
    row->add( a );
    row->add( b );
    storage_.add( row );
}

void addRow( const char* a, const char* b, const char* c )
{
    auto* row = new BufferStringSet;
    row->add( a );
    row->add( b );
    row->add( c );
    storage_.add( row );
}

void publish()
{
    stringTableChanged.trigger();
}

    Notifier<StringTableStub>			stringTableChanged;
    ManagedObjectSet<BufferStringSet>		storage_;
    BufferStringSet				collabels_;
    BufferStringSet				rowlabels_;
    BufferStringSet				iconnames_;
    TypeSet<int>				coltypes_;
    TypeSet<OD::Color>				rowcolors_;
};


static BufferString cellText( const TableModel& model, int row, int col )
{
    return BufferString( model.getCellData(row,col).text() );
}


static bool testPull()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "One" );
    stub.addRow( "Beta", "Two" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );
    TableModel& mdl = model;

    mRunStandardTest( mdl.nrRows()==2, "Pull sets the row count" );
    mRunStandardTest( cellText(mdl,0,0)=="Alpha", "First row, first column" );
    mRunStandardTest( cellText(mdl,0,1)=="One", "First row, second column" );
    mRunStandardTest( cellText(mdl,1,0)=="Beta", "Second row, first column" );
    mRunStandardTest( cellText(mdl,1,1)=="Two", "Second row, second column" );
    return true;
}


static bool testNotify()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "One" );
    stub.addRow( "Beta", "Two" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );

    stub.storage_.setEmpty();
    stub.addRow( "Only", "Row" );
    stub.publish();

    TableModel& mdl = model;
    mRunStandardTest( mdl.nrRows()==1,
		      "stringTableChanged updates the row count" );
    mRunStandardTest( cellText(mdl,0,0)=="Only",
		      "stringTableChanged updates text" );
    mRunStandardTest( cellText(mdl,0,1)=="Row",
		      "stringTableChanged updates the second column" );
    return true;
}


static bool testNull()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "One" );
    stub.collabels_.add( "Name" );
    stub.rowlabels_.add( "Top" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );
    model.setBaseData( nullptr );
    stub.publish();

    TableModel& mdl = model;
    mRunStandardTest( mdl.nrRows()==0, "Null base yields an empty table" );
    mRunStandardTest( mdl.nrCols()==0, "Null base clears the column labels" );
    mRunStandardTest( mdl.headerText(0,OD::Horizontal).isEmpty(),
		      "Null base clears the horizontal header" );
    mRunStandardTest( mdl.headerText(0,OD::Vertical).getString()=="1",
		      "Null base clears the row labels" );
    return true;
}


static bool testLabels()
{
    StringTableStub stub;
    stub.addRow( "Alpha" );
    stub.collabels_.add( "Name" );
    stub.collabels_.add( "Kind" );
    stub.collabels_.add( "Extra" );
    stub.collabels_.add( "More" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );

    TableModel& mdl = model;
    mRunStandardTest( mdl.nrCols()==4, "Column count follows the labels" );
    mRunStandardTest( mdl.headerText(0,OD::Horizontal).getString()=="Name",
		      "Horizontal header is the first label" );

    stub.collabels_.setEmpty();
    stub.collabels_.add( "Title" );
    stub.publish();
    mRunStandardTest( mdl.nrCols()==1,
		      "stringTableChanged updates the column count" );
    mRunStandardTest( mdl.headerText(0,OD::Horizontal).getString()=="Title",
		      "stringTableChanged updates the column label" );
    return true;
}


static bool testRowLabels()
{
    StringTableStub stub;
    stub.addRow( "Alpha" );
    stub.addRow( "Beta" );
    stub.rowlabels_.add( "Top" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );

    TableModel& mdl = model;
    mRunStandardTest( mdl.headerText(0,OD::Vertical).getString()=="Top",
		      "Vertical header uses the row label" );
    mRunStandardTest( mdl.headerText(1,OD::Vertical).getString()=="2",
		      "A missing row label falls back to row+1" );
    return true;
}


static bool testJagged()
{
    StringTableStub stub;
    stub.addRow( "Short" );
    stub.addRow( "A", "B", "C" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );
    TableModel& mdl = model;

    mRunStandardTest( mdl.nrCols()==3, "Column count follows the longest row" );
    mRunStandardTest( cellText(mdl,0,2).isEmpty(),
		      "A short row has an empty trailing cell" );
    mRunStandardTest( cellText(mdl,1,2)=="C",
		      "The long row keeps its last cell" );
    return true;
}


static bool testRowColor()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "One" );
    stub.addRow( "Beta", "Two" );
    const OD::Color bad = OD::Color::Red().lighter( 0.9f );
    stub.rowcolors_ += bad;
    stub.rowcolors_ += OD::Color::NoColor();

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );
    TableModel& mdl = model;

    mRunStandardTest( mdl.cellColor(0,0)==bad, "Bad row background" );
    mRunStandardTest( mdl.cellColor(0,1)==bad,
		      "Bad row background covers every column" );
    const OD::Color namecol = sFirstColumnColor();
    mRunStandardTest( mdl.cellColor(1,0)==namecol,
		      "First column has the default name background" );
    mRunStandardTest( mdl.cellColor(1,1)==OD::Color::NoColor(),
		      "Other columns keep the stripe" );

    stub.rowcolors_.setEmpty();
    stub.rowcolors_ += OD::Color::NoColor();
    stub.publish();
    mRunStandardTest( mdl.cellColor(0,0)==namecol,
		      "stringTableChanged clears a row color back to "
		      "the name column" );
    return true;
}


static bool testIcon()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "One" );
    stub.addRow( "Beta", "Two" );
    stub.iconnames_.add( "seis" );
    stub.iconnames_.add( "" );

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );
    TableModel& mdl = model;

    mRunStandardTest( mdl.pixmap(0,0).source_=="seis",
		      "Icon on the first data column" );
    mRunStandardTest( !mdl.pixmap(0,1).isValid(),
		      "Other columns have no icon" );
    mRunStandardTest( !mdl.pixmap(1,0).isValid(),
		      "An empty icon name draws nothing" );
    return true;
}


static bool testDateTimeColumn()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "2026-11-23T09:22:00" );
    stub.coltypes_ += sCast(int,TableModel::Text);
    stub.coltypes_ += sCast(int,TableModel::DateTime);

    StringTableModel<StringTableStub> model;
    model.setBaseData( &stub );
    TableModel& mdl = model;

    mRunStandardTest( mdl.getColumnCellType(0)==TableModel::Color,
		      "Text columns use the color delegate" );
    mRunStandardTest( mdl.getColumnCellType(1)==TableModel::DateTime,
		      "A DateTime column uses the date-time delegate" );
    const TableModel::CellData cd = mdl.getCellData( 0, 1 );
    mRunStandardTest( cd.qvar_.toDateTime().isValid(),
		      "DateTime column stores a time" );
    return true;
}


static bool testDetach()
{
    StringTableStub stub;
    stub.addRow( "Alpha", "One" );

    auto* model = new StringTableModel<StringTableStub>;
    model->setBaseData( &stub );
    delete model;
    stub.publish();
    mRunStandardTest( true, "Deleting the model detaches stringTableChanged" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testPull() ||
	 !testNotify() ||
	 !testNull() ||
	 !testLabels() ||
	 !testRowLabels() ||
	 !testJagged() ||
	 !testRowColor() ||
	 !testIcon() ||
	 !testDateTimeColumn() ||
	 !testDetach() )
	return 1;

    return 0;
}
