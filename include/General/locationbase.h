#pragma once
/*+
________________________________________________________________________

Copyright:	(C) 1995-2022 dGB Beheer B.V.
License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "coord.h"
#include "trckey.h"
#include "trigonometry.h"
#include "coordsystem.h"


mExpClass(General) LocationBase
{
public:
			    LocationBase(double x=0.,double y=0.,double z=0.,
				    const Pos::GeomID& =Pos::GeomID::udf());
			    LocationBase(const Coord&,double z=0.,
				    const Pos::GeomID& =Pos::GeomID::udf());
			    LocationBase(const Coord3&,
				    const Pos::GeomID& =Pos::GeomID::udf());
			    LocationBase(const LocationBase&);
			    ~LocationBase();

    bool		    operator ==(const LocationBase&) const;
    bool		    operator !=(const LocationBase&) const;
    void		    operator =(const LocationBase&);

    bool		    hasPos() const;
    bool		    hasTrcKey() const;
    bool		    is2D() const;
    const Coord3&	    pos() const;
    float		    z() const;

    void		    setPos(const Coord3&);
    void		    setPos(const Coord&);
    void		    setPos(double x,double y,double zval);
    void		    setPos(const Coord& c,float zval);

    template <class T>
    inline void		    setZ(T zval) { pos_.z = zval; }

    OD::GeomSystem	    geomSystem() const;
    Pos::GeomID		    geomID() const;
    const TrcKey&	    trcKey() const;
    Pos::LineID		    lineNr() const;
    Pos::TraceID	    trcNr() const;
    const BinID&	    binID() const;

    void		    setTrcKey(const TrcKey&);

    void		    setLineNr(Pos::LineID);
    void		    setTrcNr(Pos::LineID);
    void		    setGeomID(const Pos::GeomID&);
    void		    setBinID(const BinID&,bool updcoord=false);
    void		    setGeomSystem(OD::GeomSystem,
						    bool updfromcoord=true);
protected:

    Coord3		    pos_	= Coord3::udf();
    TrcKey		    trckey_	= TrcKey::udf();

};
