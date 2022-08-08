#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2008
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

    struct Setup : public uiPosFiltGroup::Setup
    {
			Setup( bool is_2d, bool with_step, bool with_z )
			    : uiPosFiltGroup::Setup(is_2d)
			    , withstep_(with_step)
			    , withz_(with_z)
			    , tkzs_(!is_2d)
			    , withrandom_(false)
			 { if ( is_2d ) tkzs_.set2DDef(); }

	virtual ~Setup()				{}
	Setup& cs(TrcKeyZSampling d) { tkzs_ = d; return *this; }
	//!<For legacy compliance
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(TrcKeyZSampling,tkzs)
	mDefSetupMemb(bool,withrandom)
	mDefSetupMemb(TypeSet< StepInterval<int> >,trcrgs)
	mDefSetupMemb(TypeSet< StepInterval<float> >,zrgs)
    };

			uiPosProvGroup(uiParent*,const Setup&);

    virtual void	setExtractionDefaults()		{}
    virtual bool	hasRandomSampling() const	{ return false; }
    Notifier<uiPosProvGroup>	posProvGroupChg;

    static::Factory2Param<uiPosProvGroup,uiParent*,const Setup&>& factory();

    uiString		factoryDisplayName() const override
			{ return ::toUiString( factoryKeyword() ); }

    const char*		factoryKeyword() const override { return nullptr; }

};

