#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
________________________________________________________________________


-*/

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "keystrs.h"
#include "iopar.h"
#include "trckey.h"
#include "stratlevel.h"


namespace EM
{

/*!\brief Horizon RowColSurfaceGeometry */

mExpClass(EarthModel) HorizonGeometry : public RowColSurfaceGeometry
{
protected:
				HorizonGeometry( Surface& surf )
				    : RowColSurfaceGeometry(surf)	{}
public:

    virtual PosID		getPosID(const TrcKey&) const		= 0;
    virtual TrcKey		getTrcKey(const PosID&) const		= 0;

};


/*!\brief Horizon Surface */

mExpClass(EarthModel) Horizon : public Surface
{
public:

    typedef Strat::Level::ID		LevelID;

    virtual HorizonGeometry&		geometry()			= 0;
    virtual const HorizonGeometry&	geometry() const
					{ return const_cast<Horizon*>(this)
								->geometry(); }

    virtual bool	is2D() const				= 0;
    virtual float	getZ(const TrcKey&) const		= 0;
    virtual bool	setZ(const TrcKey&,float z,bool addtohist,
			    NodeSourceType type=Auto)		= 0;
    virtual bool	hasZ(const TrcKey&) const		= 0;
    virtual Coord3	getCoord(const TrcKey&) const		= 0;

    virtual float	getZValue(const Coord&,bool allow_udf=true,
				  int nr=0) const		= 0;
    virtual void	setAttrib(const TrcKey&,int attr,bool yn,bool undo) = 0;
    virtual bool	isAttrib(const TrcKey&,int attr) const	= 0;

    void		setStratLevelID( LevelID lvlid )
			{ stratlevelid_ = lvlid; }
    LevelID		stratLevelID() const
			{ return stratlevelid_; }

    virtual void	fillPar( IOPar& par ) const
			{
			    Surface::fillPar( par );
			    par.set( sKey::StratRef(), stratlevelid_ );
			}

    virtual bool	usePar( const IOPar& par )
			{
			    par.get( sKey::StratRef(), stratlevelid_ );
			    return Surface::usePar( par );
			}

    virtual OD::GeomSystem geomSystem() const				= 0;

protected:
			Horizon(const char* nm)
			    : Surface(nm)				{}

    virtual const IOObjContext&	getIOObjContext() const			= 0;

    LevelID			stratlevelid_;
};

} // namespace EM
