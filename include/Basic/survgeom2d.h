#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2010
________________________________________________________________________

-*/


#include "basicmod.h"
#include "survgeom.h"

class DBKey;
namespace PosInfo { class Line2DData; }


namespace Survey
{

class SubGeometry2D;


/*!\brief Geometry of a 2D Line. */

mExpClass(Basic) Geometry2D : public Geometry
{
public:
				Geometry2D(const char* lnm);
				Geometry2D(PosInfo::Line2DData*);
				//!<Line2DData becomes mine

    virtual bool		is2D() const		{ return true; }
    virtual const name_type&	name() const;

    void			add(const Coord&,int trcnr,int spnr=-1);
    void			add(double x,double y,int trcnr,int spnr=-1);
    int				size() const;
    void			setEmpty();
    virtual Coord		toCoord(int linenr,int tracenr) const;
    virtual TrcKey		nearestTrace(const Coord&,float* dist) const;

    virtual bool		includes(int linenr,int tracenr) const;

    PosInfo::Line2DData&	dataAdmin()		{ return data_; }
				//!<If data is changed, call touch afterwards
    void			touch();
    const PosInfo::Line2DData&	data() const		{ return data_; }
    TypeSet<int>&		spnrs()			{ return spnrs_; }
    const TypeSet<int>&		spnrs() const		{ return spnrs_; }

    StepInterval<float>		zRange() const;

    static BufferString		makeUniqueLineName(const char* lsnm,
						   const char* lnm);
    float			averageTrcDist() const;
    void			setAverageTrcDist(float);
    float			lineLength() const;
    void			setLineLength(float);
    virtual RelationType	compare(const Geometry&,bool usezrg) const;

    static ID			getIDFrom(const DBKey&);

protected:

				Geometry2D();
				~Geometry2D();

    PosInfo::Line2DData&	data_;
    TypeSet<int>		spnrs_;
    mutable float		trcdist_;
    mutable float		linelength_;
    mutable Threads::Lock	lock_;

    virtual Geometry2D*		gtAs2D() const
				{ return const_cast<Geometry2D*>(this); }

    friend class		SubGeometry2D;

};

} // namespace Survey
