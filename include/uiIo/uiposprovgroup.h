#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiocommon.h"

#include "uiposfiltgroup.h"

#include "factory.h"
#include "fullsubsel.h"


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
			    , withrandom_(false)
			 {
			     fss_.setToAll( is_2d );
			 }

	virtual	~Setup()				{}
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(BufferString,zdomkey)
	mDefSetupMemb(Survey::FullSubSel,fss)
	mDefSetupMemb(bool,withrandom)
    };

			uiPosProvGroup(uiParent*,const Setup&);

    virtual void	setExtractionDefaults()		{}
    virtual bool	hasRandomSampling() const	{ return false; }
    Notifier<uiPosProvGroup>	posProvGroupChg;

    mDefineFactory2ParamInClass(uiPosProvGroup,uiParent*,const Setup&,factory);

};
