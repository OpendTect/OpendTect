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

#include "ranges.h"
#include "bufstringset.h"
#include "tableascio.h"
#include <iosfwd>

namespace Table { class FormatDesc; }
class UnitOfMeasure;


namespace Well
{
class Data;
class Track;
class D2TModel;
class MarkerSet;


mClass LASImporter
{
public:

			LASImporter( Data& d ) : wd(d), useconvs_(false) {}
			~LASImporter();

    mClass FileInfo
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
    };

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

    Data&		wd;

    mutable BufferStringSet	unitmeasstrs_;
    mutable ObjectSet<const UnitOfMeasure>	convs_;
    bool		useconvs_;

    void		parseHeader(char*,char*&,char*&,char*&) const;
    const char*		getLogData(std::istream&,const BoolTypeSet&,
	    			   const FileInfo&,bool,int,int);

};


mClass TrackAscIO : public Table::AscIO
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


mClass D2TModelAscIO : public Table::AscIO
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


mClass MarkerSetAscIO : public Table::AscIO
{
public:
    			MarkerSetAscIO( const Table::FormatDesc& fd )
			    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc();

    bool		get(std::istream&,MarkerSet&,const Track&) const;

};


}; // namespace Well

#endif
