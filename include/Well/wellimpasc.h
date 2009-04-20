#ifndef wellimpasc_h
#define wellimpasc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellimpasc.h,v 1.21 2009-04-20 13:29:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "strmdata.h"
#include "bufstringset.h"
#include "tableascio.h"

#include <iosfwd>

namespace Table { class FormatDesc; }
class UnitOfMeasure;


namespace Well
{
class Data;

mClass AscImporter
{
public:

			AscImporter( Data& d ) : wd(d), useconvs_(false) {}
			~AscImporter();
    mClass D2TModelInfo
    {
    public:
			D2TModelInfo();

	BufferString	fname_;
	bool		istwt_;
	bool		istvd_;
	bool		zinft_;

	float		vel_;	// used if fname_.isEmpty()
    };

    const char*		getD2T(const D2TModelInfo&);
    const char*		getMarkers(const char*,bool istvd,
	    			   bool depthinfeet);
    Data&		getWellData() 				{ return wd; }

    mClass LasFileInfo
    {
    public:
			LasFileInfo()
			    : zrg(mUdf(float),mUdf(float))
			    , depthcolnr(-1)
			    , undefval(-999.25)	{}
			~LasFileInfo()		{ deepErase(lognms); }

	BufferStringSet	lognms;
	Interval<float>	zrg;
	int		depthcolnr;
	float		undefval;
	BufferString	zunitstr;

	BufferString	wellnm; //!< only info; not used by getLogs
    };

    const char*		getLogInfo(const char* lasfnm,LasFileInfo&) const;
    const char*		getLogInfo(std::istream& lasstrm,LasFileInfo&) const;
    const char*		getLogs(const char* lasfnm,const LasFileInfo&,
	    			bool istvd=true);
    const char*		getLogs(std::istream& lasstrm,const LasFileInfo&,
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
	    			   const LasFileInfo&,bool,int,int);

};


mClass WellAscIO : public Table::AscIO
{
public:
    				WellAscIO( const Table::FormatDesc& fd,
					   std::istream& strm )
				    : Table::AscIO(fd)
	      		    	    , strm_(strm)	{}

    static Table::FormatDesc*	getDesc();
    bool 			getData(Data&,bool first_is_surface) const;

protected:
    std::istream&		strm_;
};

}; // namespace Well

#endif
