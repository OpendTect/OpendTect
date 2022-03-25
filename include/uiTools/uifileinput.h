#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          21/9/2000
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigeninput.h"
#include "uifiledlg.h"
#include "file.h"

class uiButton;
class BufferStringSet;

/*! \brief A file-name input.

Displays a uiLineEdit field showing the current selected file. The user can
edit the filename by hand, or pop up a file selector trough the included
"Select..." push button. Optional is an Examine button for browsing the file.

The filter you specify will automatically get an 'All files' added unless
you specify otherwise in the setup.

*/

mExpClass(uiTools) uiFileInput : public uiGenInput
{ mODTextTranslationClass(uiFileInput);
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
	mDefSetupMemb(uiFileDialog::Type,filedlgtype) //!< Gen
	mDefSetupMemb(uiString,objtype)		//!< empty
    };

			uiFileInput(uiParent*,const uiString& seltxt,
				    const char* fnm=0);
			uiFileInput(uiParent*,const uiString& seltxt,
				    const Setup&);
			~uiFileInput();

    void		setFileName(const char*);
    void		setDefaultSelectionDir(const char*);
    const char*		defaultSelectionDir() const	   { return defseldir_;}
    void		setFilter( const char* fltr )	   { filter_ = fltr; }
    const char*		selectedFilter() const		   { return selfltr_; }
    void		setSelectedFilter( const char* f ) { selfltr_ = f; }
    void		setObjType( const uiString& s )    { objtype_ = s; }
    void		setExamine( const CallBack& cb )   { excb_ = cb; }
			//!< Overrules the simple stand-alone file browser

    const char*		fileName() const; // Full file path
    const char*		pathOnly() const;
    const char*		baseName() const; // Filename without path and extension
    void		getFileNames(BufferStringSet&) const;

    uiFileDialog::Mode	selectMode() const
			{
			    return selmodset_ ? selmode_
				 : (forread_  ? uiFileDialog::ExistingFile
					      : uiFileDialog::AnyFile);
			}

    void		setSelectMode( uiFileDialog::Mode m)
			{ selmodset_ = true;  selmode_ = m; }

    void		setSensitive(bool yn)	{ setChildrenSensitive(yn); }
    void		enableExamine(bool);
			//!< only if examinebut present
    void		setDefaultExtension(const char* ext);
			// only when forread is false

    void		selectFile( CallBacker* cb )	{ doSelect(cb); }

    void		setExamStyle( File::ViewStyle vs ) { examstyle_ = vs; }
    File::ViewStyle	examStyle() const		{ return examstyle_; }

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
    CallBack		excb_;
    BufferString	defaultext_;
    uiString		objtype_;

    bool		selmodset_;
    uiFileDialog::Mode  selmode_;
    uiFileDialog::Type  filedlgtype_;

    uiButton*		examinebut_;

    virtual void	doSelect(CallBacker*);
    void		inputChg(CallBacker*);
    void		examineFile(CallBacker*);
    void		isFinalized(CallBacker*);
    void		fnmEntered(CallBacker*);
};


mExpClass(uiTools) uiASCIIFileInput : public uiFileInput
{
public:
			uiASCIIFileInput(uiParent*,bool forread);
			uiASCIIFileInput(uiParent*,const uiString& lbl,
					 bool forread);
			~uiASCIIFileInput();

protected:

    void		fileSelCB(CallBacker*);
};
