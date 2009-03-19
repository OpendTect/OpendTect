#ifndef uiobjfileman_h
#define uiobjfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiobjfileman.h,v 1.7 2009-03-19 09:01:54 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class IOObjContext;
class uiIOObjSelGrp;
class uiToolButton;
class uiTextEdit;


mClass uiObjFileMan : public uiDialog
{
public:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
					     const IOObjContext&,
					     const char* survdefaultkey=0);
				~uiObjFileMan();

    static BufferString		getFileSizeString(double);

protected:

    uiTextEdit*			infofld;
    uiIOObjSelGrp*		selgrp;
    uiToolButton*		mkdefbut;

    IOObj*			curioobj_;
    IOObjContext&		ctxt_;
    BufferString		defkey_;

    void			createDefaultUI();
    BufferString		getFileInfo();
    virtual void		mkFileInfo()			=0;
    virtual double		getFileSize(const char*,int&) const;

    void			selChg(CallBacker*);
    virtual void		ownSelChg()			{}
    void			makeDefault(CallBacker*);

};


#endif
