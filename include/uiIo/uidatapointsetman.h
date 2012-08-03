#ifndef uicrossplotman_h
#define uicrossplotman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
 RCS:           $Id: uidatapointsetman.h,v 1.2 2012-08-03 13:00:59 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiobjfileman.h"

/*! \brief
CrossPlot manager
*/

mClass(uiIo) uiDataPointSetMan : public uiObjFileMan
{
public:
    				uiDataPointSetMan(uiParent*);
				~uiDataPointSetMan();

protected:

    void			mergePush(CallBacker*);

    void			mkFileInfo();

};

#endif

