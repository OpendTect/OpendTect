#ifndef uiposprovgroup_h
#define uiposprovgroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovgroup.h,v 1.9 2009-01-08 07:23:07 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uiposfiltgroup.h"
#include "cubesampling.h"
#include "factory.h"


/*! \brief group for providing positions, usually for 2D or 3D seismics */

mClass uiPosProvGroup : public uiPosFiltGroup
{
public:

    struct Setup : public uiPosFiltGroup::Setup
    {
			Setup( bool is_2d, bool with_step, bool with_z )
			    : uiPosFiltGroup::Setup(is_2d)
			    , withstep_(with_step)
			    , withz_(with_z)
			    , cs_(!is_2d)
			 { if ( is_2d ) cs_.set2DDef(); }

	virtual	~Setup()				{}
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,withz)
	mDefSetupMemb(CubeSampling,cs)
    };

			uiPosProvGroup(uiParent*,const Setup&);

    virtual void	setExtractionDefaults()		{}

    mDefineFactory2ParamInClass(uiPosProvGroup,uiParent*,const Setup&,factory);

};

#endif
