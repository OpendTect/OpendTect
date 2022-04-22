#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2007
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "keystrs.h"
#include "iopar.h"
#include "trckey.h"


namespace EM
{
class EMManager;

/*!
\brief Horizon RowColSurfaceGeometry
*/

mExpClass(EarthModel) HorizonGeometry : public RowColSurfaceGeometry
{
protected:
				HorizonGeometry( Surface& surf )
				    : RowColSurfaceGeometry(surf)	{}
public:
    virtual PosID		getPosID(const TrcKey&) const		= 0;
    virtual TrcKey		getTrcKey(const PosID&) const		= 0;
};


/*!
\brief Horizon Surface
*/

mExpClass(EarthModel) Horizon : public Surface
{
public:
    HorizonGeometry&		geometry() override		= 0;
    const HorizonGeometry&	geometry() const override
				{ return const_cast<Horizon*>(this)
							->geometry(); }

    virtual float	getZ(const TrcKey&) const			= 0;
    virtual bool	setZ(const TrcKey&,float z,bool addtohist)	= 0;
    virtual bool	setZAndNodeSourceType(const TrcKey&,float z,
			    bool addtohist, NodeSourceType type=Auto)	= 0;
    virtual bool	hasZ(const TrcKey&) const			= 0;
    virtual Coord3	getCoord(const TrcKey&) const			= 0;

    virtual float	getZValue(const Coord&,bool allow_udf=true,
				  int nr=0) const			= 0;
    virtual void	setAttrib(const TrcKey&,int attr,int yn,bool undo) = 0;
    virtual bool	isAttrib(const TrcKey&,int attr) const = 0;

    void		setStratLevelID( int lvlid )
			{ stratlevelid_ = lvlid; }
    int			stratLevelID() const
			{ return stratlevelid_; }

    void		fillPar( IOPar& par ) const override
			{
			    Surface::fillPar( par );
			    par.set( sKey::StratRef(), stratlevelid_ );
			}

    bool		usePar( const IOPar& par ) override
			{
			    par.get( sKey::StratRef(), stratlevelid_ );
			    return Surface::usePar( par );
			}

    virtual OD::GeomSystem getSurveyID() const				= 0;

protected:
			Horizon( EMManager& emm )
			    : Surface(emm), stratlevelid_(-1)	{}

    const IOObjContext& getIOObjContext() const override		= 0;

    int			stratlevelid_;
};

} // namespace EM

