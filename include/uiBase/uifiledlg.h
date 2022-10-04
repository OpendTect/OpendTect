#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "bufstringset.h"
#include "uistring.h"

class uiParent;
class FileMultiString;

/*!\brief Dialog to get a file or directory name from user
*/

mExpClass(uiBase) uiFileDialog
{ mODTextTranslationClass(uiFileDialog);
public:
    //! File selection mode
    enum Mode
    {
	AnyFile,	/*!< The name of a file, whether it exists or not. */
	ExistingFile,	/*!< The name of a single existing file. */
	Directory,	/*!< The name of a directory. Both files and
			     directories displayed. */
	DirectoryOnly,	/*!< The name of a directory. The file dialog will
			     only display directories. */
	ExistingFiles	/*!< The names of zero or more existing files. */
    };


			uiFileDialog(uiParent*,bool forread,
				     const char* fname=0,
				     const char* filter=0,
			const uiString& caption=uiString::emptyString());

			uiFileDialog(uiParent*,Mode mode=AnyFile,
				     const char* fname=0,
				     const char* filter=0,
			const uiString& caption=uiString::emptyString());
    enum Type		{ Gen, Img, Txt, Html };
			uiFileDialog(uiParent*,Type,
				     const char* fname=0,
			const uiString& caption=uiString::emptyString());
						//!< Always AnyFile
    virtual		~uiFileDialog();

    virtual int		go();

    const char*		fileName() const	{ return fn; }
    void		getFileNames(BufferStringSet&) const;

    void		setMode( Mode m)	{ mode_=m; }
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

    static const char*	filesep_;

    static void		list2String(const BufferStringSet&,
				    BufferString&);
    static void		string2List(const BufferString&,
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
