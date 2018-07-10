#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2017
________________________________________________________________________

-*/

#include "bufstringset.h"
#include "uistring.h"


namespace File
{

/*!\brief A file format description for simple selection of files.

  Extensions are treated case insensitive, but if you add different versions
  with different case the exact match will be preferred.

*/

mExpClass(Basic) Format
{ mODTextTranslationClass(File::Format)
public:

			Format(const uiString&,const char* ext=0,
				const char*ext2=0,const char* ext3=0);
			Format(const char* traditional_img_fmt_str);

    const uiString&	userDesc() const		{ return usrdesc_; }
    int			nrExtensions() const		{ return exts_.size(); }
    const char*		extension(int) const;
    bool		hasExtension(const char*) const;

    void		addExtension(const char*);

    static const Format& allFiles();
    static const Format& textFiles();
    static const Format& parFiles();
    static const Format& zipFiles();
    static const Format& hdf5Files();
    static const Format& shlibFiles();
    static const Format& imageFiles();

    BufferString	getFileFilter() const;

protected:

    uiString		usrdesc_;
    BufferStringSet	exts_;

    friend class	FormatList;

};


/*!\brief A list of file formats for simple selection of files */


mExpClass(Basic) FormatList
{ mIsContainer( FormatList, ObjectSet<Format>, fmts_ )
public:

			FormatList()		{}
			FormatList(const Format&);
			FormatList(const char* traditional_img_fmt_str);
			FormatList(const uiString&,const char* ext,
				   const char*ext2=0,const char* ext3=0);
			FormatList(const FormatList&);
			~FormatList()		{ setEmpty(); }
    FormatList&		operator =(const FormatList&);

    int			size() const		{ return fmts_.size(); }
    int			indexOf(const char* ext) const;
    bool		isPresent( const char* ext ) const
						{ return indexOf(ext) >= 0; }
    Format		format(int) const;
    uiString		userDesc(int) const;

    void		addFormat( const Format& fmt )
						{ fmts_ += new Format(fmt); }
    void		addFormat(const uiString&,const char*);
    void		addFormats(const FormatList&);
    void		setEmpty();
    void		removeFormat(int);

    BufferString	getFileFilter(int) const;
    BufferString	getFileFilters() const;

};


} // namespace File
