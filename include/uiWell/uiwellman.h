#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.9 2004-10-28 15:01:42 nanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiListBox;

namespace Well { class Data; class Reader; };


/*! \brief
Well manager
*/

class uiWellMan : public uiObjFileMan
{
public:
    				uiWellMan(uiParent*);
				~uiWellMan();

protected:

    uiListBox*			logsfld;

    Well::Data*			welldata;
    Well::Reader*		wellrdr;
    BufferString		fname;

    void			ownSelChg();
    void			getCurrentWell();
    void			mkFileInfo();
    void			fillLogsFld();
    void			removeLogPush(CallBacker*);
    void			renameLogPush(CallBacker*);

    void			edMarkers(CallBacker*);
    void			edD2T(CallBacker*);
    void			addLogs(CallBacker*);
    void			exportLogs(CallBacker*);
};

#endif
