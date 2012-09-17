#ifndef uicrossplotman_h
#define uicrossplotman_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          August 2011
 RCS:           $Id: uidatapointsetman.h,v 1.1 2011/09/05 10:51:41 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

/*! \brief
CrossPlot manager
*/

mClass uiDataPointSetMan : public uiObjFileMan
{
public:
    				uiDataPointSetMan(uiParent*);
				~uiDataPointSetMan();

protected:

    void			mergePush(CallBacker*);

    void			mkFileInfo();

};

#endif
