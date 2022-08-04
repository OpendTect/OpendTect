#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "file.h"
#include "uistring.h"
class uiTextEdit;
class uiTextBrowser;
class uiTable;


mExpClass(uiTools) uiTextFile : public CallBacker
{ mODTextTranslationClass(uiTextFile);
public:

    typedef File::ViewPars Setup;

			uiTextFile( uiParent* p, const char* fnm,
				    const Setup& s=Setup() )
			    : setup_(s)
			    , filename_(fnm)
			    , fileNmChg(this)	{ init(p); }

    const char*		fileName() const	{ return filename_; }
    bool		isModified () const	{ return ismodified_; }
    int			maxLines() const	{ return setup_.maxnrlines_; }
    virtual bool	isEditable() const	{ return setup_.editable_; }
    virtual bool	isTable() const		{ return setup_.style_ ==
								 File::Table; }

    bool		open(const char*);
    bool		reLoad();
    bool		save();
    bool		saveAs(const char*);

    int			nrLines() const;
    void		toLine(int);

    const char*		text() const;

    uiTextEdit*		textEdit()		{ return txted_; }
    uiTextBrowser*	textBrowser()		{ return txtbr_; }
    uiTable*		tableEditor()		{ return tbl_; }
    uiObject*		uiObj();

    Notifier<uiTextFile> fileNmChg;

protected:

    Setup		setup_;
    BufferString	filename_;
    mutable bool	ismodified_;

    uiTextEdit*		txted_;
    uiTextBrowser*	txtbr_;
    uiTable*		tbl_;

    void		init(uiParent*);
    void		setFileName(const char*);

    void		valChg(CallBacker*);

};


mExpClass(uiTools) uiTextFileDlg : public uiDialog
{ mODTextTranslationClass(uiTextFileDlg);
public:

    mExpClass(uiTools) Setup : public uiDialog::Setup
    {
    public:
	Setup( const uiString& winttl = uiString::emptyString() )
	    : uiDialog::Setup(winttl.isSet() ? winttl : tr("File viewer"),
				      mNoDlgTitle,mNoHelpKey)
		    , scroll2bottom_(false)
		    , allowopen_(false)
		    , allowsave_(false)
		{
		    oktext(uiStrings::sClose())
			.canceltext( uiStrings::sReload() )
			.separator(false).modal(false).menubar(true);
		}

	mDefSetupMemb(bool, scroll2bottom)
	mDefSetupMemb(bool, allowopen)
	mDefSetupMemb(bool, allowsave)

    };

			uiTextFileDlg(uiParent* p,const char* fnm,
					bool rdonly=false,bool tbl=false);
			uiTextFileDlg( uiParent* p,const Setup&);
			uiTextFileDlg( uiParent* p, const uiTextFile::Setup& ts,
					const Setup& s, const char* fnm )
			    : uiDialog(p,s)	{ init(s,ts,fnm); }

    uiTextFile*		editor()		{ return editor_; }
    void		setFileName(const char*);
    const char*		fileName() const	{ return editor_->fileName(); }

protected:

    uiTextFile*		editor_;
    bool		captionisfilename_;

    void		open(CallBacker*);
    void		save(CallBacker*);
    void		saveAs(CallBacker*);
    void		dismiss(CallBacker*);
    bool		rejectOK(CallBacker*) override;
    bool		acceptOK(CallBacker*) override;
    void		finalizeCB(CallBacker*);

    void		fileNmChgd(CallBacker*);
    void		init(const Setup&,const uiTextFile::Setup&,const char*);
    bool		okToExit();
    int			doMsg(const char*,bool iserr=true);
};


