#ifndef uiposprovgroupstd_h
#define uiposprovgroupstd_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovgroupstd.h,v 1.2 2008-02-07 16:51:37 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposprovgroup.h"

/*! \brief UI for RangePosProvider */

class uiRangePosProvGroup : public uiPosProvGroup
{
public:
			uiRangePosProvGroup(uiParent*,
					   const uiPosProvider::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    static uiPosProvGroup*	create( uiParent* p, const uiPosProvider::Setup& s )
    			{ return new uiRangePosProvGroup(p,s); }
    static void		initClass();
};


/*! \brief UI for PolyPosProvider */

class uiPolyPosProvGroup : public uiPosProvGroup
{
public:
			uiPolyPosProvGroup(uiParent*,
					   const uiPosProvider::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvider::Setup& s )
    			{ return new uiPolyPosProvGroup(p,s); }
    static void		initClass();
};


#endif
