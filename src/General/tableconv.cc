/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2006
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "tableconvimpl.h"
#include "string2.h"

const GlobExpr Table::RecordMatcher::emptyge_;


void Table::ImportHandler::addToCol( char c )
{
    if ( colpos_ < col_.bufSize()-1 )
	*(col_.buf() + colpos_) = c;
    else
    {
	*(col_.buf() + colpos_) = '\0';
	char buf[3]; buf[0] = c; buf[1] = ' '; buf[2] = '\0';
	col_ += buf;
    }

    colpos_++;
}


bool Table::ExportHandler::isNumber( const char* str )
{
    return isNumberString( str );
}


bool Table::ExportHandler::init()
{
    if ( *prepend_.buf() )
	strm_ << prepend_;

    return strm_.good();
}


void Table::ExportHandler::finish()
{
    if ( *append_.buf() )
	strm_ << append_;
}


int Table::Converter::nextStep()
{
    if ( atend_ )
	{ exphndlr_.finish(); return Finished(); }

    if ( selcolnr_ == -1 && !exphndlr_.init() )
    {
	msg_ = "Cannot write first output";
	return ErrorOccurred();
    }

    selcolnr_ = colnr_ = 0;

    while ( true )
    {
	char c = imphndlr_.readNewChar();

	Table::ImportHandler::State impstate = imphndlr_.add( c );
	if ( !handleImpState(impstate) )
	    return msg_.isEmpty() ? Finished() : ErrorOccurred();

	if ( imphndlr_.atEnd() )
	    atend_ = true;
	if ( impstate == Table::ImportHandler::EndRow )
	    return MoreToDo();
    }
}


bool Table::Converter::handleImpState( Table::ImportHandler::State impstate )
{
    switch ( impstate )
    {

    case Table::ImportHandler::Error:

	msg_ = imphndlr_.errMsg();

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
	    const char* msg = exphndlr_.putRow( row_ );
	    if ( msg )
	    {
		if ( *msg )
		    msg_ = msg;

		return false;
	    }

	    if ( !msg )
		rowsdone_++;
	}

	row_.erase();
	imphndlr_.newRow();

    return true; }

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
    else if ( !indoubqstring_ && !insingqstring_ && isspace(c) )
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


void Table::WSExportHandler::addVal( int col, const char* val )
{
    if ( col )
	strm_ << '\t';

    bool needsquotes = false;
    const bool isquotecand = !*val || strcspn( val, " \t" );
    const char quotechar = colwshanld_ == SingQuot ? '\'' : '"';

    if ( colwshanld_ != None && isquotecand )
    {
       if ( colwshanld_ >= SingQuot )
       {
	    needsquotes = true;
	    if ( strchr( val, quotechar ) )
		replaceCharacter( (char*)val, quotechar, '`' );
	    	//TODO should in fact escape with '\\'
       }
       else
       {
	   char* ptr = (char*)val;
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
    if ( *val )
	strm_ << val;
    if ( needsquotes )
	strm_ << quotechar;
}


const char* Table::WSExportHandler::putRow( const BufferStringSet& row )
{
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( idx, row.get(idx) );

    strm_ << std::endl;
    return getStrmMsg();
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


const char* Table::CSVExportHandler::putRow( const BufferStringSet& row )
{
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( idx, row.get(idx) );

    strm_ << std::endl;
    return getStrmMsg();
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


const char* Table::SQLInsertExportHandler::putRow( const BufferStringSet& row )
{
    if ( nrrows_ == 0 )
    {
	if ( tblname_.isEmpty() )
	    return "No table name provided";

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

    strm_ << ");" << std::endl;

    nrrows_++;
    return getStrmMsg();
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
