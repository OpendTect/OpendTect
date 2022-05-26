#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uidialog.h"

class IOObj;
class IOObjContext;
class uiButtonGroup;
class uiGroup;
class uiIOObjSelGrp;
class uiTextEdit;
class uiToolButton;


mExpClass(uiIo) uiObjFileMan : public uiDialog
{ mODTextTranslationClass(uiObjFileMan)
public:
				~uiObjFileMan();

    uiIOObjSelGrp*		selGroup()		{ return selgrp_; }
    const IOObj*		curIOObj() const	{ return curioobj_; }
    const IOObjContext&		ioobjContext() const	{ return ctxt_; }

    uiGroup*			listGroup()		{ return listgrp_; }
    uiGroup*			infoGroup()		{ return infogrp_; }
    uiButtonGroup*		extraButtonGroup()	{ return extrabutgrp_; }

    uiToolButton*		addExtraButton(const char* iconnm,
						const uiString& tooltip,
						const CallBack&);
    uiToolButton*		addManipButton(const char* iconnm,
						const uiString& tooltip,
						const CallBack&);

    void			getChosen(BufferStringSet&) const;
    void			getChosen(TypeSet<MultiID>&) const;
    void			fullUpdate(const MultiID&);

protected:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
					     const IOObjContext&,
					     const char* ctxtfilt = nullptr);
	//!<ctxtfilt can be either a translator group name or omf metadata key

    uiTextEdit*			infofld_;
    uiTextEdit*			notesfld_;
    uiIOObjSelGrp*		selgrp_;
    uiGroup*			listgrp_;
    uiGroup*			infogrp_;
    uiButtonGroup*		extrabutgrp_;

    IOObj*			curioobj_ = nullptr;
    IOObjContext&		ctxt_;
    bool			curimplexists_ = false;
    BufferString		ctxtfilter_;

    void			finalizeStartCB(CallBacker*);
    void			finalizeDoneCB(CallBacker*);
    void			saveNotes(CallBacker*);
    void			readNotes();
    void			setInfo(const char* txt);
    void			setPrefWidth(int width); //!< width in char
    void			createDefaultUI(bool needreloc=false,
						bool needremove=true,
						bool multisel=true);
    void			getTimeStamp(const char*,BufferString&);
    void			getTimeLastModified(const char*,BufferString&);
    BufferString		getFileInfo();
    virtual void		mkFileInfo()			= 0;
    virtual od_int64		getFileSize(const char*,int&) const;

    void			selChg(CallBacker*);
    virtual void		initDlg()		{}
    virtual void		ownSelChg()		{}
    void			updateCB(CallBacker*);
    virtual void		updateList()		{}
};


