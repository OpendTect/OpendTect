#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id$
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

    enum Type		{ Hor2D, Hor3D, AnyHor, StickSet, Flt3D, Body };
			DeclareEnumUtils(Type);

			uiSurfaceMan(uiParent*,Type);
			~uiSurfaceMan();

    mDeclInstanceCreatedNotifierAccess(uiSurfaceMan);
    void		addTool(uiButton*);

protected:

    const Type		type_;

    uiListBox*		attribfld_;

    bool		isCur2D() const;
    bool		isCurFault() const;

    uiToolButton*	man2dbut_;
    void		copyCB(CallBacker*);
    void		man2dCB(CallBacker*);
    void		merge3dCB(CallBacker*);
    void		setRelations(CallBacker*);
    void		stratSel(CallBacker*);

    void		mergeBodyCB(CallBacker*);
    void		createBodyRegionCB(CallBacker*);
    void		switchValCB(CallBacker*);
    void		calVolCB(CallBacker*);

    void		removeAttribCB(CallBacker*);
    void		renameAttribCB(CallBacker*);

    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
    double		getFileSize(const char*,int&) const;
};


#endif
