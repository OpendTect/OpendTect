#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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

    bool			is2D() const override	{ return true; }
    const char*			getName() const override;

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

    Coord			toCoord(int linenr,int tracenr) const override;
    Coord			toCoord(int tracenr) const;
    TrcKey			nearestTrace(const Coord&,
					     float* dist) const override;

    bool			includes(int linenr,int tracenr) const override;

    PosInfo::Line2DData&	dataAdmin()		{ return data_; }
				//!<If data is changed, call touch afterwards
    void			touch();
    const PosInfo::Line2DData&	data() const		{ return data_; }
    TypeSet<float>&		spnrs()			{ return spnrs_; }
    const TypeSet<float>&	spnrs() const		{ return spnrs_; }

    StepInterval<float>		zRange() const;

    static BufferString		makeUniqueLineName(const char* lsnm,
						   const char* lnm);
    float			averageTrcDist() const override;
    void			setAverageTrcDist(float);
    float			lineLength() const;
    void			setLineLength(float);
    RelationType		compare(const Geometry&,
					bool usezrg) const override;

    Geometry2D*			as2D() override		{ return this; }

protected:
				~Geometry2D();

    PosInfo::Line2DData&	data_;
    TypeSet<float>		spnrs_;
    mutable float		trcdist_;
    mutable float		linelength_;
    mutable Threads::Lock	lock_;
};

} // namespace Survey
