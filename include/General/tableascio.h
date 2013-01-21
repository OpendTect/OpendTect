#ifndef tableascio_h
#define tableascio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Nov 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstringset.h"
#include "repos.h"
#include <iosfwd>
class IOPar;
class UnitOfMeasure;

namespace Table
{

class FormatDesc;
class ImportHandler;
class ExportHandler;
class Converter;
class FileFormatRepository;

mGlobal(General) FileFormatRepository& FFR();


/*!\brief Ascii I/O using Format Description.

  The idea is to create a subclass of AscIO which synthesises an object
  from the Selection of a Table::FormatDesc. Or, in the case of export,
  outputs the data according to the Selection object.
 
 */

mExpClass(General) AscIO
{
public:

				AscIO( const FormatDesc& fd )
				    : fd_(fd)
				    , imphndlr_(0)
				    , exphndlr_(0)
				    , cnvrtr_(0)
				    , needfullline_(false)
				    , hdrread_(false) { units_.allowNull(true);}
    virtual			~AscIO();

    const FormatDesc&		desc() const		{ return fd_; }
    const char*			errMsg() const		{ return errmsg_.str(); }

protected:

    const FormatDesc&		fd_;
    mutable BufferString	errmsg_;
    BufferStringSet		vals_;
    ObjectSet<const UnitOfMeasure> units_;
    ImportHandler*		imphndlr_;
    ExportHandler*		exphndlr_;
    Converter*			cnvrtr_;
    mutable bool		hdrread_;
    bool			needfullline_;
    BufferStringSet		fullline_;

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
    int				getIntValue(int,int udf=mUdf(int)) const;
    float			getfValue(int,float udf=mUdf(float)) const;
    double			getdValue(int,double udf=mUdf(double)) const;
    				// For more, use Conv:: stuff

    int				formOf(bool hdr,int iinf) const;
    int				columnOf(bool hdr,int iinf,int ielem) const;

};


/*!\brief Holds system- and user-defined formats for different data types
  ('groups') */

mExpClass(General) FileFormatRepository
{
public:

    void		getGroups(BufferStringSet&) const;
    void		getFormats(const char* grp,BufferStringSet&) const;

    const IOPar*	get(const char* grp,const char* nm) const;
    void		set(const char* grp,const char* nm,
	    		    IOPar*,Repos::Source);
			    //!< IOPar* will become mine; set to null to remove

    bool		write(Repos::Source) const;

protected:

    			FileFormatRepository();
    void		addFromFile(const char*,Repos::Source);
    const char*		grpNm(int) const;
    int			gtIdx(const char*,const char*) const;

    struct Entry
    {
			Entry( Repos::Source src, IOPar* iop )
			    : iopar_(iop), src_(src)	{}
			~Entry();

	IOPar*		iopar_;
	Repos::Source	src_;
    };

    ObjectSet<Entry>	entries_;

    mGlobal(General) friend FileFormatRepository& FFR();

};


}; // namespace Table


#endif

