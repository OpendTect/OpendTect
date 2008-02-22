#ifndef uiposprovgroup_h
#define uiposprovgroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovgroup.h,v 1.4 2008-02-22 09:31:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposfiltgroup.h"
#include "factory.h"


/*! \brief group for providing positions, usually for 2D or 3D seismics */

class uiPosProvGroup : public uiGroup
{
public:

    struct Setup : public uiPosFiltGroup::Setup
    {
			Setup( bool is_2d, bool with_z )
			    : uiPosFiltGroup::Setup(is_2d)
			    , withz_(with_z)		{}

	virtual	~Setup()				{}
	mDefSetupMemb(bool,withz)
    };

			uiPosProvGroup(uiParent*,const Setup&);

    virtual void	usePar(const IOPar&)		= 0;
    virtual bool	fillPar(IOPar&) const		= 0;

    virtual void	setExtractionDefaults()		{}

    mDefineFactory2ParamInClass(uiPosProvGroup,uiParent*,const Setup&,factory);

};

#endif
