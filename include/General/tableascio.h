#ifndef tableascio_h
#define tableascio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2006
 RCS:		$Id: tableascio.h,v 1.2 2006-11-07 12:26:27 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include <iosfwd>

namespace Table
{

class FormatDesc;

/*!\brief Ascii I/O using Format Description */

class AscIO
{
public:

				AscIO( const Table::FormatDesc& fd )
				    : fd_(fd)		{}

    const Table::FormatDesc&	desc() const		{ return fd_; }
    const char*			errMsg() const		{ return errmsg_; }

protected:

    const Table::FormatDesc&	fd_;
    mutable BufferString	errmsg_;
    BufferStringSet		vals_;

    friend class		AscIOImp_ExportHandler;
    friend class		AscIOExp_ImportHandler;

    bool			getHdrVals(std::istream&) const;
    bool			getNextBodyVals(std::istream&) const;
    bool			putHdrVals(std::ostream&) const;
    bool			putNextBodyVals(std::ostream&) const;

};


}; // namespace Table


#endif
