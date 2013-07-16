#ifndef uiodhelpmenumgr_h
#define uiodhelpmenumgr_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Aug 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "sets.h"

class uiODMenuMgr;
class uiMenu;
class uiODHelpDocInfo;


/*!\brief The OpendTect help menu manager */

mExpClass(uiODMain) uiODHelpMenuMgr
{
public:
    				uiODHelpMenuMgr(uiODMenuMgr*);
    				~uiODHelpMenuMgr();

    void			handle(int,const char*);

protected:

    bool			havedtectdoc_;
    uiODMenuMgr*		mnumgr_;
    uiMenu*			helpmnu_;
    ObjectSet<uiODHelpDocInfo>	aboutentries_;
    ObjectSet<uiODHelpDocInfo>	varentries_;
    ObjectSet<uiODHelpDocInfo>	creditsentries_;

    void			scanEntries(const char*);
    void			mkVarMenu();
    void			mkAboutMenu();
    void			mkCreditsMenu();
    bool			getPopupData(int,BufferString&);
    void			insertVarItem(uiMenu*,int,bool);
};

#endif

