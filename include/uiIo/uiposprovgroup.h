#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiposfiltgroup.h"
#include "trckeyzsampling.h"
#include "factory.h"


/*! \brief group for providing positions, usually for 2D or 3D seismics */

mExpClass(uiIo) uiPosProvGroup : public uiPosFiltGroup
{ mODTextTranslationClass(uiPosProvGroup);
public:

    mExpClass(uiIo) Setup : public uiPosFiltGroup::Setup
    {
    public:
			Setup(bool is_2d,bool with_step,bool with_z);
	virtual		~Setup();

	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(BufferString,zunitstr)
	mDefSetupMemb(TrcKeyZSampling,tkzs)
	mDefSetupMemb(bool,withrandom)		// false
	mDefSetupMemb(TypeSet< StepInterval<int> >,trcrgs)
	mDefSetupMemb(TypeSet< StepInterval<float> >,zrgs)

	mDeprecated("Use tkzs")
	Setup&		cs(const TrcKeyZSampling&);
    };

			uiPosProvGroup(uiParent*,const Setup&);
			~uiPosProvGroup();

    virtual void	setExtractionDefaults()		{}
    virtual bool	hasRandomSampling() const	{ return false; }
    Notifier<uiPosProvGroup> posProvGroupChg;

    static::Factory2Param<uiPosProvGroup,uiParent*,const Setup&>& factory();

    uiString		factoryDisplayName() const override
			{ return ::toUiString( factoryKeyword() ); }

    const char*		factoryKeyword() const override { return nullptr; }

};
