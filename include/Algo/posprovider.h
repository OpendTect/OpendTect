#ifndef posprovider_h
#define posprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posprovider.h,v 1.20 2011/11/14 07:39:14 cvssatyaki Exp $
________________________________________________________________________


-*/

#include "posfilter.h"
#include "ranges.h"
class CubeSampling;


namespace Pos
{

/*!\brief provides a series of positions; can also be used fo subselection.

  toNextPos() will ignore any Z settings and go to the first Z on the next 
  position. toNextZ() is the normal 'iterator increment'. After initialization,
  you need to do toNextZ() or toNextPos() for a valid position.

 */

class Provider : public virtual Filter
{
public:

    virtual bool	isProvider() const			{ return true; }

    mGlobal virtual Provider*	clone() const			= 0;

    virtual bool	toNextPos()				= 0;
    virtual bool	toNextZ()				= 0;
    virtual Coord	curCoord() const			= 0;
    virtual float	curZ() const				= 0;

    virtual od_int64	estNrPos() const			= 0;
    virtual int		estNrZPerPos() const			{ return 1; }

    mGlobal virtual float	estRatio(const Provider&) const;
    mGlobal virtual void	getCubeSampling(CubeSampling&) const;

    mGlobal static Provider*	make(const IOPar&,bool is2d);
};


/*!\brief provides a subselection for 3D surveys */

mClass Provider3D : public Filter3D
		 , public Provider
{
public:

    virtual bool	is2D() const		{ return false; }

    virtual BinID	curBinID() const				= 0;
    virtual Coord	curCoord() const;

    virtual bool	includes(const BinID&,float z=mUdf(float)) const = 0;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;

    virtual void	getExtent(BinID& start,BinID& stop) const	= 0;
    virtual void	getZRange(Interval<float>&) const	= 0;

    mDefineFactoryInClass(Provider3D,factory);
    static Provider3D*	make(const IOPar&);

};


/*!\brief provides a subselection for 2D surveys - requires the line name(s). */

mClass Provider2D : public Filter2D
		 , public Provider
{
public:

    virtual bool	is2D() const				{ return true; }

    virtual int		curNr() const				= 0;
    virtual bool	includes(int,float z=mUdf(float), int nr=0) const= 0;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const = 0;

    virtual void	getExtent(Interval<int>&,int lidx) const = 0;
    virtual void	getZRange(Interval<float>&,int lidx) const = 0;

    mDefineFactoryInClass(Provider2D,factory);
    static Provider2D*	make(const IOPar&);

};


} // namespace

#endif
