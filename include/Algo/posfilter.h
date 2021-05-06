#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________


-*/

#include "algomod.h"
#include "coord.h"
#include "factory.h"
#include "posinfo2dsurv.h"

class TaskRunner;

/*!\brief Position*/

namespace Pos
{
class Provider;

/*!
\brief Decides whether a given position should be included.

  Some Filters require initialization. There are two levels of initialization:

  * After 'usePar' the object may be in an intermediate state. You should be
  able to ask all kinds of global questions, but not toNextPos(), toNextZ(),
  curCoord(), curZ(), or includes().
  * To be able to fully use all functions, you have to initialize() the
  object.

  Filter2D and Filter3D have factories. Providers too. Standard providers
  are not added to the Filter factory. Non-standard should in general be added
  to both. 
*/

mExpClass(Algo) Filter
{
public:

    virtual Filter*	clone() const				= 0;
    virtual		~Filter()				{}

    virtual const char*	type() const				= 0;
    virtual bool	is2D() const				= 0;
    virtual bool	isProvider() const			{ return false;}

    virtual bool	initialize( TaskRunner* tr=0 )
    			{ reset(); return true; }
    virtual void	reset()					= 0;

    virtual bool	includes(const Coord&,
	    			 float z=mUdf(float)) const	= 0;
    virtual bool	hasZAdjustment() const			{ return false;}
    virtual float	adjustedZ(const Coord&, float z ) const	{ return z; }

    virtual void	usePar(const IOPar&)			= 0;
    virtual void	fillPar(IOPar&) const			= 0;

    virtual void	getSummary(BufferString&) const		= 0;
    virtual float	estRatio(const Provider&) const		= 0;

    static Filter*	make(const IOPar&,bool is2d);
};


/*!
\brief Provides a filter related to 3D data.
*/

mExpClass(Algo) Filter3D : public virtual Filter
{
public:

    virtual bool	is2D() const		{ return false; }

    virtual bool	includes(const BinID&,float z=mUdf(float)) const = 0;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;

    mDefineFactoryInClass(Filter3D,factory);
    static Filter3D*	make(const IOPar&);

};


/*!
\brief Provides a filter related to 2D seismic data.
*/

mExpClass(Algo) Filter2D : public virtual Filter
{
public:
    			Filter2D()				{}
			~Filter2D();

    virtual bool	is2D() const				{ return true; }
    virtual bool	includes(int,float z=mUdf(float),int lidx=0) const = 0;
    virtual bool	includes(const Coord&,
	    			 float z=mUdf(float)) const	= 0;

    void		addGeomID(const Pos::GeomID);
    void		removeGeomID(int lidx);
    Pos::GeomID		geomID(int) const;
    int			nrLines() const;

    mDefineFactoryInClass(Filter2D,factory);
    static Filter2D*	make(const IOPar&);

protected:

    TypeSet<Pos::GeomID> geomids_;
};


} // namespace

