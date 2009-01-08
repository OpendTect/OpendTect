#ifndef uiobjfileman_h
#define uiobjfileman_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiobjfileman.h,v 1.6 2009-01-08 07:23:07 cvsranojay Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class IOObjContext;
class uiIOObjSelGrp;
class uiTextEdit;


mClass uiObjFileMan : public uiDialog
{
public:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
					     const IOObjContext&);
				~uiObjFileMan();

    static BufferString		getFileSizeString(double);

protected:

    uiTextEdit*			infofld;
    uiIOObjSelGrp*		selgrp;

    IOObj*			curioobj_;
    IOObjContext&		ctxt_;

    void			createDefaultUI();
    BufferString		getFileInfo();
    virtual void		mkFileInfo()			=0;
    virtual double		getFileSize(const char*,int&) const;

    void			selChg(CallBacker*);
    virtual void		ownSelChg()			{}

};


#endif
