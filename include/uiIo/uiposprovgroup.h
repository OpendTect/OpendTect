#ifndef uiposprovgroup_h
#define uiposprovgroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovgroup.h,v 1.1 2008-02-07 16:10:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposprovider.h"
#include "factory.h"


/*! \brief group for subselecting an area for 2D or 3D seismics */

class uiPosProvGroup : public uiGroup
{
public:
			uiPosProvGroup(uiParent*,const uiPosProvider::Setup&);

    virtual void	usePar(const IOPar&)		= 0;
    virtual bool	fillPar(IOPar&) const		= 0;

    mDefineFactory2ParamInClass(uiPosProvGroup,uiParent*,
	    			const uiPosProvider::Setup&,factory);

};


#endif
