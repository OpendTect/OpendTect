#ifndef uiwellman_h
#define uiwellman_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:           2003
 RCS:           $Id: uiwellman.h,v 1.13 2009-04-21 11:36:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

class uiListBox;

namespace Well { class Data; class Reader; };


/*! \brief
Well manager
*/

mClass uiWellMan : public uiObjFileMan
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
    void			edWellTrack(CallBacker*);
    void			edD2T(CallBacker*);
    void			edChckSh(CallBacker*);
    void			addLogs(CallBacker*);
    void			exportLogs(CallBacker*);

    double			getFileSize(const char*,int&) const;
    void			defD2T(bool);

};

#endif
