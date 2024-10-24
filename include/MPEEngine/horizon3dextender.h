#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "emhorizon3d.h"
#include "factory.h"
#include "sectionextender.h"
#include "trckeyvalue.h"


namespace MPE
{

/*!
\brief Sub class of SectionExtender for 3D Horizons with its factory
*/

mExpClass(MPEEngine) Horizon3DExtenderBase : public SectionExtender
{
mODTextTranslationClass(Horizon3DExtenderBase)
public:
    mDefineFactory1ParamInClass( Horizon3DExtenderBase, EM::Horizon3D&,factory);

				~Horizon3DExtenderBase();

    static Horizon3DExtenderBase* createInstance(EM::Horizon3D&);

    void			setDirection(const TrcKeyValue&) override;
    const TrcKeyValue*		getDirection() const override
				{ return &direction_; }

    int				nextStep() override;

    int				maxNrPosInExtArea() const override;
    void			preallocExtArea() override;

    const TrcKeyZSampling&	getExtBoundary() const override;

protected:
				Horizon3DExtenderBase(EM::Horizon3D&);

    virtual float		getDepth(const TrcKey& src,
					 const TrcKey& target) const override;

    TrcKeyValue			direction_;
    EM::Horizon3D&		horizon_;
};


/*!
\brief Simple implementation for extending a EM::Horizon3D.
*/

mExpClass(MPEEngine) Horizon3DExtender : public Horizon3DExtenderBase
{
mODTextTranslationClass(Horizon3DExtender)
public:

    mDefaultFactoryInstantiation1Param( Horizon3DExtenderBase,Horizon3DExtender,
					EM::Horizon3D&,
					EM::Horizon3D::typeStr(),
					tr("Horizon 3D") );
private:
				Horizon3DExtender(EM::Horizon3D&);
				~Horizon3DExtender();
};

} // namespace MPE
