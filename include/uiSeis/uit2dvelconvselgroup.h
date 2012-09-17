#ifndef uit2dvelconvselgroup_h
#define uit2dvelconvselgroup_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: uit2dvelconvselgroup.h,v 1.1 2010/07/08 05:54:22 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uit2dconvsel.h"

#include "multiid.h"

class uiVelSel;
class IOPar;

mClass uiT2DVelConvSelGroup : public uiT2DConvSelGroup
{
public:
    				uiT2DVelConvSelGroup(uiParent*);

    static void			initClass();
    static uiT2DConvSelGroup*	create( uiParent* p )
				{ return new uiT2DVelConvSelGroup(p); }

    bool			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

protected:
    uiVelSel*			velsel_;
};


#endif
