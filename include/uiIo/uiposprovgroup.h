#ifndef uiposprovgroup_h
#define uiposprovgroup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
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
			 { if ( is_2d ) tkzs_.set2DDef(); }

	virtual	~Setup()				{}
	Setup& cs(TrcKeyZSampling d) { tkzs_ = d; return *this; }
	//!<For legacy compliance
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(TrcKeyZSampling,tkzs)
	mDefSetupMemb(TypeSet< StepInterval<int> >,trcrgs)
	mDefSetupMemb(TypeSet< StepInterval<float> >,zrgs)
    };

			uiPosProvGroup(uiParent*,const Setup&);

    virtual void	setExtractionDefaults()		{}

    mDefineFactory2ParamInClass(uiPosProvGroup,uiParent*,const Setup&,factory);

};

#endif
