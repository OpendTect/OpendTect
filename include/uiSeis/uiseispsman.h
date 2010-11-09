#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.10 2010-11-09 04:41:37 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"


mClass uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

    static Notifier<uiSeisPreStackMan>* fieldsCreated();

protected:

    bool		is2d_;

    void		mkFileInfo();

    void		copyPush(CallBacker*);
    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif
