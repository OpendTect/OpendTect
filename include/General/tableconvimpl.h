#ifndef tableconvimpl_h
#define tableconvimpl_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Jul 2006
 RCS:		$Id: tableconvimpl.h,v 1.2 2006-07-26 10:52:09 cvsbert Exp $
________________________________________________________________________

-*/

#include "tableconv.h"
#include "bufstringset.h"


class CSVTableImportHandler : public TableImportHandler
{
public:
    			CSVTableImportHandler()
			    : nlreplace_('\n')
			    , instring_(false)	{}

    State		add(char);
    const char*		getCol() const		{ return col_.buf(); }
    const char*		errMsg() const		{ return col_.buf(); }

    virtual void	newRow()		{ instring_ = false; }

    char		nlreplace_;	//!< newlines in strings: replace with

protected:

    bool		instring_;

};


class CSVTableExportHandler : public TableExportHandler
{
public:
    			CSVTableExportHandler();
			~CSVTableExportHandler();

    const char*		useColVal(int col,const char*);
    const char*		putRow(std::ostream&);

    std::ostringstream&	strstrm_;

};


class SQLInsertTableExportHandler : public TableExportHandler
{
public:
    			SQLInsertTableExportHandler();
			~SQLInsertTableExportHandler();

    const char*		useColVal(int col,const char*);
    const char*		putRow(std::ostream&);

    std::ostringstream&	strstrm_;
    BufferString	tblname_;
    BufferStringSet	fields_;

};


#endif
