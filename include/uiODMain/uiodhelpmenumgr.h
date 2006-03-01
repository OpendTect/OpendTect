#ifndef uiodhelpmnumgr_h
#define uiodhelpmnumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id: uiodhelpmenumgr.h,v 1.3 2006-03-01 16:46:49 cvsnanne Exp $
________________________________________________________________________

-*/

#include "sets.h"
class uiODMenuMgr;
class uiPopupMenu;
class uiODHelpDocInfo;


/*!\brief The OpendTect help menu manager */

class uiODHelpMenuMgr
{
public:

    			uiODHelpMenuMgr(uiODMenuMgr*);
    			~uiODHelpMenuMgr();

    void		handle(int,const char*);

protected:

    bool		havedtectdoc;
    uiODMenuMgr*	mnumgr;
    uiPopupMenu*	helpmnu;
    ObjectSet<uiODHelpDocInfo>	entries;

    void		mkVarMenu();
    void		insertVarItem(uiPopupMenu*,int,bool);

};


#endif
