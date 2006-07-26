/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2006
-*/

static const char* rcsID = "$Id: tableconv.cc,v 1.1 2006-07-26 08:32:47 cvsbert Exp $";

#include "tableconvimpl.h"
#include <sstream>


void TableImportHandler::addToCol( char c )
{
    if ( colpos_ < col_.bufSize() )
	*(col_.buf() + colpos_) = c;
    else
    {
	char buf[2]; buf[0] = c; buf[1] = '\0';
	col_ += buf;
    }
    colpos_++;
}


bool TableExportHandler::isNumber( const char* str )
{
    return isNumberString( str, NO );
}


int TableConverter::nextStep()
{
    selcolnr_ = colnr_ = 0;

    while ( true )
    {
	char c = readNewChar();
	if ( istrm_.eof() ) return false;

	handleImpState( imphndlr_.add(c) );
    }

    return true;
}


bool TableConverter::handleImpState( TableImportHandler::State impstate )
{
    switch ( impstate )
    {

    case TableImportHandler::Error:

	msg_ = imphndlr_.errMsg();

    return false;

    case TableImportHandler::EndCol: {

	if ( colSel() )
	{
	    const char* msg = exphndlr_.useColVal( selcolnr_,
						   imphndlr_.getCol() );
	    if ( msg && *msg )
	    {
		msg_ = msg;
		return false;
	    }
	    selcolnr_++;
	}
	colnr_++;
	imphndlr_.newCol();

    return true; }

    case TableImportHandler::EndRow: {

	if ( !handleImpState(TableImportHandler::EndCol) )
	    return false;

	const char* msg = exphndlr_.putRow( ostrm_ );
	if ( msg && *msg )
	{
	    msg_ = msg;
	    return false;
	}

	rowsdone_++;
	imphndlr_.newRow();

    return true; }

    default:
    break;

    }

    return true;

}


TableImportHandler::State CSVTableImportHandler::add( char c )
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
    else if ( c == ',' )
    {
	addToCol( '\0' );
	return EndCol;
    }

    addToCol( c );
    return InCol;
}


CSVTableExportHandler::CSVTableExportHandler()
    : strstrm_(*new std::ostringstream)
{
}


CSVTableExportHandler::~CSVTableExportHandler()
{
    delete &strstrm_;
}


const char* CSVTableExportHandler::useColVal( int col, const char* val )
{
    if ( col )
	strstrm_ << ',';
    const bool needsquotes = val && *val && !isNumber( val );
    if ( needsquotes )
	strstrm_ << '"';
    if ( val && *val )
	strstrm_ << val;
    if ( needsquotes )
	strstrm_ << '"';
    return 0;
}


const char* CSVTableExportHandler::putRow( std::ostream& strm )
{
    strm << strstrm_.str().c_str() << std::endl;
    strstrm_.str("");
    return strm.good() ? 0 : "Error writing to output";
}
