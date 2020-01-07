#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2002
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uidialog.h"

class IOObj;
class IOObjContext;
class uiButton;
class uiButtonGroup;
class uiGroup;
class uiIOObjSelGrp;
class uiTextEdit;


mExpClass(uiIo) uiObjFileMan : public uiDialog
{ mODTextTranslationClass(uiObjFileMan)
public:
				~uiObjFileMan();

    uiIOObjSelGrp*		selGroup()		{ return selgrp_; }
    const IOObj*		curIOObj() const	{ return curioobj_; }
    const IOObjContext&		ioobjContext() const	{ return ctxt_; }
    bool			getItemInfo( const IOObj& ioobj,
					     uiPhraseSet& info ) const
				{ return gtItemInfo( ioobj, info ); }

    uiGroup*			listGroup()		{ return listgrp_; }
    uiGroup*			infoGroup()		{ return infogrp_; }
    uiButtonGroup*		extraButtonGroup()	{ return extrabutgrp_; }

protected:
				uiObjFileMan(uiParent*,const uiDialog::Setup&,
			    const IOObjContext&,const char* ctxtfilt = nullptr);

    uiTextEdit*			infofld_;
    uiTextEdit*			notesfld_;
    uiIOObjSelGrp*		selgrp_;
    uiGroup*			listgrp_;
    uiGroup*			infogrp_;
    uiButtonGroup*		extrabutgrp_;

    IOObj*			curioobj_;
    IOObjContext&		ctxt_;
    bool			curimplexists_;
    BufferString		ctxtfilt_;

    void			finaliseStartCB(CallBacker*);
    void			saveNotes(CallBacker*);
    void			selChg(CallBacker*);

    void			updateFromSelected();
    void			setInfo(const uiString&);
    void			setPrefWidth(int width); //!< width in char
    void			createDefaultUI(bool needreloc=false,
						bool needremove=true,
						bool multisel=true);
    void			getTimeStamp(const char*,BufferString&) const;
    void			getTimeLastModified(const char*,
						    BufferString&) const;

    virtual bool		gtItemInfo(const IOObj&,uiPhraseSet&) const= 0;
				//!< used to be in 'mkFileInfo'
    virtual void		ownSelChg()		{}
    virtual od_int64		getFileSize(const char*,int&) const;

    uiString&			addObjInfo(uiPhraseSet&,const uiWord& subj,
					    const uiString& val) const;
    template <class T>
    inline uiString&		addObjInfo( uiPhraseSet& ps,
				    const uiWord& subj, const T& val ) const
				{ return addObjInfo(ps,subj,toUiString(val)); }

private:

    bool			getFileInfo(const IOObj&,uiPhraseSet&) const;
    void			refreshItemInfo();
    void			readNotes();

};
