#ifndef posprovider_h
#define posprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posprovider.h,v 1.3 2008-02-04 16:23:26 cvsbert Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "factory.h"
#include "ranges.h"
class Executor;
class IOPar;
class CubeSampling;

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

    virtual		~Provider()		{}

    virtual bool	initialize();
    virtual Executor*	initializer() const	{ return 0; }

    virtual void	reset()			= 0;

    virtual bool	toNextPos()		= 0;
    virtual bool	toNextZ()		= 0;
    virtual Coord	curCoord() const	= 0;
    virtual float	curZ() const		= 0;

    virtual bool	includes(const Coord&,float z=mUdf(float)) const = 0;

    virtual void	usePar(const IOPar&)			= 0;
    virtual void	fillPar(IOPar&) const			= 0;

};


/*!\brief provides a subselection for 3D surveys */

class Provider3D : public Provider
{
public:

    virtual BinID	curBinID() const	= 0;
    virtual Coord	curCoord() const;

    virtual bool	includes(const BinID&,float z=mUdf(float)) const = 0;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;

    mDefineFactoryInClass(Provider3D,factory);

};


/*!\brief provides a subselection for 2D surveys - requires the line name(s). */

class Provider2D
{
public:

    virtual int		curNr() const				= 0;
    virtual bool	includes(int,float z=mUdf(float)) const	= 0;

    mDefineFactoryInClass(Provider2D,factory);

};


/*!\brief 3D provider based on CubeSampling */

class Rect3DProvider : public Provider3D
{
public:

			Rect3DProvider();
			~Rect3DProvider();

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const		{ return curz_; }
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    CubeSampling&	sampling()		{ return cs_; }
    const CubeSampling&	sampling() const	{ return cs_; }

protected:

    CubeSampling&	cs_;
    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new Rect3DProvider; }

};


/*!\brief 2D provider based on StepInterval<int> */

class Rect2DProvider : public Provider2D
{
public:

			Rect2DProvider();

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual int		curNr() const		{ return curnr_; }
    virtual float	curZ() const		{ return curz_; }
    virtual bool	includes(int,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    const StepInterval<int>&	nrRange() const	{ return rg_; }
    StepInterval<int>&		nrRange()	{ return rg_; }
    const StepInterval<float>&	zRange() const	{ return zrg_; }
    StepInterval<float>&	zRange()	{ return zrg_; }

protected:

    StepInterval<int>	rg_;
    StepInterval<float>	zrg_;
    int			curnr_;
    float		curz_;

public:

    static void		initClass();
    static Provider2D*	create()	{ return new Rect2DProvider; }

};


} // namespace

#endif
