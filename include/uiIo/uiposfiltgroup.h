#ifndef uiposfiltgroup_h
#define uiposfiltgroup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposfiltgroup.h,v 1.5 2009/07/22 16:01:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "factory.h"
class IOPar;


/*! \brief group for providing positions, usually for 2D or 3D seismics */

mClass uiPosFiltGroup : public uiGroup
{
public:

    struct Setup
    {
			Setup( bool is_2d )
			    : is2d_(is_2d)		{}

	virtual	~Setup()				{}
	mDefSetupMemb(bool,is2d)
    };

			uiPosFiltGroup(uiParent*,const Setup&);

    virtual void	usePar(const IOPar&)		= 0;
    virtual bool	fillPar(IOPar&) const		= 0;
    virtual void	getSummary(BufferString&) const	= 0;

    mDefineFactory2ParamInClass(uiPosFiltGroup,uiParent*,const Setup&,factory);

};


#endif
