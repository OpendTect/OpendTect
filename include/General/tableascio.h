#ifndef tableascio_h
#define tableascio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Nov 2006
 RCS:		$Id: tableascio.h,v 1.4 2006-12-22 10:53:26 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"
#include <iosfwd>
class UnitOfMeasure;

namespace Table
{

class FormatDesc;
class ImportHandler;
class ExportHandler;
class Converter;

/*!\brief Ascii I/O using Format Description.

  The idea is to create a subclass of AscIO which synthesises an object
  from the Selection of a Table::FormatDesc. Or, in the case of export,
  outputs the data according to the Selection object.
 
 */

class AscIO
{
public:

				AscIO( const Table::FormatDesc& fd )
				    : fd_(fd)
				    , imphndlr_(0)
				    , exphndlr_(0)
				    , cnvrtr_(0) { units_.allowNull(true);}
    virtual			~AscIO();

    const Table::FormatDesc&	desc() const		{ return fd_; }
    const char*			errMsg() const		{ return errmsg_; }

protected:

    const Table::FormatDesc&	fd_;
    mutable BufferString	errmsg_;
    BufferStringSet		vals_;
    ObjectSet<const UnitOfMeasure> units_;
    ImportHandler*		imphndlr_;
    ExportHandler*		exphndlr_;
    Converter*			cnvrtr_;

    friend class		AscIOImp_ExportHandler;
    friend class		AscIOExp_ImportHandler;

    void			emptyVals() const;
    void			addVal(const char*,const UnitOfMeasure*) const;
    bool			getHdrVals(std::istream&) const;
    int				getNextBodyVals(std::istream&) const;
    				//!< Executor convention
    bool			putHdrVals(std::ostream&) const;
    bool			putNextBodyVals(std::ostream&) const;

    const char*			text(int) const; // Never returns null
    int				getIntValue(int) const;
    float			getfValue(int) const;
    double			getdValue(int) const;
    				// For more, use Conv:: stuff

};


}; // namespace Table


#endif
