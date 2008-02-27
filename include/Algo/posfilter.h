#ifndef posfilter_h
#define posfilter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posfilter.h,v 1.5 2008-02-27 09:48:36 cvsbert Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "factory.h"
class Executor;
class IOPar;


namespace Pos
{

/*!\brief decideds whether a given position should be included

  Some Filters require time-consuming initialization.
  initialize() will always initialize the object, but this may take a long
  time. If that is an issue, try obtaining the initializer().
  If that is null, then call initialize(). If you need to iterate again, use
  reset().

  After 'usePar' the object may be in an intermediate state. You should be
  able to ask all kinds of global questions, but not toNextPos(), toNextZ(),
  curCoord(), curZ(), or includes(). For that, you have to initialize() the
  object.

  Filter2D and Filter3D have factories. Providers too. Standard providers
  are not added to the Filter factory. Non-standard should in general be added
  to both.
 
 */

class Filter
{
public:

    virtual Filter*	clone() const				= 0;
    virtual		~Filter()				{}

    virtual const char*	type() const				= 0;
    virtual bool	is2D() const				= 0;
    virtual bool	isProvider() const			{ return false;}

    virtual bool	initialize();
    virtual Executor*	initializer()				{ return 0; }
    virtual void	reset()					= 0;

    virtual bool	includes(const Coord&,
	    			 float z=mUdf(float)) const	= 0;
    virtual bool	hasZAdjustment() const			{ return false;}
    virtual float	adjustedZ(const Coord&, float z ) const	{ return z; }

    virtual void	usePar(const IOPar&)			= 0;
    virtual void	fillPar(IOPar&) const			= 0;

    virtual void	getSummary(BufferString&) const		= 0;
};


/*!\brief provides a filter related to 3D data */

class Filter3D : public Filter
{
public:

    virtual bool	is2D() const		{ return false; }

    virtual bool	includes(const BinID&,float z=mUdf(float)) const = 0;
    virtual bool	includes(const Coord&,float z=mUdf(float)) const;

    mDefineFactoryInClass(Filter3D,factory);
    static Filter3D*	make(const IOPar&);

};


/*!\brief provides a filter related to 2D seismic data */

class Filter2D : public Filter
{
public:

    virtual bool	is2D() const				{ return true; }

    virtual bool	includes(int,float z=mUdf(float)) const	= 0;

    mDefineFactoryInClass(Filter2D,factory);
    static Filter2D*	make(const IOPar&);

};


} // namespace

#endif
