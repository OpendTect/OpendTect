#ifndef tableascio_h
#define tableascio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2006
 RCS:		$Id: tableascio.h,v 1.3 2006-11-07 17:51:31 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include <iosfwd>

namespace Table
{

class FormatDesc;
class ImportHandler;
class ExportHandler;
class Converter;

/*!\brief Ascii I/O using Format Description */

class AscIO
{
public:

				AscIO( const Table::FormatDesc& fd )
				    : fd_(fd)
				    , imphndlr_(0)
				    , exphndlr_(0)
				    , cnvrtr_(0)	{}
    virtual			~AscIO();

    const Table::FormatDesc&	desc() const		{ return fd_; }
    const char*			errMsg() const		{ return errmsg_; }

protected:

    const Table::FormatDesc&	fd_;
    mutable BufferString	errmsg_;
    BufferStringSet		vals_;
    ImportHandler*		imphndlr_;
    ExportHandler*		exphndlr_;
    Converter*			cnvrtr_;

    friend class		AscIOImp_ExportHandler;
    friend class		AscIOExp_ImportHandler;

    void			addVal( const char* s ) const
				{ const_cast<AscIO*>(this)->vals_.add(s); }
    bool			getHdrVals(std::istream&) const;
    int				getNextBodyVals(std::istream&) const;
    				//!< Executor convention
    bool			putHdrVals(std::ostream&) const;
    bool			putNextBodyVals(std::ostream&) const;

};


}; // namespace Table


#endif
