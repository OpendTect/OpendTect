#ifndef uiobjfileman_h
#define uiobjfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiobjfileman.h,v 1.11 2010-03-11 11:18:00 cvsbert Exp $
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
					     const IOObjContext&);
				~uiObjFileMan();

    static BufferString		getFileSizeString(double);

protected:

    uiTextEdit*			infofld;
    uiIOObjSelGrp*		selgrp;
    uiToolButton*		mkdefbut;

    IOObj*			curioobj_;
    IOObjContext&		ctxt_;
    bool			curimplexists_;

    void			createDefaultUI(bool needreloc=false);
    BufferString		getFileInfo();
    virtual void		mkFileInfo()			= 0;
    virtual double		getFileSize(const char*,int&) const;
    virtual const char*		getDefKey() const;

    void			selChg(CallBacker*);
    virtual void		ownSelChg()		{}
    void			makeDefault(CallBacker*);

};


#endif
