#ifndef wellimpasc_h
#define wellimpasc_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellimpasc.h,v 1.10 2004-03-01 14:35:26 nanne Exp $
________________________________________________________________________


-*/

#include "ranges.h"
#include "strmdata.h"
#include "bufstringset.h"
#include <iosfwd>

class UnitOfMeasure;


namespace Well
{
class Data;

class AscImporter
{
public:

			AscImporter( Data& d ) : wd(d), useconvs_(false) {}
			~AscImporter();

    const char*		getTrack(const char*,bool first_is_surface,
	    			 bool depthinfeet=false);
    const char*		getD2T(const char*,bool istvd,bool depthinfeet=false);

    class LasFileInfo
    {
    public:
			LasFileInfo()
			    : zrg(mUndefValue,mUndefValue)
			    , undefval(-999.25)	{}
			~LasFileInfo()		{ deepErase(lognms); }

	BufferStringSet	lognms;
	Interval<float>	zrg;
	float		undefval;
	BufferString	zunitstr;

	BufferString	wellnm; //!< only info; not used by getLogs
    };

    const char*		getLogInfo(const char* lasfnm,LasFileInfo&) const;
    const char*		getLogInfo(istream& lasstrm,LasFileInfo&) const;
    const char*		getLogs(const char* lasfnm,const LasFileInfo&,
	    			bool istvd=true);
    const char*		getLogs(istream& lasstrm,const LasFileInfo&,
	    			bool istvd=true);

    bool		willConvertToSI() const		{ return useconvs_; }
    			//!< Note that depth is always converted
    void		setConvertToSI( bool yn )	{ useconvs_ = yn; }
    			//!< Note that depth is always converted

protected:

    Data&		wd;

    mutable BufferStringSet	unitmeasstrs;
    mutable ObjectSet<const UnitOfMeasure>	convs;
    bool		useconvs_;

    void		parseHeader(char*,char*&,char*&,char*&) const;

};

}; // namespace Well

#endif
