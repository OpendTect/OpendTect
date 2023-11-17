#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2023 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "uistring.h"


/*!\brief A file format description for simple selection of files.

  Extensions are treated case insensitive, but if you add different versions
  with different case the exact match will be preferred.

*/

mExpClass(Basic) FileFormat
{ mODTextTranslationClass(FileFormat)
public:

			FileFormat(const uiString&,const char* ext=0,
				const char*ext2=0,const char* ext3=0);
			FileFormat(const char* traditional_img_fmt_str);

    const uiString&	userDesc() const		{ return usrdesc_; }
    int			nrExtensions() const		{ return exts_.size(); }
    const char*		extension(int) const;
    bool		hasExtension(const char*) const;

    void		addExtension(const char*);

    static const FileFormat& allFiles();
    static const FileFormat& textFiles();
    static const FileFormat& parFiles();
    static const FileFormat& zipFiles();
    static const FileFormat& hdf5Files();
    static const FileFormat& shlibFiles();
    static const FileFormat& imageFiles();

    BufferString	getFileFilter() const;

protected:

    uiString		usrdesc_;
    BufferStringSet	exts_;

    friend class	FileFormatList;

};


/*!\brief A list of file formats for simple selection of files */


mExpClass(Basic) FileFormatList
{
mIsContainer( FileFormatList, ObjectSet<FileFormat>, fmts_ )
public:

			FileFormatList()		{}
			FileFormatList(const FileFormat&);
			FileFormatList(const char* traditional_img_fmt_str);
			FileFormatList(const uiString&,const char* ext,
				       const char* ext2=nullptr,
				       const char* ext3=nullptr);
			FileFormatList(const FileFormatList&);
			~FileFormatList()		{ setEmpty(); }
    FileFormatList&	operator =(const FileFormatList&);

    int			size() const		{ return fmts_.size(); }
    int			indexOf(const char* ext) const;
    bool		isPresent( const char* ext ) const
			{ return indexOf(ext) >= 0; }
    FileFormat		format(int) const;
    uiString		userDesc(int) const;

    void		addFormat( const FileFormat& fmt )
			{ fmts_ += new FileFormat(fmt); }
    void		addFormat(const uiString&,const char*);
    void		addFormats(const FileFormatList&);
    void		setEmpty();
    void		removeFormat(int);

    BufferString	getFileFilter(int) const;
    BufferString	getFileFilters() const;

};
