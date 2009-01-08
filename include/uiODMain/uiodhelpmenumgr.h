#ifndef uiodhelpmnumgr_h
#define uiodhelpmnumgr_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id: uiodhelpmenumgr.h,v 1.8 2009-01-08 10:47:25 cvsranojay Exp $
________________________________________________________________________

-*/

#include "sets.h"

class uiODMenuMgr;
class uiPopupMenu;
class uiODHelpDocInfo;


/*!\brief The OpendTect help menu manager */

mClass uiODHelpMenuMgr
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
