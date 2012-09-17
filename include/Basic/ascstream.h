#ifndef ascstream_h
#define ascstream_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id: ascstream.h,v 1.21 2009/07/22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "plftypes.h"
#include <limits.h>
#include <iosfwd>

#define mAscStrmParagraphMarker		"!"
#define mAscStrmKeyValSep		':'


/*!\brief OpendTect standard ascii format file writing.

An ascostream puts data in an OpendTect standard ascii format file. That means
it has a OpendTect standard header and often keyword/value pairs separated by
a colon.

*/

mClass ascostream
{

public:
		ascostream( std::ostream& strm )
			: mystrm(false), streamptr(&strm) {}
		ascostream( std::ostream* strm )
			: mystrm(true), streamptr(strm)   {}
					//!<\note strm becomes mine
		~ascostream();

    bool	putHeader(const char* filetype);
    bool	put(const char*,const char* val=0);

#define mAscStreamDefFns(fn,typ) \
    bool	fn(const char*,typ); \
    bool	fn(const char*,typ,typ); \
    bool	fn(const char*,typ,typ,typ); \
    bool	fn(const char*,typ,typ,typ,typ)
    		mAscStreamDefFns(put,int);
		mAscStreamDefFns(put,od_uint32);
		mAscStreamDefFns(put,od_int64);
		mAscStreamDefFns(put,od_uint64);
		mAscStreamDefFns(put,float);
		mAscStreamDefFns(put,double);
		mAscStreamDefFns(putYN,bool);
#undef mAscStreamDefFns

    void	newParagraph();

    std::ostream& stream()			{ return *streamptr; }
    const std::ostream& stream() const		{ return *streamptr; }
    operator	std::ostream&()			{ return *streamptr; }
    operator	const std::ostream&() const	{ return *streamptr; }

protected:

    std::ostream* streamptr;
    bool	mystrm;

    void	putKeyword(const char*,bool wsep=true);

};


/*!\brief reading OpendTect standard ascii format file.

An ascistream gets data from a OpendTect standard ascii format file. This format
consists of the OpendTect header (version, file type, date), and then a number
of 'paragraphs', each separated by a single '!' on a line.

*/

mClass ascistream
{
public:
			ascistream( std::istream& strm, bool rdhead=true )
				: mystrm(false)	{ init(&strm,rdhead); }
			ascistream( std::istream* strm, bool rdhead=true )
				: mystrm(true)	{ init(strm,rdhead); }
			~ascistream();

    ascistream&		next();

    const char*		headerStartLine() const	{ return header.buf(); }
    const char*		fileType() const	{ return filetype.buf(); }
    bool		isOfFileType(const char*) const;
    const char*		version() const;
    const char*		timeStamp() const	{ return timestamp.buf(); }
    int			majorVersion() const;
    int			minorVersion() const;

    enum EntryType	{ Empty, Keyword, KeyVal, ParagraphMark, EndOfFile };
    EntryType		type() const;
    bool		atEOS() const		{ return type() > KeyVal; }
			//!< returns true if at end of segment (='paragraph')

    const char*		keyWord() const		{ return keybuf.buf(); }
    const char*		value() const		{ return valbuf.buf(); }
    bool		hasKeyword(const char*) const;
    bool		hasValue(const char*) const;
    int			getIValue(int i=0) const;
    od_uint32		getUIValue(int i=0) const;
    od_int64		getI64Value(int i=0) const;
    od_uint64		getUI64Value(int i=0) const;
    float		getFValue(int i=0) const;
    double		getDValue(int i=0) const;
    bool		getYN(int i=0) const;

    inline std::istream& stream() const		{ return *streamptr; }

			// This is for overriding what's in the file
    void		setKeyWord( const char* s ) { keybuf = s; }
    void		setValue( const char* s ) { valbuf = s; }

protected:

    std::istream*	streamptr;
    bool		mystrm;
    BufferString	keybuf;
    BufferString	valbuf;

    BufferString	header;
    BufferString	filetype;
    BufferString	timestamp;

private:

    void		init(std::istream*,bool);

};


inline bool atEndOfSection( const ascistream& strm )
{ return strm.atEOS(); }


#endif
