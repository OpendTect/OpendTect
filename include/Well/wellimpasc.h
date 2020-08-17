#pragma once

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
#include "coord.h"
#include "ranges.h"
#include "tableascio.h"
#include "typeset.h"
#include "od_iosfwd.h"
#include "uistring.h"

namespace Table { class FormatDesc; }
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
{ mODTextTranslationClass(LASImporter)
public:

			LASImporter( Data& d ) : wd_(&d), useconvs_(false)   {}
			LASImporter()	       : wd_(0), useconvs_(false)   {}
			~LASImporter();

    mExpClass(Well) FileInfo
    { mODTextTranslationClass(FileInfo)
    public:
			~FileInfo()		{}

	int		size() const		{ return lognms.size(); }

	BufferStringSet	logcurves;
	BufferStringSet	logunits;
	BufferStringSet	lognms;
	Interval<float>	zrg = Interval<float>::udf();
	bool		revz = false;
	int		depthcolnr = -1;
	float		undefval = -999.25f;
	BufferString	zunitstr;

			//!< only info; not used by getLogs
	BufferString	comp_;
	BufferString	wellnm;
	BufferString	state_;
	BufferString	county_;
	BufferString	srvc_;
	BufferString	uwi;
	Coord		loc_ = Coord::udf();
	double		kbelev_ = mUdf(double);
	double		glelev_ = mUdf(double);
    };

    void		setData( Data* wd )	    { wd_ = wd; }
    void		copyInfo(const FileInfo&,bool& changed);
    void		adjustTrack(const Interval<float>& zrg,bool istvdss,
				    bool& changed);
    const char*		getLogInfo(const char* lasfnm,FileInfo&) const;
    const char*		getLogInfo(od_istream& lasstrm,FileInfo&) const;
    const char*		getLogs(const char* lasfnm,const FileInfo&,
				bool istvd=true,bool usecurvenms=false);
    const char*		getLogs(od_istream& lasstrm,const FileInfo&,
				bool istvd=true,bool usecurvenms=false);

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
    static void		parseLocation(const char*,const char*,Coord&);
    const char*		getLogData(od_istream&,const BoolTypeSet&,
				   const FileInfo&,bool,int,int);

};


/*!
\brief Track Ascii I/O.
*/

mExpClass(Well) TrackAscIO : public Table::AscIO
{ mODTextTranslationClass(TrackAscIO)
public:
				TrackAscIO( const Table::FormatDesc& fd,
					   od_istream& strm )
				    : Table::AscIO(fd)
				    , strm_(strm)	{}

    static Table::FormatDesc*	getDesc();
    bool			getData(Data&,float kbelev=mUdf(float),
					float td=mUdf(float)) const;

protected:

    od_istream&			strm_;
    bool			readTrackData(TypeSet<Coord3>&,
					      TypeSet<double>&,
					      double& kbelevinfile) const;
    bool			computeMissingValues(TypeSet<Coord3>&,
						     TypeSet<double>&,
						    double& kbelevinfile) const;
    bool			adjustSurfaceLocation(TypeSet<Coord3>&,
						     Coord& surfacecoord) const;

};


/*!
\brief D2TModel Ascii I/O.
*/

mExpClass(Well) D2TModelAscIO : public Table::AscIO
{ mODTextTranslationClass(D2TModelAscIO)
    public:
				D2TModelAscIO( const Table::FormatDesc& fd )
				: Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc(bool withunitfld);
    static void			updateDesc(Table::FormatDesc&,bool withunitfld);
    static void			createDescBody(Table::FormatDesc*,bool unitfld);

    bool			get(od_istream&,Well::D2TModel&,
				    const Well::Data&) const;
};


/*!
\brief MarkerSet Ascii I/O.
*/

mExpClass(Well) MarkerSetAscIO : public Table::AscIO
{ mODTextTranslationClass(MarkerSetAscIO)
public:
				MarkerSetAscIO( const Table::FormatDesc& fd )
				: Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc();

    bool			get(od_istream&,MarkerSet&,const Track&) const;

};


/*!
\brief Bulk WellTrack Ascii I/O.
*/

mExpClass(Well) BulkTrackAscIO : public Table::AscIO
{ mODTextTranslationClass(BulkTrackAscIO)
public:
				BulkTrackAscIO(const Table::FormatDesc&,
					       od_istream&);

    static Table::FormatDesc*	getDesc();
    bool			get(BufferString& wellnm,Coord3& crd,float& md,
				    BufferString& uwi) const;
    bool			depthIsTVD() const;

protected:

    od_istream&	strm_;

};


/*!
\brief Bulk MarkerSet Ascii I/O.
*/

mExpClass(Well) BulkMarkerAscIO : public Table::AscIO
{ mODTextTranslationClass(BulkMarkerAscIO)
public:
			BulkMarkerAscIO(const Table::FormatDesc&,od_istream&);

    static Table::FormatDesc*	getDesc();
    bool			get(BufferString& wellnm,
				    float& md,BufferString& markernm) const;
    bool			identifierIsUWI() const;

protected:

    od_istream&	strm_;

};


/*!
\brief Bulk D2TModel Ascii I/O.
*/

mExpClass(Well) BulkD2TModelAscIO : public Table::AscIO
{ mODTextTranslationClass(BulkD2TModelAscIO)
public:
				BulkD2TModelAscIO(const Table::FormatDesc&,
						  od_istream&);
				~BulkD2TModelAscIO();

    static Table::FormatDesc*	getDesc();
    bool			get(BufferString& wellnm,float& md,float& twt);
    bool			identifierIsUWI() const;

protected:

    od_istream&			strm_;
    BufferStringSet		wells_;
    ObjectSet<Data>		wellsdata_;
};


/*!
\brief Directional survey Ascii I/O.
*/

mExpClass(Well) DirectionalAscIO : public Table::AscIO
{ mODTextTranslationClass(DirectionalAscIO)
public:
				DirectionalAscIO(const Table::FormatDesc&,
						 od_istream&);

    static Table::FormatDesc*	getDesc();
    bool			getData(Data&,float kb) const;

protected:
    bool			readFile(TypeSet<double>&,TypeSet<double>&,
					 TypeSet<double>&) const;
    od_istream&			strm_;

};

} // namespace Well

