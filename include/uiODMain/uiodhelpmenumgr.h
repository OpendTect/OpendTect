#ifndef uiodhelpmnumgr_h
#define uiodhelpmnumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id: uiodhelpmenumgr.h,v 1.7 2008-12-18 16:03:51 cvsbert Exp $
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

    void			handle(int,const char*);

protected:

    bool			havedtectdoc_;
    uiODMenuMgr*		mnumgr_;
    uiPopupMenu*		helpmnu_;
    ObjectSet<uiODHelpDocInfo>	aboutentries_;
    ObjectSet<uiODHelpDocInfo>	varentries_;
    ObjectSet<uiODHelpDocInfo>	creditsentries_;

    void			scanEntries(const char*);
    void			mkVarMenu();
    void			mkAboutMenu();
    void			mkCreditsMenu();
    bool			getPopupData(int,BufferString&);
    void			insertVarItem(uiPopupMenu*,int,bool);
};

#endif
