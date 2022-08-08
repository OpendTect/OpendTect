#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiobjfileman.h"
#include "enums.h"

class BufferStringSet;

class uiButton;
class uiListBox;
class uiStratLevelSel;
class uiToolButton;

mExpClass(uiEarthModel) uiSurfaceMan : public uiObjFileMan
{ mODTextTranslationClass(uiSurfaceMan);
public:

    enum Type		{ Hor2D, Hor3D, AnyHor, StickSet, Flt3D, Body, FltSet };
			mDeclareEnumUtils(Type);

			uiSurfaceMan(uiParent*,Type);
			~uiSurfaceMan();

    mDeclInstanceCreatedNotifierAccess(uiSurfaceMan);

protected:

    const Type		type_;

    uiListBox*		attribfld_;

    bool		isCur2D() const;
    bool		isCurFault() const;

    uiToolButton*	man2dbut_;
    uiToolButton*	surfdatarenamebut_;
    uiToolButton*	surfdataremovebut_;
    uiToolButton*	copybut_;
    uiToolButton*	mergehorbut_;
    uiToolButton*	applybodybut_;
    uiToolButton*	createregbodybut_;
    uiToolButton*	volestimatebut_;
    uiToolButton*	switchvalbut_;
    uiToolButton*	manselsetbut_ = nullptr;

    void		attribSel(CallBacker*);
    void		copyCB(CallBacker*);
    void		man2dCB(CallBacker*);
    void		merge3dCB(CallBacker*);
    void		setRelations(CallBacker*);
    void		stratSel(CallBacker*);

    void		mergeBodyCB(CallBacker*);
    void		createBodyRegionCB(CallBacker*);
    void		switchValCB(CallBacker*);
    void		calcVolCB(CallBacker*);

    void		removeAttribCB(CallBacker*);
    void		renameAttribCB(CallBacker*);

    void		mkFileInfo() override;
    void		fillAttribList();
    od_int64		getFileSize(const char*,int&) const override;
    void		setToolButtonProperties();
    void		ownSelChg() override;
    void		manFltSetCB(CallBacker*);

private:
    uiString		sRenameSelData();
    uiString		sRemoveSelData();
};


