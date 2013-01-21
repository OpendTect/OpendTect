#ifndef uiobjfileman_h
#define uiobjfileman_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class IOObj;
class IOObjContext;
class uiButton;
class uiGroup;
class uiIOObjSelGrp;
class uiToolButton;
class uiTextEdit;


mExpClass(uiIo) uiObjFileMan : public uiDialog
{
public:
				~uiObjFileMan();

    static BufferString		getFileSizeString(double);

    uiIOObjSelGrp*		selGroup()		{ return selgrp_; }
    const IOObj*		curIOObj() const	{ return curioobj_; }
    const IOObjContext&		ioobjContext() const	{ return ctxt_; }

    virtual void		addTool(uiButton*);
    uiGroup*			listGroup()		{ return listgrp_; }
    uiGroup*			infoGroup()		{ return infogrp_; }

protected:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
					     const IOObjContext&);

    uiTextEdit*			infofld_;
    uiTextEdit*			notesfld_;
    uiIOObjSelGrp*		selgrp_;
    uiGroup*			listgrp_;
    uiGroup*			infogrp_;
    uiButton*			lastexternal_;

    IOObj*			curioobj_;
    IOObjContext&		ctxt_;
    bool			curimplexists_;

    void			saveNotes(CallBacker*);
    void			readNotes();
    void			setInfo(const char* txt);
    void			setPrefWidth(int width); //!< width in char
    void			createDefaultUI(bool needreloc=false);
    BufferString		getFileInfo();
    virtual void		mkFileInfo()			= 0;
    virtual double		getFileSize(const char*,int&) const;

    void			selChg(CallBacker*);
    virtual void		ownSelChg()		{}

};


#endif

