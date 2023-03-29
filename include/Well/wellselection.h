#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "welldata.h"

namespace Well
{

class LogSet;

mExpClass(Well) SelInfo
{
public:
				SelInfo(const Well::Data&);
    virtual			~SelInfo();

    const char*			wellName() const;
    MultiID			wellID() const;

    inline void			setSelectedLogs( const BufferStringSet& nms )
				{ lognms_ = nms; }
    inline const BufferStringSet& selectedLogs() const
				{ return lognms_; }
    inline void			setSelectedMarkers( const BufferStringSet& nms )
				{markernms_ = nms; }
    inline const BufferStringSet& selectedMarkers() const
				{ return markernms_; }

    inline void			setMDRange( const Interval<float>& rg )
				{ mdrg_ = rg; }
    inline Interval<float>	getMDRange() const	{ return mdrg_; }

protected:

    ConstRefMan<Well::Data>	wd_;
    Interval<float>		mdrg_		= Interval<float>::udf();

    BufferStringSet		lognms_;
    BufferStringSet		markernms_;
};


mExpClass(Well) MultiSelSpec
{
public:
				MultiSelSpec();
    virtual			~MultiSelSpec();

    void			clear();

    DBKeySet			wellkeys_;
    BufferStringSet		lognms_;
    BufferStringSet		mnemonicnms_;
    BufferStringSet		markernms_;
};


//!\brief Holds a set of logs and markers.
//! This object does not own logs. It borrows from the Well::Data object
//! or from users of an instance.

mExpClass(Well) SubSelData
{
public:
				SubSelData(const SelInfo&);
    virtual			~SubSelData();

    const char*			wellName() const;
    MultiID			wellID() const;

    inline LogSet&		logs()			{ return logs_; }
    inline const LogSet&	logs() const		{ return logs_; }
    inline MarkerSet&		markers()		{ return markers_; }
    inline const MarkerSet&	markers() const		{ return markers_; }
    inline Interval<float>	getMDRange() const	{ return mdrg_; }

protected:
    void			init(const SelInfo&);

    ConstRefMan<Well::Data>	wd_;
    LogSet&			logs_;
    MarkerSet&			markers_;
    Interval<float>		mdrg_		= Interval<float>::udf();
};

} // namespace Well
