#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "coord.h"
#include "emhorizon2d.h"
#include "factory.h"
#include "sectionextender.h"
#include "trckeyvalue.h"


namespace MPE
{

/*!
\brief Sub class of SectionExtender for 2D Horizons with its factory
*/

mExpClass(MPEEngine) Horizon2DExtenderBase : public SectionExtender
{
mODTextTranslationClass(Horizon2DExtenderBase)
public:
    mDefineFactory1ParamInClass( Horizon2DExtenderBase, EM::Horizon2D&,factory);

				~Horizon2DExtenderBase();

    static Horizon2DExtenderBase* createInstance(EM::Horizon2D&);

    void			setAngleThreshold(float radians);
    float			getAngleThreshold() const;

    void			setDirection(const TrcKeyValue&) override;
    const TrcKeyValue*		getDirection() const override;
    void			setGeomID(const Pos::GeomID&);
    Pos::GeomID			geomID() const;

    int				nextStep() override;

protected:
				Horizon2DExtenderBase(EM::Horizon2D&);

    void			addNeighbor(bool upwards,
					    const TrcKey& sourcesid);
    float			getDepth(const TrcKey& src,
					 const TrcKey& target) const override;
    void			prepareDataIfRequired() override	{}

    float			anglethreshold_		= 0.5f;
    bool			alldirs_		= true;
    Coord			xydirection_;
    TrcKeyValue			direction_;
    EM::Horizon2D&		hor2d_;
    Pos::GeomID			geomid_;
};


/*!
\brief Simple implementation for extending a EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DExtender : public Horizon2DExtenderBase
{
mODTextTranslationClass(Horizon2DExtender)
public:

    mDefaultFactoryInstantiation1Param( Horizon2DExtenderBase,Horizon2DExtender,
					EM::Horizon2D&,
					EM::Horizon2D::typeStr(),
					tr("Horizon 2D") );
private:
				Horizon2DExtender(EM::Horizon2D&);
				~Horizon2DExtender();
};

} // namespace MPE
