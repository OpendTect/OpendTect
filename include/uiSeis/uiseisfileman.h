#ifndef uiseisfileman_h
#define uiseisfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uiobjfileman.h"
class uiToolButton;


mExpClass(uiSeis) uiSeisFileMan : public uiObjFileMan
{ mODTextTranslationClass(uiSeisFileMan);
public:
			uiSeisFileMan(uiParent*,bool);
			~uiSeisFileMan();

    bool		is2D() const		{ return is2d_; }

    mDeclInstanceCreatedNotifierAccess(uiSeisFileMan);

protected:

    bool		is2d_;
    uiToolButton*	browsebut_;
    uiToolButton*	attribbut_;
    uiToolButton*	copybut_;
    uiToolButton*	man2dlinesbut_;
    uiToolButton*	mergecubesbut_;

    void		mergePush(CallBacker*);
    void		browsePush(CallBacker*);
    void		copyPush(CallBacker*);
    void		man2DPush(CallBacker*);
    void		manPS(CallBacker*);
    void		showAttribSet(CallBacker*);

    virtual void	mkFileInfo();
    virtual void	ownSelChg();
    od_int64		getFileSize(const char*,int&) const;
    void		setToolButtonProperties();

};

#endif

