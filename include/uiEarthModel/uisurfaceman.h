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
#include "uidialog.h"
#include "uiobjfileman.h"

class BufferStringSet;
namespace EM { class IOObjInfo; }

class uiButton;
class uiListBox;
class uiStratLevelSel;
class uiTextEdit;
class uiToolButton;

mExpClass(uiEarthModel) uiSurfaceMan : public uiObjFileMan
{
public:
			uiSurfaceMan(uiParent*,const char* typ);
			~uiSurfaceMan();

    mDeclInstanceCreatedNotifierAccess(uiSurfaceMan);
    void		addTool(uiButton*);

protected:

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


mExpClass(uiEarthModel) uiSurface2DMan : public uiDialog
{
public:
    			uiSurface2DMan(uiParent*,const EM::IOObjInfo&);
protected:
    void		lineSel(CallBacker*);

    uiListBox*		linelist_;
    uiTextEdit*		infofld_;
    const EM::IOObjInfo& eminfo_;
};


#endif

