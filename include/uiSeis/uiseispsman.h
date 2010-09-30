#ifndef uiseispsman_h
#define uiseispsman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2006
 RCS:           $Id: uiseispsman.h,v 1.9 2010-09-30 10:03:34 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"


class uiButton;

mClass uiSeisPreStackMan : public uiObjFileMan
{
public:
			uiSeisPreStackMan(uiParent*,bool for2d);
			~uiSeisPreStackMan();

    static Notifier<uiSeisPreStackMan>* fieldsCreated();
    void		addTool(uiButton*);

protected:

    uiButton*		lastexternal_;
    bool		is2d_;

    void		mkFileInfo();

    void		copyPush(CallBacker*);
    void                mergePush(CallBacker*);
    void                mkMultiPush(CallBacker*);
};


#endif
