#ifndef uiodhelpmnumgr_h
#define uiodhelpmnumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id: uiodhelpmenumgr.h,v 1.1 2005-08-22 07:30:43 cvsbert Exp $
________________________________________________________________________

-*/

#include "gendefs.h"
class uiODMenuMgr;


/*!\brief The OpendTect help menu manager */

class uiODHelpMenuMgr
{
public:

    			uiODHelpMenuMgr(uiODMenuMgr*);

    void		handle(int,const char*);

};


#endif
