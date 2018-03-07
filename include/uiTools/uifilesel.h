#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          21/9/2000
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "fileview.h"
#include "uigroup.h"
#include "uifileselector.h"

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

    typedef OD::FileSelectionMode	SelectionMode;
    typedef OD::FileContentType		ContentType;
    typedef File::ViewStyle		ViewStyle;

    mExpClass(uiTools) Setup : public uiFileSelectorSetup
    {
    public:
			Setup(const char* filenm=0);
			Setup(OD::FileContentType,const char* filenm=0);

	mDefSetupMemb(bool,withexamine)		//!< false (unless spec. Txt)
	mDefSetupMemb(ViewStyle,examstyle)	//!< File::Text (Bin if Img)
	mDefSetupMemb(bool,exameditable)	//!< false
	mDefSetupMemb(bool,displaylocalpath)	//!< false
	mDefSetupMemb(bool,checkable)		//!< false
	mDefSetupMemb(ContentType,contenttype)	//!< General
	mDefSetupMemb(uiString,objtype)		//!< empty
    };

			uiFileSel(uiParent*,const uiString& seltxt,
				    const char* fnm=0);
			uiFileSel(uiParent*,const uiString& seltxt,
				    const Setup&);
			~uiFileSel();

    Setup&		setup()			{ return setup_; }
    const Setup&	setup() const		{ return setup_; }
    uiString		labelText() const;
    bool		isCheckable() const	{ return checkbox_; }

    void		setFileName(const char*);
    void		setFileNames(const BufferStringSet&);
    void		setSelectionMode(SelectionMode);
    void		setObjType( const uiString& s )	    { objtype_ = s; }
    void		setLabelText(const uiString&);
    void		setSensitive(bool yn);
    void		setChecked(bool);

    const char*		fileName() const;	//!< the first if multiple
    void		getFileNames(BufferStringSet&) const;
    BufferString	selectedExtension() const;
    BufferString	selectedProtocol() const;
    bool		isChecked() const;

    Notifier<uiFileSel>	newSelection;
    Notifier<uiFileSel>	acceptReq;
    Notifier<uiFileSel>	checked;

    void		selectFile( CallBacker* cb )	{ doSelCB(cb); }
    uiButton*		selectButton()			{ return selbut_; }

protected:

    Setup		setup_;
    uiString		objtype_;
    BufferStringSet	factnms_;

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
    const char*		protKy(int) const;
    const File::SystemAccess& fsAccess(int) const;
    const char*		curProtKy() const;
    void		setSel(const BufferStringSet&);

private:

    void		init(const uiString&);

public:

			// raw, unchecked stuff
    const char*		text() const;
    void		setText(const char*);

};
