#ifndef uiodhelpmenumgr_h
#define uiodhelpmenumgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id: uiodhelpmenumgr.h,v 1.10 2010/08/04 14:49:36 cvsbert Exp $
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
