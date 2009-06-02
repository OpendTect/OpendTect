#ifndef uifileinput_h
#define uifileinput_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          21/9/2000
 RCS:           $Id: uifileinput.h,v 1.27 2009-06-02 15:09:17 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "uifiledlg.h"

class uiPushButton;
class BufferStringSet;

/*! \brief A file-name input. 

Displays a uiLineEdit field showing the current selected file. The user can
edit the filename by hand, or pop up a file selector trough the included
"Select..." push button. Optional is an Examine button for browsing the file.

The filter you specify will automatically get an 'All files' added unless
you specify otherwise in the setup.

*/

mClass uiFileInput : public uiGenInput
{ 	
public:

    mClass Setup
    {
    public:
			Setup(const char* filenm=0)
			    : fnm(filenm)
			    , forread_(true)
			    , withexamine_(false)
			    , examinetablestyle_(false)
			    , directories_(false)
			    , allowallextensions_(true)
			    , confirmoverwrite_(true)
			    , filedlgtype_(uiFileDialog::Gen)
			    {}
			Setup(uiFileDialog::Type t,const char* filenm=0)
			    : fnm(filenm)
			    , forread_(true)
			    , withexamine_(true)
			    , examinetablestyle_(false)
			    , confirmoverwrite_(true)
			    , filedlgtype_(t)
			    {}

	BufferString	fnm;
	mDefSetupMemb(BufferString,filter)
	mDefSetupMemb(BufferString,defseldir)
	mDefSetupMemb(bool,forread)
	mDefSetupMemb(bool,withexamine)
	mDefSetupMemb(bool,examinetablestyle)
	mDefSetupMemb(bool,directories)
	mDefSetupMemb(bool,allowallextensions)
	mDefSetupMemb(bool,confirmoverwrite)
	mDefSetupMemb(uiFileDialog::Type,filedlgtype)
				//!< if not Gen, overrules some other members
	
    };

    			uiFileInput(uiParent*,const char* seltxt,
				    const char* fnm=0);
			uiFileInput(uiParent*,const char* seltxt,const Setup&);
			~uiFileInput();

    void		setFileName(const char*);
    void		setDefaultSelectionDir( const char* nm )
			{ defseldir_ = nm; }
    void		setFilter( const char* fltr )	   { filter_ = fltr; }
    const char*		selectedFilter() const		   { return selfltr_; }
    void		setSelectedFilter( const char* f ) { selfltr_ = f; }
    void		setExamine( const CallBack& cb )   { excb_ = cb; }
    			//!< Overrules the simple stand-alone file browser

    const char*		fileName() const;
    void		getFileNames(BufferStringSet&) const;

    uiFileDialog::Mode	selectMode() const		
			{ 
			    return selmodset_ ? selmode_ 
				 : (forread_  ? uiFileDialog::ExistingFile 
					      : uiFileDialog::AnyFile); 
			}

    void		setSelectMode( uiFileDialog::Mode m) 
			{ selmodset_ = true;  selmode_ = m; }

    void		enableExamine(bool);
    			//!< only if examinebut present

protected:

    bool		forread_;
    BufferString	filter_;
    BufferString	defseldir_;
    BufferString	selfltr_;
    bool		addallexts_;
    bool		tablevw_;
    bool		confirmoverwrite_;
    CallBack		excb_;

    bool		selmodset_;
    uiFileDialog::Mode  selmode_;
    uiFileDialog::Type  filedlgtype_;

    uiPushButton*	examinebut_;

    virtual void	doSelect(CallBacker*);
    void		examineFile(CallBacker*);
    void		isFinalised(CallBacker*);
};


#endif
