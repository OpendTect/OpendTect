#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uiobjfileman.h"
#include "emmanager.h"
#include "enums.h"

class BufferStringSet;

class uiButton;
class uiListBox;
class uiStratLevelSel;
class uiToolButton;

mExpClass(uiEarthModel) uiSurfaceMan : public uiObjFileMan
{ mODTextTranslationClass(uiSurfaceMan);
public:
			uiSurfaceMan(uiParent*,EM::ObjectType);
			~uiSurfaceMan();

    mDeclInstanceCreatedNotifierAccess(uiSurfaceMan);

protected:

    const EM::ObjectType		type_;

    uiListBox*		attribfld_		= nullptr;

    bool		isCur2D() const;
    bool		isCurFault() const;

    uiToolButton*	man2dbut_		= nullptr;
    uiToolButton*	surfdatarenamebut_	= nullptr;
    uiToolButton*	surfdataremovebut_	= nullptr;
    uiToolButton*	copybut_		= nullptr;
    uiToolButton*	mergehorbut_		= nullptr;
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
    void		copyFault2FaultSetCB(CallBacker*);

private:
    uiString		sRenameSelData();
    uiString		sRemoveSelData();
};
