#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          21/9/2000
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"
#include "uifiledlg.h"
#include "file.h"

class uiButton;
class uiCheckBox;
class uiComboBox;
class uiLabel;
class uiLineEdit;
class BufferStringSet;
namespace File { class SystemAccess; }

/*! \brief A selector of file names, local, web or cloud. */

mExpClass(uiTools) uiFileSel : public uiGroup
{ mODTextTranslationClass(uiFileSel);
public:

    mExpClass(uiTools) Setup
    {
    public:
			Setup(const char* filenm=0);
			Setup(uiFileDialog::Type t,const char* filenm=0);

	BufferString	fnm;

	mDefSetupMemb(BufferString,filter)	//!< empty
	mDefSetupMemb(BufferString,defseldir)	//!< empty
	mDefSetupMemb(bool,forread)		//!< true
	mDefSetupMemb(bool,withexamine)		//!< false (unless spec. Txt)
	mDefSetupMemb(File::ViewStyle,examstyle) //!< File::Text (Bin if Img)
	mDefSetupMemb(bool,exameditable)	//!< false
	mDefSetupMemb(bool,displaylocalpath)	//!< false

	mDefSetupMemb(bool,directories)		//!< false
	mDefSetupMemb(bool,allowallextensions)	//!< true
	mDefSetupMemb(bool,confirmoverwrite)	//!< true
	mDefSetupMemb(bool,checkable)		//!< false
	mDefSetupMemb(uiFileDialog::Type,filedlgtype) //!< Gen
	mDefSetupMemb(uiString,objtype)		//!< empty
    };

			uiFileSel(uiParent*,const uiString& seltxt,
				    const char* fnm=0);
			uiFileSel(uiParent*,const uiString& seltxt,
				    const Setup&);
			~uiFileSel();

    void		setFileName(const char*);
    void		setDefaultSelectionDir(const char*);
    const char*		defaultSelectionDir() const	   { return defseldir_;}
    void		setFilter( const char* fltr )	   { filter_ = fltr; }
    const char*		selectedFilter() const		   { return selfltr_; }
    void		setSelectedFilter( const char* f ) { selfltr_ = f; }
    void		setObjType( const uiString& s )    { objtype_ = s; }
    void		setExamine( const CallBack& cb )   { examinecb_ = cb; }
			//!< Overrules the simple stand-alone file browser
    void		setNoManualEdit();
    uiString		labelText() const;
    void		setLabelText(const uiString&);

    const char*		fileName() const;
    void		getFileNames(BufferStringSet&) const;

    uiFileDialog::Mode	selectMode() const;
    void		setSelectMode(uiFileDialog::Mode);
    bool		inDirectorySelectMode() const;

    void		setSensitive(bool yn);
    void		setDefaultExtension(const char* ext);
			// only when forread is false

    void		selectFile( CallBacker* cb )	{ doSelCB(cb); }

    void		setChecked(bool);
    bool		isCheckable() const		{ return checkbox_; }
    bool		isChecked() const;

    Notifier<uiFileSel>	newSelection;
    Notifier<uiFileSel>	acceptReq;
    Notifier<uiFileSel>	checked;

protected:

    bool		forread_;
    BufferString	filter_;
    BufferString	defseldir_;
    bool		displaylocalpath_;
    BufferString	selfltr_;
    bool		addallexts_;
    File::ViewStyle	examstyle_;
    bool		exameditable_;
    bool		confirmoverwrite_;
    CallBack		examinecb_;
    BufferString	defaultext_;
    uiString		objtype_;
    BufferStringSet	factnms_;

    bool		selmodset_;
    uiFileDialog::Mode  selmode_;
    uiFileDialog::Type  filedlgtype_;

    uiCheckBox*		checkbox_;
    uiComboBox*		protfld_;
    uiLabel*		lbl_;
    uiLineEdit*		fnmfld_;
    uiButton*		selbut_;
    uiButton*		examinebut_;

    void		doSelCB(CallBacker*);
    void		inputChgCB(CallBacker*);
    void		fnmEnteredCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		examineFileCB(CallBacker*);
    void		atFinaliseCB(CallBacker*);
    void		protChgCB(CallBacker*);

    void		setButtonStates();
    const File::SystemAccess& fsAccess(int) const;

private:

    void		init(const Setup&,const uiString&);

public:

			// raw, unchecked stuff
    const char*		text() const;
    void		setText(const char*);

};
