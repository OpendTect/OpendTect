#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstringset.h"
#include "coordsystem.h"
#include "repos.h"
#include "survinfo.h"
#include "od_iosfwd.h"
#include "uistring.h"
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
{ mODTextTranslationClass(AscIO);
public:

				AscIO( const FormatDesc& fd )
				    : fd_(fd)
				    , imphndlr_(0)
				    , exphndlr_(0)
				    , cnvrtr_(0)
				    , needfullline_(false)
				    , hdrread_(false) { units_.allowNull(true);}
    virtual			~AscIO();

    const FormatDesc&		desc() const	{ return fd_; }
    uiString			errMsg() const	{ return errmsg_; }
    uiString			warnMsg() const { return warnmsg_; }

protected:

    const FormatDesc&		fd_;
    mutable uiString		errmsg_;
    mutable uiString		warnmsg_;
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
    bool			getHdrVals(od_istream&) const;
    int				getNextBodyVals(od_istream&) const;
				//!< Executor convention
    bool			putHdrVals(od_ostream&) const;
    bool			putNextBodyVals(od_ostream&) const;

    BufferString		getText(int) const;
    int				getIntValue(int,int udf=mUdf(int)) const;
    float			getFValue(int,float udf=mUdf(float)) const;
    double			getDValue(int,double udf=mUdf(double)) const;
				// For more, use Conv:: stuff

    Coord			 getPos(int xfld,int yfld,
					double udf=mUdf(double),bool isll=false,
					ConstRefMan<Coords::CoordSystem> crs=
					SI().getCoordSystem()) const;
				//Return CRS converted Coord
    Coord3			 getPos3D(int xfld,int yfld, int zfld,
					  double udf=mUdf(double),
					  bool isll=false,
					  ConstRefMan<Coords::CoordSystem> crs=
					  SI().getCoordSystem()) const;
    BinID			 getBinID(int inlfld,int crlfld,
					  double udf=mUdf(double)) const;

    int				formOf(bool hdr,int iinf) const;
    int				columnOf(bool hdr,int iinf,int ielem) const;

public:
    mDeprecated			("Use getFValue")
    float			getfValue(int idx,float udf=mUdf(float)) const
				{ return getFValue( idx, udf ); }
    mDeprecated			("Use getDValue")
    double			getdValue(int idx,double udf=mUdf(double)) const
				{ return getDValue( idx, udf ); }
    mDeprecated			("Use getText")
    const char*			text(int ifld) const { return getText(ifld); }
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
    BufferString	grpNm(int) const;
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


} // namespace Table
