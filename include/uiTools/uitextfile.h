#ifndef uitextfile_h
#define uitextfile_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          09/02/2001
 RCS:           $Id: uitextfile.h,v 1.6 2012-08-03 13:01:16 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
class uiTextEdit;
class uiTextBrowser;
class uiTable;


mClass(uiTools) uiTextFile : public CallBacker
{
public:

    mClass(uiTools) Setup
    {
    public:

			Setup( bool rdonly, bool tbl, const char* fname=0 )
			    : readonly_(rdonly)
			    , table_(tbl)
			    , filename_(fname)
			    , maxlines_(mUdf(int))
			    , logviewmode_(false)
			{
			}

	mDefSetupMemb(bool,readonly)
	mDefSetupMemb(bool,table)
	mDefSetupMemb(BufferString,filename)
	mDefSetupMemb(int,maxlines)
	mDefSetupMemb(bool,logviewmode)

    };

    			uiTextFile( uiParent* p, const Setup& s )
			    : setup_(s)
			    , fileNmChg(this)	{ init(p); }
    			uiTextFile( uiParent* p, bool rdonly, bool tbl,
						 const char* fname )
			    : setup_(rdonly,tbl,fname)
			    , fileNmChg(this)	{ init(p); }

    virtual bool	isEditable() const	{ return !setup_.readonly_; }
    bool		isModified () const;
    virtual bool	isTable() const		{ return setup_.table_; }
    const char*		fileName() const	{ return setup_.filename_; }
    int			maxLines() const	{ return setup_.maxlines_; }

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
    uiTextEdit*		txted_;
    uiTextBrowser*	txtbr_;
    uiTable*		tbl_;

    void		init(uiParent*);

};


mClass(uiTools) uiTextFileDlg : public uiDialog
{
public:

    mClass(uiTools) Setup : public uiDialog::Setup
    {
    public:

			Setup( const char* fname=0 )
			    : uiDialog::Setup(fname?fname:"File viewer",
				    		0,mNoHelpID)
			    , scroll2bottom_(false)
			    , allowopen_(false)
			    , allowsave_(false)
			{
			    oktext("&Dismiss").canceltext("&Reload")
			    .separator(false).modal(false).menubar(true);
			}

	mDefSetupMemb(bool,	scroll2bottom)
	mDefSetupMemb(bool,	allowopen)
	mDefSetupMemb(bool,	allowsave)

    };

    			uiTextFileDlg(uiParent* p,bool rdonly,bool tbl,
				      const char* fnm);
			uiTextFileDlg(uiParent* p,const Setup&);
			uiTextFileDlg( uiParent* p, const uiTextFile::Setup& ts,
						    const Setup& s )
			    : uiDialog(p,s)	{ init(s,ts); }

    uiTextFile*		editor()		{ return editor_; }
    bool		setFileName( const char* fn )
						{ return editor_->open(fn); }
    const char*		fileName() const
						{ return editor_->fileName(); }

protected:

    uiTextFile*		editor_;
    bool		scroll2bottom_;

    void		open(CallBacker*);
    void		save(CallBacker*);
    void		saveAs(CallBacker*);
    void		dismiss(CallBacker*);
    bool		rejectOK(CallBacker*);
    bool		acceptOK(CallBacker*);

    void		fileNmChgd(CallBacker*);
    void		init(const Setup&,const uiTextFile::Setup&);
    bool		okToExit();
    int			doMsg(const char*,bool iserr=true);
};


#endif

