#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
________________________________________________________________________

-*/

#include "seiscommon.h"
#include "namedobj.h"
#include "geomid.h"
#include "objectset.h"
#include "od_iosfwd.h"

class SeisTrcBuf;
class TrcKeyZSampling;
class BinnedValueSet;
class BufferStringSet;
class Seis2DLinePutter;
class Seis2DTraceGetter;
class Seis2DLineIOProvider;
namespace PosInfo	{ class LineSet2DData; class Line2DData; }
namespace Seis		{ class SelData; }

static const char* sKeyNoOfLines mUnusedVar = "Number of Lines";

/*!\brief Set of 2D lines comparable with 3D seismic cube */


mExpClass(Seis) Seis2DDataSet : public NamedObject
{ mODTextTranslationClass(Seis2DDataSet)
public:

    mUseType( Pos,	GeomID );

			Seis2DDataSet(const IOObj&);
			Seis2DDataSet(const Seis2DDataSet&);

    virtual		~Seis2DDataSet();
    void		setReadOnly( bool yn=true )	{ readonly_ = yn; }

    const char*		dataType() const		{ return datatype_; }
    const char*		fileName() const		{ return fname_; }
    const char*		type() const;
    int			nrLines() const
			{ return geomids_.size(); }
    bool		isEmpty() const;

    GeomID		geomID(int) const;
    const char*		lineName(int) const;
    int			indexOf(GeomID) const;
    int			indexOf(const char* linename) const;
    bool		isPresent(GeomID) const;
    bool		isPresent(const char* linename) const;
    bool		isEmpty(GeomID) const;

    void		getGeomIDs(GeomIDSet&) const;
    void		getLineNames(BufferStringSet&) const;
    uiRetVal		getGeometry(GeomID,PosInfo::Line2DData&) const;

    Seis2DTraceGetter*	traceGetter(GeomID,const Seis::SelData*,
				    uiRetVal&) const;
				//!< May return null
    Seis2DLinePutter*	linePutter(GeomID,uiRetVal&);
				//!< May return null.
				//!< will return replacer if geomid exists

    bool		getTxtInfo(GeomID,BufferString& uinfo,
				   BufferString& stdinfo) const;
    bool		getRanges(GeomID,StepInterval<int>& trcrg,
				  StepInterval<float>& zrg) const;
    bool		haveMatch(GeomID,const BinnedValueSet&) const;

    bool		rename(const char*);
    bool		remove(GeomID);
				//!< Also removes from disk

    static void		getDataSetsOnLine(const char* lnm,
					      BufferStringSet& ds);
    static void		getDataSetsOnLine(GeomID geomid,
					  BufferStringSet& ds);

protected:

    IOObj&		ioobj_;
    BufferString	fname_;
    BufferString	datatype_;
    bool		readonly_;

    Seis2DLineIOProvider* liop_;
    GeomIDSet		geomids_;

    void		init();
};
