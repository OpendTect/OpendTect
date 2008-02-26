#ifndef uiposfiltgroup_h
#define uiposfiltgroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposfiltgroup.h,v 1.3 2008-02-26 08:55:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "factory.h"
class IOPar;


/*! \brief group for providing positions, usually for 2D or 3D seismics */

class uiPosFiltGroup : public uiGroup
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
