#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "file.h"
#include "uigroup.h"
#include "uifileselector.h"

class uiButton;
class uiCheckBox;
class uiComboBox;
class uiLabel;
class uiLineEdit;
class BufferStringSet;
namespace OD { class FileSystemAccess; }


/*! \brief A selector of file names, local, web or cloud. */

mExpClass(uiTools) uiFileSel : public uiGroup
{ mODTextTranslationClass(uiFileSel);
public:

    typedef OD::FileSelectionMode	SelectionMode;
    typedef OD::FileContentType		ContentType;

    mExpClass(uiTools) Setup : public uiFileSelectorSetup
    {
    public:
			Setup(const char* filenm=0);
			Setup(OD::FileContentType,const char* filenm=0);

	mDefSetupMemb(bool,withexamine)		//!< false (unless spec. Txt)
	mDefSetupMemb(File::ViewStyle,examstyle)//!< File::Text (Bin if Img)
	mDefSetupMemb(bool,exameditable)	//!< false
	mDefSetupMemb(bool,displaylocalpath)	//!< false
	mDefSetupMemb(bool,checkable)		//!< false
	mDefSetupMemb(ContentType,contenttype)	//!< General
	mDefSetupMemb(uiString,objtype)		//!< empty
    };

			uiFileSel(uiParent*,const uiString& seltxt,
				  const char* fnm=nullptr);
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
    void		setObjType( const uiString& s )		{ objtype_ = s;}
    void		setLabelText(const uiString&);
    void		setSensitive(bool yn);
    void		setChecked(bool);

    const char*		protocol() const;
    const char*		fileName() const;	//!< the first if multiple
    void		getFileNames(BufferStringSet&) const;
    BufferString	selectedExtension() const;
    BufferString	selectedProtocol() const;
    bool		isChecked() const;
    const IOPar&	getPars() const			{ return filepars_; }

    Notifier<uiFileSel>	newSelection;
    Notifier<uiFileSel>	acceptReq;
    Notifier<uiFileSel>	checked;
    Notifier<uiFileSel>	protocolChanged;

    void		selectFile( CallBacker* cb )	{ doSelCB(cb); }
    uiButton*		selectButton()			{ return selbut_; }

protected:

    Setup		setup_;
    uiString		objtype_;
    BufferStringSet	factnms_;
    IOPar		filepars_;

    uiCheckBox*		checkbox_		= nullptr;
    uiComboBox*		protfld_		= nullptr;
    uiLabel*		lbl_			= nullptr;
    uiLineEdit*		fnmfld_;
    uiButton*		selbut_;
    uiButton*		examinebut_		= nullptr;

    void		doSelCB(CallBacker*);
    void		inputChgCB(CallBacker*);
    void		fnmEnteredCB(CallBacker*);
    void		checkCB(CallBacker*);
    void		examineFileCB(CallBacker*);
    void		finalizeCB(CallBacker*);
    void		protChgCB(CallBacker*);

    const OD::FileSystemAccess& fsAccess(int) const;
    void		setButtonStates();
    const char*		protKy(int) const;
    const char*		curProtKy() const;
    void		setSel(const BufferStringSet&);

private:

    void		init(const uiString&);

public:

			// raw, unchecked stuff
    const char*		text() const;
    void		setText(const char*);

};
