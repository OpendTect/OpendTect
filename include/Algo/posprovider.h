#ifndef posprovider_h
#define posprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posprovider.h,v 1.1 2008-02-01 14:02:04 cvsbert Exp $
________________________________________________________________________


-*/

#include "position.h"
#include "factory.h"
#include "cubesampling.h"
class Executor;
class IOPar;

namespace Pos
{

/*!\brief provides an area subselection.

  Most providers require initialization. initialize() will always initialize
  the object, but this may take a long time. If that is an issue,
  try obtaining the initializer(). If that is null, then call initialize().
  If you need to iterate through the area again, use reset().
 */

class AreaProvider
{
public:

    virtual bool	initialize();
    virtual Executor*	initializer() const		{ return 0; }

    virtual void	reset()				{}

    virtual bool	toNextPos()			= 0;
    virtual BinID	curBinID() const		= 0;
    virtual Coord	curCoord() const;

    virtual bool	isSelected(const BinID&) const	= 0;
    virtual bool	isSelected(const Coord&) const;

    virtual void	usePar(const IOPar&)		= 0;
    virtual void	fillPar(IOPar&) const		= 0;

    mDefineFactory1ParamInClass(AreaProvider,bool,factory);

};


/*!\brief provides a volume subselection.

  * Requires creation parameter: for 2D lines?
  * toNextZ may go to the next position.
 */

class VolumeProvider : public AreaProvider
{
public:

    virtual bool	toNextZ()				{ return false;}
    virtual float	curZ() const				= 0;

    virtual bool	isSelected(const BinID&,float) const	= 0;
    virtual bool	isSelected(const Coord&,float) const;

    mDefineFactory1ParamInClass(VolumeProvider,bool,factory);

};


/*!\brief Volume/Area provider based on CubeSampling */

class RectVolumeProvider : public VolumeProvider
{
public:

			RectVolumeProvider(bool for2d);
    virtual void	reset();
    virtual bool	initialize();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const		{ return curz_; }
    virtual bool	isSelected( const BinID& b ) const
			{ return cs_.hrg.includes( b ); }
    virtual bool	isSelected(const BinID&,float) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;

    CubeSampling	cs_;

protected:

    bool		is2d_;
    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static AreaProvider* createAP( bool for2d )
    			{ return new RectVolumeProvider(for2d); }
    static VolumeProvider* createVP( bool for2d )
    			{ return new RectVolumeProvider(for2d); }

};


} // namespace

#endif
