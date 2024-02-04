/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "tableconvimpl.h"

#include "od_iostream.h"
#include "string2.h"
#include "survinfo.h"
#include "uistrings.h"

#include <string.h>

const GlobExpr Table::RecordMatcher::emptyge_;


Table::ImportHandler::ImportHandler( od_istream& strm )
    : strm_(strm)
{}


Table::ImportHandler::~ImportHandler()
{}


char Table::ImportHandler::readNewChar() const
{
    char c = strm_.peek();
    strm_.ignore( 1 );
    if ( c == '\r' )
    {
	c = strm_.peek();
	if ( c == '\n' )
	    strm_.ignore( 1 );
	else
	    c = '\n';
    }

    return atEnd() ? '\n' : c;
}


bool Table::ImportHandler::atEnd() const
{
    return !strm_.isOK();
}


void Table::ImportHandler::addToCol( char c )
{
    if ( colpos_ < col_.bufSize()-1 )
	*(col_.getCStr() + colpos_) = c;
    else
    {
	*(col_.getCStr() + colpos_) = '\0';
	char buf[3]; buf[0] = c; buf[1] = ' '; buf[2] = '\0';
	col_ += buf;
    }

    colpos_++;
}



// Table::ExportHandler
Table::ExportHandler::ExportHandler( od_ostream& strm )
    : strm_(strm)
{}


Table::ExportHandler::~ExportHandler()
{}


bool Table::ExportHandler::isNumber( const char* str )
{
    return isNumberString( str );
}


bool Table::ExportHandler::init()
{
    if ( *prepend_.buf() )
	strm_ << prepend_;

    return strm_.isOK();
}


void Table::ExportHandler::finish()
{
    if ( *append_.buf() )
	strm_ << append_;
}


uiString Table::ExportHandler::getStrmMsg() const
{
    if ( strm_.isOK() )
	return uiString::emptyString();
    uiString ret = strm_.errMsg();
    return ret.isEmpty() ? uiStrings::phrCannotWrite(uiStrings::sOutput()) :ret;
}



// Table::Converter
Table::Converter::Converter( ImportHandler& i, ExportHandler& o )
    : Executor("Data import")
    , imphndlr_(i)
    , exphndlr_(o)
{}


Table::Converter::~Converter()
{}


#define mFinishReturn( retval )		{  exphndlr_.finish(); return retval; }

int Table::Converter::nextStep()
{
    if ( atend_ )
	mFinishReturn( Finished() );

    if ( selcolnr_ == -1 && !exphndlr_.init() )
    {
	msg_ = tr("Cannot write first output");
	mFinishReturn( ErrorOccurred() );
    }

    selcolnr_ = colnr_ = 0;

    while ( true )
    {
	char c = imphndlr_.readNewChar();
	if ( !c )
	{
	    msg_ = tr("The input file is probably not ASCII");
	    mFinishReturn( ErrorOccurred() );
	}

	Table::ImportHandler::State impstate = imphndlr_.add( c );
	if ( !handleImpState(impstate) )
	    mFinishReturn( msg_.isEmpty() ? Finished() : ErrorOccurred() );

	if ( imphndlr_.atEnd() )
	{
	    atend_ = true;
	    mFinishReturn( Finished() );
	}

	if ( impstate == Table::ImportHandler::EndRow )
	    return MoreToDo();
    }
}


bool Table::Converter::handleImpState( Table::ImportHandler::State impstate )
{
    switch ( impstate )
    {

    case Table::ImportHandler::Error:

	msg_ = mToUiStringTodo( imphndlr_.errMsg() );

    return false;

    case Table::ImportHandler::EndCol: {

	if ( colSel() )
	{
	    row_.add( imphndlr_.getCol() );
	    selcolnr_++;
	}

	colnr_++;
	imphndlr_.newCol();

    return true; }

    case Table::ImportHandler::EndRow: {

	if ( !handleImpState(Table::ImportHandler::EndCol) )
	    return false;

	bool accepted = true;
	for ( int idx=0; idx<manipulators_.size(); idx++ )
	{
	    const RowManipulator* rm = manipulators_[idx];
	    if ( rm && !rm->accept(row_) )
		{ accepted = false; break; }
	}

	if ( accepted )
	{
	    uiString msg;
	    if ( !exphndlr_.putRow(row_,msg) )
	    {
		if ( !msg.isEmpty() )
		    msg_ = msg;

		return false;
	    }
	    else
		rowsdone_++;
	}

	row_.erase();
	imphndlr_.newRow();

    return true;
				       }

    default:
    break;

    }

    return true;

}


Table::ImportHandler::State Table::WSImportHandler::add( char c )
{
    if ( c == '"' && !insingqstring_ )
	{ indoubqstring_ = !indoubqstring_; return InCol; }
    else if ( c == '\'' && !indoubqstring_ )
	{ insingqstring_ = !insingqstring_; return InCol; }
    else if ( c == '\n' )
    {
	indoubqstring_ = insingqstring_ = false;
	addToCol( '\0' );
	return EndRow;
    }
    else if ( !indoubqstring_ && !insingqstring_ && iswspace(c) )
    {
	if ( *col_.buf() )
	{
	    addToCol( '\0' );
	    return EndCol;
	}

	return InCol;
    }

    addToCol( c );
    return InCol;
}


Table::ImportHandler::State Table::CSVImportHandler::add( char c )
{
    if ( c == '"' )
	{ instring_ = !instring_; return InCol; }
    else if ( c == '\n' )
    {
	if ( instring_ )
	    c = nlreplace_;
	else
	{
	    addToCol( '\0' );
	    return EndRow;
	}
    }
    else if ( c == ',' && !instring_ )
    {
	addToCol( '\0' );
	return EndCol;
    }

    addToCol( c );
    return InCol;
}


void Table::WSExportHandler::addVal( int col, const char* inpval )
{
    if ( col )
	strm_ << od_tab;

    bool needsquotes = false;
    const bool isquotecand = !*inpval || strcspn( inpval, " \t" );
    const char quotechar = colwshanld_ == SingQuot ? '\'' : '"';

    BufferString val( inpval );
    if ( colwshanld_ != None && isquotecand )
    {
       if ( colwshanld_ >= SingQuot )
       {
	    needsquotes = true;
	    if ( firstOcc( inpval, quotechar ) )
		val.replace( quotechar, '`' );
		//TODO should in fact escape with '\\'
       }
       else
       {
	   char* ptr = val.getCStr();
	   while ( *ptr )
	   {
	       if ( *ptr == ' ' || *ptr == '\t' )
		   *ptr = '_';
	       ptr++;
	   }
       }
    }

    if ( needsquotes )
	strm_ << quotechar;
    if ( !val.isEmpty() )
	strm_ << val;
    if ( needsquotes )
	strm_ << quotechar;
}


bool Table::WSExportHandler::putRow( const BufferStringSet& row, uiString& msg )
{
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( idx, row.get(idx) );

    strm_ << od_endl;
    msg = getStrmMsg();
    return msg.isEmpty();
}


void Table::CSVExportHandler::addVal( int col, const char* val )
{
    if ( col )
	strm_ << ',';

    const bool needsquotes = *val && !isNumber( val );
    if ( needsquotes )
	strm_ << '"';
    if ( *val )
	strm_ << val;
    if ( needsquotes )
	strm_ << '"';
}


bool Table::CSVExportHandler::putRow( const BufferStringSet& row, uiString& msg)
{
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( idx, row.get(idx) );

    strm_ << od_endl;
    msg = getStrmMsg();
    return msg.isEmpty();
}


void Table::SQLInsertExportHandler::addVal( int col, const char* val )
{
    if ( col )
	strm_ << ',';

    const bool needsquotes = !*val || !isNumber( val );
    if ( needsquotes )
	strm_ << "'";
    if ( val && *val )
	strm_ << val;
    if ( needsquotes )
	strm_ << "'";
}


bool Table::SQLInsertExportHandler::putRow( const BufferStringSet& row,
					    uiString& msg )
{
    if ( nrrows_ == 0 )
    {
	if ( tblname_.isEmpty() )
	{
	    msg = tr("No table name provided");
	    return false;
	}

	addindex_ = !indexcolnm_.isEmpty();
	nrextracols_ = extracolnms_.size();
    }

    strm_ << "INSERT INTO " << tblname_;
    if ( colnms_.size() )
    {
	strm_ << " (";

	if ( addindex_ )
	    strm_ << indexcolnm_;
	else
	    strm_ << (nrextracols_ ? extracolnms_ : colnms_).get(0).buf();

	int idx0 = addindex_ ? 0 : 1;
	for ( int idx=idx0; idx<nrextracols_; idx++ )
	    strm_ << ',' << extracolnms_.get(idx).buf();

	idx0 = addindex_ || nrextracols_ > 0 ? 0 : 1;
	for ( int idx=idx0; idx<colnms_.size(); idx++ )
	    strm_ << ',' << colnms_.get(idx).buf();

	strm_ << ')';
    }

    strm_ << " VALUES (";

    if ( addindex_ )
	strm_ << startindex_ + nrrows_ * stepindex_;
    int idxoffs = addindex_ ? 1 : 0;
    for ( int idx=0; idx<nrextracols_; idx++ )
	addVal( idx+idxoffs, extracolvals_.get(idx) );

    idxoffs += nrextracols_;
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( idx+idxoffs, row.get(idx) );

    strm_ << ");" << od_endl;

    nrrows_++;
    msg = getStrmMsg();
    return msg.isEmpty();
}


bool Table::RecordMatcher::accept( BufferStringSet& cols ) const
{
    for ( int idx=0; idx<ckcols_.size(); idx++ )
    {
	const int colnr = ckcols_[idx];
	const GlobExpr& ge = colvals_.size() <= idx ? emptyge_ : colvals_[idx];
	if ( ge.matches(cols.get(colnr)) )
	{
	    if ( !any_ && not_)
		return false;
	    if ( any_ && !not_)
		return true;
	}
	else
	{
	    if ( any_ && !not_)
		return false;
	    if ( !any_ && not_)
		return true;
	}
    }

    // If it was any_, obviously we failed to find what we needed.
    // If it was not any_, apparently everything was ok.
    return !any_;
}


void Table::DuplicateKeyRemover::setPrevKeys( const BufferStringSet& cols) const
{
    if ( prevkeys_.size() < 1 )
    {
	for ( int idx=0; idx<keycols_.size(); idx++ )
	    prevkeys_.add( "" );
    }

    for ( int idx=0; idx<keycols_.size(); idx++ )
	prevkeys_.get(idx) = cols.get( keycols_[idx] );
}


bool Table::DuplicateKeyRemover::accept( BufferStringSet& cols ) const
{
    nrdone_++;
    if ( keycols_.size() < 1 )
	return true;

    if ( nrdone_ == 1 )
	{ setPrevKeys( cols ); return true; }

    for ( int idx=0; idx<keycols_.size(); idx++ )
    {
	if ( cols.get(keycols_[idx]) != prevkeys_.get(idx) )
	    { setPrevKeys( cols ); return true; }
    }

    nrremoved_++;
    return false;
}


bool Table::StartStopManipulator::isGEMatch(
	const Table::StartStopManipulator::Criterion& crit,
	const BufferStringSet& cols ) const
{
    if ( crit.matchcolidx_ >= cols.size() )
	return false;

    const int startidx = crit.matchcolidx_ < 0 ? 0 : crit.matchcolidx_;
    const int stopidx = crit.matchcolidx_ < 0 ? cols.size() : startidx + 1;
    for ( int idx=startidx; idx<stopidx; idx++ )
    {
	if ( crit.matchexpr_.matches(cols.get(idx).buf()) )
	    return true;
    }

    return false;
}


void Table::StartStopManipulator::updCount(
	const Table::StartStopManipulator::Criterion& crit,
	const BufferStringSet& cols ) const
{
    if ( crit.type_ == Criterion::Records )
	count_++;
    else
	count_ += isGEMatch( crit, cols ) ? 1 : 0;
}


bool Table::StartStopManipulator::accept( BufferStringSet& cols ) const
{
    if ( start_.type_ != Criterion::None && !startdone_ )
    {
	updCount( start_, cols );
	if ( count_ >= start_.count_ )
	    { startdone_ = true; count_ = 0; }

	return false;
    }

    if ( stop_.type_ != Criterion::None )
    {
	if ( count_ >= stop_.count_ ) // To avoid detection after end
	    return false;

	updCount( stop_, cols );
	if ( count_ >= stop_.count_ ) // To stop at detection of end
	    return false;
    }

    return true;
}

namespace Table
{

// FormatProvider
FormatProvider::FormatProvider()
{
    readSettings();
}


FormatProvider::~FormatProvider()
{}


void FormatProvider::readSettings()
{
// TODO
}


const char* FormatProvider::xy() const
{
    mDeclStaticString( ret );
    const od_uint16 width = 12;
    const od_uint16 precision = 2;
    ret.set( cformat( 'f', width, precision ) )
       .add( cformat( 'f', width, precision ) );
    return ret.buf();
}


const char* FormatProvider::z( od_uint16 extradecimals ) const
{
    mDeclStaticString( ret );
    const od_uint16 width = 12;
    const od_uint16 precision = SI().nrZDecimals() + extradecimals + 2;
    ret.set( cformat( 'f', width, precision ) );
    return ret.buf();
}


const char* FormatProvider::trcnr() const
{
    mDeclStaticString( ret );
    const od_uint16 width = 10;
    ret.set( cformat( 'd', width ) );
    return ret.buf();
}


const char* FormatProvider::spnr() const
{
    mDeclStaticString( ret );
    const od_uint16 width = 10;
    const od_uint16 precision = 2;
    ret.set( cformat( 'f', width, precision ) );
    return ret.buf();
}

const char* FormatProvider::string( od_uint16 length ) const
{
    mDeclStaticString( ret );
    ret.set( cformat( 's', length ) );
    return ret.buf();
}

} // namespace Table
