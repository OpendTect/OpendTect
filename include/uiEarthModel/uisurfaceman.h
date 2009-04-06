#ifndef uisurfaceman_h
#define uisurfaceman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uisurfaceman.h,v 1.20 2009-04-06 03:39:28 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uiobjfileman.h"
class BufferStringSet;
namespace EM { class SurfaceIOData; }

class uiListBox;
class uiTextEdit;
class uiToolButton;


mClass uiSurfaceMan : public uiObjFileMan
{
public:
			uiSurfaceMan(uiParent*,const char* typ);
			~uiSurfaceMan();

protected:

    uiListBox*		attribfld;

    bool		isCur2D() const;
    bool		isCurFault() const;

    uiToolButton*	man2dbut_;
    void		copyCB(CallBacker*);
    void		man2d(CallBacker*);
    void		setRelations(CallBacker*);
    void		stratSel(CallBacker*);

    void		removeAttribCB(CallBacker*);
    void		renameAttribCB(CallBacker*);

    void		mkFileInfo();
    void		fillAttribList(const BufferStringSet&);
    double		getFileSize(const char*,int&) const;
};


mClass uiSurface2DMan : public uiDialog
{
public:
    			uiSurface2DMan(uiParent*,const EM::SurfaceIOData&);
protected:
    void		lineSel(CallBacker*);

    uiListBox*		linelist_;
    uiTextEdit*		infofld_;
    const EM::SurfaceIOData& sd_;
};


#endif
