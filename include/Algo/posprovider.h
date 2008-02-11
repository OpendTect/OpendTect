#ifndef posprovider_h
#define posprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posprovider.h,v 1.5 2008-02-11 17:23:05 cvsbert Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "factory.h"
#include "ranges.h"
class Executor;
class IOPar;
class CubeSampling;
namespace PosInfo { class Line2DData; }


namespace Pos
{

/*!\brief provides a subselection.

  Most providers require initialization. initialize() will always initialize
  the object, but this may take a long time. If that is an issue,
  try obtaining the initializer(). If that is null, then call initialize().
  If you need to iterate again, use reset().

  toNextPos() will ignore any Z settings and go to the first Z on the next 
  position. toNextZ() is the normal 'iterator increment'. After initialization,
  you need to do toNextZ() or toNextPos() for a valid position.

  The Provider can also return whether a position is included.

 */

class Provider
{
public:

    virtual Provider*	clone() const		= 0;
    virtual		~Provider()		{}
    virtual bool	is2D() const		= 0;

    virtual bool	initialize();
    virtual Executor*	initializer() const	{ return 0; }

    virtual void	reset()			= 0;

    virtual bool	toNextPos()		= 0;
    virtual bool	toNextZ()		= 0;
    virtual Coord	curCoord() const	= 0;
    virtual float	curZ() const		= 0;

    virtual bool	includes(const Coord&,
	    			 float z=mUdf(float)) const	= 0;
    virtual void	getZRange(Interval<float>&) const	= 0;

    virtual void	usePar(const IOPar&)	= 0;
    virtual void	fillPar(IOPar&) const	= 0;

    virtual int		estNrPos() const	= 0;
    virtual int		estNrZPerPos() const	{ return 1; }
};


/*!\brief provides a subselection for 3D surveys */

class Provider3D : public Provider
{
public:

    virtual bool	is2D() const		{ return false; }

    virtual BinID	curBinID() const				= 0;
    virtual Coord	curCoord() const;

    virtual bool	includes(const BinID&,float z=mUdf(float)) const = 0;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;

    virtual void	getExtent(BinID& start,BinID& stop) const	= 0;

    mDefineFactoryInClass(Provider3D,factory);

};


/*!\brief provides a subselection for 2D surveys - requires the line name(s). */

class Provider2D : public Provider
{
public:

    virtual bool	is2D() const				{ return true; }

    virtual int		curNr() const				= 0;
    virtual bool	includes(int,float z=mUdf(float)) const	= 0;

    mDefineFactoryInClass(Provider2D,factory);

};


} // namespace

#endif
