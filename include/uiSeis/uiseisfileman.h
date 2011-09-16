#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiseisfileman.h,v 1.27 2011-09-16 10:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiobjfileman.h"
class uiToolButton;


mClass uiSeisFileMan : public uiObjFileMan
{
public:
			uiSeisFileMan(uiParent*,bool);
			~uiSeisFileMan();

    bool		is2D() const		{ return is2d_; }

    mDeclInstanceCreatedNotifierAccess(uiSeisFileMan);

protected:

    bool		is2d_;
    uiToolButton*	browsebut_;

    void		mergePush(CallBacker*);
    void		dump2DPush(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		man2DPush(CallBacker*);
    void		manPS(CallBacker*);
    void		makeDefault(CallBacker*);

    virtual void	mkFileInfo();
    virtual void	ownSelChg();
    double		getFileSize(const char*,int&) const;

    const char*		getDefKey() const;

};

#endif
