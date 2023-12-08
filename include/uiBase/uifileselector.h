#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "fileformat.h"
#include "uistring.h"
#include "odcommonenums.h"

class uiParent;
class uiFileSelTool;


/*!\brief Setup for any file or directory selection process */

mExpClass(uiBase) uiFileSelectorSetup
{
public:

    typedef uiFileSelectorSetup		Setup;

    mDefSetupMemb(OD::FileSelectionMode,selmode)
    mDefSetupMemb(OD::FileContentType,	contenttype)
    mDefSetupMemb(BufferStringSet,	initialselection)
    mDefSetupMemb(BufferString,		initialselectiondir)
    mDefSetupMemb(FileFormatList,	formats)
    mDefSetupMemb(bool,			allowallextensions)
    mDefSetupMemb(BufferString,		defaultextension)
    mDefSetupMemb(BufferString,		mask)
    mDefSetupMemb(bool,			confirmoverwrite)
    mDefSetupMemb(bool,			onlylocal)
    mDefSetupMemb(bool,			skiplocal)


			uiFileSelectorSetup(const char* fnm=nullptr);
			uiFileSelectorSetup(OD::FileSelectionMode,
					   const char* fnm=nullptr);

    bool		isForRead() const
			{ return !isForWrite(); }
    bool		isForWrite() const
			{ return selmode_==OD::SelectFileForWrite; }
    bool		isForDirectory() const
			{ return selmode_==OD::SelectDirectory; }
    bool		isSingle() const
			{ return selmode_!=OD::SelectMultiFile; }
    bool		isForFile() const
			{ return !isForDirectory(); }
    Setup&		setFileName( const char* fnm )
			{
			    initialselection_.setEmpty();
			    if ( fnm && *fnm )
				initialselection_.add( fnm );
			    return *this;
			}
    Setup&		setForWrite( bool yn=true )
			{
			    selmode_ = yn ? OD::SelectFileForWrite
					  : OD::SelectFileForRead;
			    return *this;
			}
    Setup&		selectDirectory( bool yn=true )
			{
			    selmode_ = yn ? OD::SelectDirectory
					  : OD::SelectFileForRead;
			    return *this;
			}
    Setup&		selectMultiFile( bool yn=true )
			{
			    selmode_ = yn ? OD::SelectMultiFile
					  : OD::SelectFileForRead;
			    return *this;
			}
    Setup&		setInitialSelection( const char* fnm )
			{
			    initialselection_.setEmpty();
			    initialselection_.add( fnm );
			    return *this;
			}
    Setup&		setFormat( const uiString& ftype, const char* ext,
				   const char* ext2=0, const char* ext3=0 )
			{
			    formats_.setEmpty();
			    formats_.addFormat(
					FileFormat(ftype,ext,ext2,ext3) );
			    return *this;
			}
    Setup&		setFormat( const FileFormat& ffmt )
			{
			    formats_.setEmpty();
			    formats_.addFormat( ffmt );
			    return *this;
			}

private:

    void			init(const char*);

};


mExpClass(uiBase) uiFileSelector
{ mODTextTranslationClass(uiFileSelector);
public:
			uiFileSelector(uiParent*,const char* fnm=nullptr,
					bool onlylocal=false);
			uiFileSelector(uiParent*,const uiFileSelectorSetup&);
			~uiFileSelector();

    uiString&		caption()		{ return caption_; }
    uiFileSelectorSetup& setup()		{ return setup_; }

    bool		go();

    BufferString	fileName() const;
    void		getSelected(BufferStringSet&) const;

protected:

    uiParent*		parent_;
    uiFileSelectorSetup	setup_;
    uiString		caption_;

    uiFileSelTool*	seltool_			= nullptr;

};
