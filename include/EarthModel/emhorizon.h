#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "stratlevel.h"


namespace EM
{
class EMManager;

/*!
\brief Horizon RowColSurfaceGeometry
*/

mExpClass(EarthModel) HorizonGeometry : public RowColSurfaceGeometry
{
public:
				~HorizonGeometry();

    virtual PosID		getPosID(const TrcKey&) const		= 0;
    virtual TrcKey		getTrcKey(const PosID&) const		= 0;

protected:
				HorizonGeometry(Surface&);
};


/*!
\brief Horizon Surface
*/

mExpClass(EarthModel) Horizon : public Surface
{
public:
				~Horizon();

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

    void		setStratLevelID( Strat::LevelID lvlid )
			{ stratlevelid_ = lvlid; }
    void		setNoLevelID()
			{ setStratLevelID( Strat::LevelID::udf() ); }
    Strat::LevelID	stratLevelID() const
			{ return stratlevelid_; }

    void		fillPar( IOPar& par ) const override;
    bool		usePar( const IOPar& par ) override;

    virtual OD::GeomSystem getSurveyID() const				= 0;

protected:
			Horizon(EMManager&);

    const IOObjContext& getIOObjContext() const override		= 0;

    Strat::LevelID	stratlevelid_;
};

} // namespace EM
