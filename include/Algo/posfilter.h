#ifndef posfilter_h
#define posfilter_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posfilter.h,v 1.1 2008-02-20 12:44:02 cvsbert Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "factory.h"
class Executor;
class IOPar;


namespace Pos
{

/*!\brief decideds whether a given position should be included

  Filter2D and Filter3D have factories. These are meant for 'true' Filters,
  therefore no Providers wanted.
 
 */

class Filter
{
public:

    virtual Filter*	clone() const				= 0;
    virtual		~Filter()				{}

    virtual const char*	type() const				= 0;
    virtual bool	is2D() const				= 0;

    virtual bool	initialize();
    virtual Executor*	initializer() const			{ return 0; }
    virtual void	reset()					= 0;

    virtual bool	includes(const Coord&,
	    			 float z=mUdf(float)) const	= 0;
    virtual bool	hasZAdjustment() const			{ return false;}
    virtual float	adjustedZ( const Coord&, float z )	{ return z; }

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
