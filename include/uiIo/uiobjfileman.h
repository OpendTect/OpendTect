#ifndef uiobjfileman_h
#define uiobjfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uiobjfileman.h,v 1.13 2010-09-30 09:53:16 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class IOObjContext;
class uiButton;
class uiIOObjSelGrp;
class uiToolButton;
class uiTextEdit;


mClass uiObjFileMan : public uiDialog
{
public:
				~uiObjFileMan();

    static BufferString		getFileSizeString(double);

    uiIOObjSelGrp*		selGroup()		{ return selgrp; }
    const IOObj*		curIOObj() const	{ return curioobj_; }
    const IOObjContext&		ioobjContext() const	{ return ctxt_; }

protected:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
					     const IOObjContext&);

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
