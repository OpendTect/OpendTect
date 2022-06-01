#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
 RCS:		$Id$
________________________________________________________________________

-*/


#include "basicmod.h"
#include "survgeom.h"

namespace PosInfo { class Line2DData; }


namespace Survey
{

/*!\brief Geometry of a 2D Line. */

mExpClass(Basic) Geometry2D : public Geometry
{
public:
				Geometry2D(const char* lnm);
				Geometry2D(PosInfo::Line2DData*);
				//!<Line2DData becomes mine

    virtual bool		is2D() const		{ return true; }
    virtual const char*		getName() const;

    void			add(const Coord&,int trcnr,float spnr);
    void			add(double x,double y,int trcnr,float spnr);
    int				size() const;
    bool			isEmpty() const;
    void			setEmpty();
    bool			getPosByTrcNr(int trcnr,Coord&,
					      float& spnr) const;
    bool			getPosBySPNr(float spnr,Coord&,
					     int& trcnr) const;
    bool			getPosByCoord(const Coord&,
					      int& trc,float& sp) const;

    virtual Coord		toCoord(int linenr,int tracenr) const;
    Coord			toCoord(int tracenr) const;
    virtual TrcKey		nearestTrace(const Coord&,float* dist) const;

    virtual bool		includes(int linenr,int tracenr) const;

    PosInfo::Line2DData&	dataAdmin()		{ return data_; }
				//!<If data is changed, call touch afterwards
    void			touch();
    const PosInfo::Line2DData&	data() const		{ return data_; }
    TypeSet<float>&		spnrs()			{ return spnrs_; }
    const TypeSet<float>&	spnrs() const		{ return spnrs_; }
    bool			hasValidSPNrs() const;

    StepInterval<float>		zRange() const;

    static float		udfSPNr()		{ return -1.f; }
    static BufferString		makeUniqueLineName(const char* lsnm,
						   const char* lnm);

    float			averageTrcDist() const;
    void			setAverageTrcDist(float);
    float			lineLength() const;
    void			setLineLength(float);
    RelationType		compare(const Geometry&,bool usezrg) const;

    Geometry2D*			as2D()			{ return this; }

protected:
				~Geometry2D();

    PosInfo::Line2DData&	data_;
    TypeSet<float>		spnrs_;
    mutable float		trcdist_;
    mutable float		linelength_;
    mutable Threads::Lock	lock_;
};

} // namespace Survey

