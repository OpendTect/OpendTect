#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.24 2010-12-14 08:50:32 cvsbruno Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"

mClass uiSeisFileMan : public uiObjFileMan
{
public:
			uiSeisFileMan(uiParent*,bool);
			~uiSeisFileMan();

    bool		is2D() const		{ return is2d_; }

    static Notifier<uiSeisFileMan>* fieldsCreated();

protected:

    bool		is2d_;

    void		mergePush(CallBacker*);
    void		dump2DPush(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		importFromOtherSurvPush(CallBacker*);
    void		man2DPush(CallBacker*);
    void		manPS(CallBacker*);
    void		makeDefault(CallBacker*);

    void		mkFileInfo();
    double		getFileSize(const char*,int&) const;

    const char*		getDefKey() const;

};

#endif
