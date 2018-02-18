#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/09/2000
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "bufstringset.h"
#include "uistring.h"
#include "oduicommon.h"

class uiParent;
class FileMultiString;

/*!\brief Dialog to get a file or directory name from user
*/

mExpClass(uiBase) uiFileDialog
{ mODTextTranslationClass(uiFileDialog);
public:

    typedef OD::FileSelectionMode	Mode;
    typedef OD::FileContentType		Type;

    mDeprecated		uiFileDialog(uiParent*,bool forread,
				     const char* fname=0,
				     const char* filter=0,
				     uiString caption=uiString::empty());
    mDeprecated		uiFileDialog(uiParent*,Mode mode=OD::SelectFileForWrite,
				     const char* fname=0,
				     const char* filter=0,
				     uiString caption=uiString::empty());
    mDeprecated		uiFileDialog(uiParent*,Type,
				     const char* fname=0,
				     uiString caption=uiString::empty());
						//!< Uses SelectFileForWrite

    const char*		fileName() const	{ return fn; }
    void		getFileNames(BufferStringSet&) const;

    void		setMode( Mode m )	{ mode_ = m; }
    Mode		mode() const		{ return mode_; }

    void		setSelectedFilter(const char* fltr)
			{ selectedfilter_ = fltr; }
    const char*		selectedFilter() const	{ return selectedfilter_; }

    void		setDirectory( const char* dir )
			{ currentdir_ = dir; }

    void		setAllowAllExts( bool yn )	{ addallexts_ = yn; }
			//!< default true for read, false for write
    void		setConfirmOverwrite( bool yn )
			{ confirmoverwrite_ = yn; }

    void		setDefaultExtension(const char*);
    const char*		getDefaultExtension() const;

    int			go();

    static const char*	filesep_;

    static void		joinFileNamesIntoSingleString(const BufferStringSet&,
				    BufferString&);
    static void		separateFileNamesFromSingleString(const BufferString&,
				    BufferStringSet&);

			// To be used by cmddriver to select filename(s)
			// while closing the QFileDialog
    int			processExternalFilenames(const char* dir=0,
						 const char* filters=0);
    static void		setExternalFilenames(const FileMultiString&);
    static const char*	getExternalFilenamesErrMsg();
			// Warning starts with '!'-symbol

protected:

    BufferString	fn;
    Mode		mode_;
    BufferString	fname_;
    BufferString	filter_;
    uiString		caption_;
    uiParent*		parnt_;
    BufferStringSet	filenames_;
    BufferString	selectedfilter_;
    BufferString	currentdir_;
    bool		addallexts_;
    bool		forread_;
    bool		confirmoverwrite_;
    BufferString	defaultextension_;

    static FileMultiString*	externalfilenames_;
    static BufferString		extfilenameserrmsg_;

private:

    void		setDefaultCaption();
    int			beginCmdRecEvent( const char* wintitle );
    void		endCmdRecEvent(int refnr, bool ok);

};
