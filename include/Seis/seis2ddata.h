#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Salil Agarwal
 Date:		Dec 2012
________________________________________________________________________

-*/

#include "seismod.h"
#include "namedobj.h"
#include "objectset.h"
#include "od_iosfwd.h"

class IOObj;
class Executor;
class SeisTrcBuf;
class TrcKeyZSampling;
class BinIDValueSet;
class BufferStringSet;
class SeisTrcWriter;
class Seis2DLinePutter;
class Seis2DLineIOProvider;
namespace PosInfo	{ class LineSet2DData; class Line2DData; }
namespace Seis		{ class SelData; }

static const char* sKeyNoOfLines mUnusedVar = "Number of Lines";

/*!\brief Set of 2D lines comparable with 3D seismic cube */


mExpClass(Seis) Seis2DDataSet : public NamedObject
{
    friend class SeisTrcWriter;

public:
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

    Pos::GeomID		geomID(int) const;
    const char*		lineName(int) const;
    int			indexOf(Pos::GeomID) const;
    int			indexOf(const char* linename) const;
    bool		isPresent(Pos::GeomID) const;
    bool		isPresent(const char* linename) const;
    bool		isEmpty(Pos::GeomID) const;

    void		getGeomIDs(TypeSet<Pos::GeomID>&) const;
    void		getLineNames(BufferStringSet&) const;

    Executor*		lineFetcher(Pos::GeomID,SeisTrcBuf&,
				    int nrtrcsperstep=10,
				    const Seis::SelData* sd=0) const;
				//!< May return null
    Seis2DLinePutter*	linePutter(Pos::GeomID);
				//!< May return null.
				//!< will return replacer if geomid exists

    bool		getTxtInfo(Pos::GeomID,BufferString& uinfo,
				   BufferString& stdinfo) const;
    bool		getRanges(Pos::GeomID,StepInterval<int>& trcrg,
				  StepInterval<float>& zrg) const;
    bool		haveMatch(Pos::GeomID,const BinIDValueSet&) const;

    bool		rename(const char*);
    bool		remove(Pos::GeomID);
				//!< Also removes from disk

    static void		getDataSetsOnLine(const char* lnm,
					      BufferStringSet& ds);
    static void		getDataSetsOnLine(Pos::GeomID geomid,
					  BufferStringSet& ds);

protected:

    IOObj&		ioobj_;
    BufferString	fname_;
    BufferString	datatype_;
    bool		readonly_;

    Seis2DLineIOProvider*	liop_;
    TypeSet<Pos::GeomID>	geomids_;

    void		init();

private:

    bool		getGeometry(Pos::GeomID,PosInfo::Line2DData&) const;
};

