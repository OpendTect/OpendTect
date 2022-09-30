#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uigroup.h"
#include "factory.h"


/*! \brief group for providing positions, usually for 2D or 3D seismics */

mExpClass(uiIo) uiPosFiltGroup : public uiGroup
{
public:

    struct Setup
    {
			Setup( bool is_2d )
			    : is2d_(is_2d)		{}

	virtual		~Setup()			{}
	mDefSetupMemb(bool,is2d)
    };

			uiPosFiltGroup(uiParent*,const Setup&);
			~uiPosFiltGroup();

    virtual void	usePar(const IOPar&)		= 0;
    virtual bool	fillPar(IOPar&) const		= 0;
    virtual void	getSummary(BufferString&) const	= 0;

    mDefineFactory2ParamInClass(uiPosFiltGroup,uiParent*,const Setup&,factory);

};
