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
#include "fileformat.h"
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

	mDefSetupMemb(BufferString,filename)
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
	mDefSetupMemb(File::FormatList,formats)	//!< empty
	mDefSetupMemb(BufferString,defaultext)	//!< empty

	Setup& setFormat( const uiString& ftype, const char* ext,
			  const char* ext2=0, const char* ext3=0 )
	{
	    formats_.setEmpty();
	    formats_.addFormat( File::Format(ftype,ext,ext2,ext3) );
	    return *this;
	}
    };

			uiFileSel(uiParent*,const uiString& seltxt,
				    const char* fnm=0);
			uiFileSel(uiParent*,const uiString& seltxt,
				    const Setup&);
			~uiFileSel();

    const Setup&	setup() const		{ return setup_; }
    uiString		labelText() const;

    void		setFileName(const char*);
    void		setDefaultSelectionDir(const char*);
    void		setDefaultExtension( const char* s )
						{ setup_.defaultext_ = s; }
    const char*		defaultSelectionDir() const
			{ return setup_.defseldir_;}
    void		setFormats( const File::FormatList& fmts )
			{ setup_.formats_ = fmts; }
    void		setObjType( const uiString& s )    { objtype_ = s; }
    void		setExamine( const CallBack& cb )   { examinecb_ = cb; }
			    //!< Overrules the simple stand-alone file browser
    void		setLabelText(const uiString&);
    BufferString	selectedExtension() const;
    BufferString	selectedProtocol() const;

    const char*		fileName() const;
    void		getFileNames(BufferStringSet&) const;

    uiFileDialog::Mode	selectMode() const;
    void		setSelectMode(uiFileDialog::Mode);
    bool		inDirectorySelectMode() const;

    void		setSensitive(bool yn);

    void		selectFile( CallBacker* cb )	{ doSelCB(cb); }

    void		setChecked(bool);
    bool		isCheckable() const		{ return checkbox_; }
    bool		isChecked() const;

    Notifier<uiFileSel>	newSelection;
    Notifier<uiFileSel>	acceptReq;
    Notifier<uiFileSel>	checked;

protected:

    Setup		setup_;
    CallBack		examinecb_;
    uiString		objtype_;
    BufferStringSet	factnms_;
    bool		selmodset_;
    uiFileDialog::Mode  selmode_;

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

    void		init(const uiString&);

public:

			// raw, unchecked stuff
    const char*		text() const;
    void		setText(const char*);

};
