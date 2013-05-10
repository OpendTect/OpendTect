#ifndef wellimpasc_h
#define wellimpasc_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellmod.h"
#include "bufstringset.h"
#include "ranges.h"
#include "tableascio.h"
#include <iosfwd>

namespace Table { class FormatDesc; }
class Coord3;
class UnitOfMeasure;


namespace Well
{
class Data;
class Track;
class D2TModel;
class MarkerSet;

/*!
\brief Imports files in LAS ( Log Ascii Standard ) format.
*/

mExpClass(Well) LASImporter
{
public:

			LASImporter( Data& d ) : wd_(&d), useconvs_(false)   {}
			LASImporter()	       : wd_(0), useconvs_(false)   {}
			~LASImporter();

    mExpClass(Well) FileInfo
    {
    public:
			FileInfo()
			    : zrg(mUdf(float),mUdf(float))
			    , depthcolnr(-1)
			    , revz(false)
			    , undefval(-999.25)	{}
			~FileInfo()		{ deepErase(lognms); }

	BufferStringSet	lognms;
	Interval<float>	zrg;
	bool		revz;
	int		depthcolnr;
	float		undefval;
	BufferString	zunitstr;

	BufferString	wellnm; //!< only info; not used by getLogs
	BufferString	uwi; //!< only info, not used by getLogs
    };

    void		setData( Data* wd )	    { wd_ = wd; }
    const char*		getLogInfo(const char* lasfnm,FileInfo&) const;
    const char*		getLogInfo(std::istream& lasstrm,FileInfo&) const;
    const char*		getLogs(const char* lasfnm,const FileInfo&,
	    			bool istvd=true);
    const char*		getLogs(std::istream& lasstrm,const FileInfo&,
	    			bool istvd=true);

    bool		willConvertToSI() const		{ return useconvs_; }
    			//!< Note that depth is always converted
    void		setConvertToSI( bool yn )	{ useconvs_ = yn; }
    			//!< Note that depth is always converted

protected:

    Data*		wd_;

    mutable BufferStringSet	unitmeasstrs_;
    mutable ObjectSet<const UnitOfMeasure>	convs_;
    bool		useconvs_;

    void		parseHeader(char*,char*&,char*&,char*&) const;
    const char*		getLogData(std::istream&,const BoolTypeSet&,
	    			   const FileInfo&,bool,int,int);

};


/*!
\brief Track Ascii I/O.
*/

mExpClass(Well) TrackAscIO : public Table::AscIO
{
public:
    				TrackAscIO( const Table::FormatDesc& fd,
					   std::istream& strm )
				    : Table::AscIO(fd)
	      		    	    , strm_(strm)	{}

    static Table::FormatDesc*	getDesc();
    bool 			getData(Data&,bool first_is_surface) const;

protected:

    std::istream&		strm_;

};


/*!
\brief D2TModel Ascii I/O.
*/

mExpClass(Well) D2TModelAscIO : public Table::AscIO
{   
    public:
				D2TModelAscIO( const Table::FormatDesc& fd )
				: Table::AscIO(fd)          {}

    static Table::FormatDesc*   getDesc(bool withunitfld);
    static void                 updateDesc(Table::FormatDesc&,bool withunitfld);
    static void                 createDescBody(Table::FormatDesc*,bool unitfld);

    bool                        get(std::istream&,Well::D2TModel&,
	    			    const Well::Data&) const;
};


/*!
\brief MarkerSet Ascii I/O.
*/

mExpClass(Well) MarkerSetAscIO : public Table::AscIO
{
public:
    			MarkerSetAscIO( const Table::FormatDesc& fd )
			    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc();

    bool		get(std::istream&,MarkerSet&,const Track&) const;

};


/*!
\brief Bulk WellTrack Ascii I/O.
*/

mExpClass(Well) BulkTrackAscIO : public Table::AscIO
{
public:
			BulkTrackAscIO(const Table::FormatDesc&,std::istream&);

    static Table::FormatDesc*	getDesc();
    bool			get(BufferString& wellnm,Coord3& crd,float md,
				    BufferString& uwi) const;

protected:
    std::istream&	strm_;
};


/*!
\brief Bulk MarkerSet Ascii I/O.
*/

mExpClass(Well) BulkMarkerAscIO : public Table::AscIO
{
public:
			BulkMarkerAscIO(const Table::FormatDesc&,std::istream&);

    static Table::FormatDesc*	getDesc();
    bool			get(BufferString& wellnm,float md,
				    BufferString& uwi) const;

protected:
    std::istream&	strm_;
};

} // namespace Well

#endif
