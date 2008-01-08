#ifndef ascstream_H
#define ascstream_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id: ascstream.h,v 1.15 2008-01-08 11:53:52 cvsbert Exp $
________________________________________________________________________

-*/

#include <bufstring.h>
#include <limits.h>
#include <iosfwd>

#define mAscStrmParagraphMarker		"!"
#define mAscStrmDefKeyValSep		':'


/*!\brief OpendTect standard ascii format file writing.

An ascostream puts data in an OpendTect standard ascii format file. That means
it has a OpendTect standard header and often keyword/value pairs separated by
a colon.

*/

class ascostream
{

public:
		ascostream(std::ostream& strm,char kvsep=mAscStrmDefKeyValSep)
			: mystrm(false), keyvalsep(kvsep), streamptr(&strm) {}
		ascostream(std::ostream* strm,char kvsep=mAscStrmDefKeyValSep)
			: mystrm(true), keyvalsep(kvsep), streamptr(strm)   {}
					//!<\note strm becomes mine
		~ascostream();

    bool	put(const char*,const char* val=0);
    bool	put(const char*,int);
    bool	put(const char*,float);
    bool	put(const char*,double);
    bool	putYN(const char*,bool);
    bool	putHeader(const char*,const char* pspec=0);

    void	newParagraph();
    void	putKeyword(const char*);
    std::ostream& stream() const		{ return *streamptr; }
		operator std::ostream&()	{ return *streamptr; }

protected:

    std::ostream* streamptr;
    bool	mystrm;
    const char	keyvalsep;

};


/*!\brief reading OpendTect standard ascii format file.

An ascistream gets data from a OpendTect standard ascii format file. This format
consists of the OpendTect header (version, file type, date), and then a number
of 'paragraphs', each separated by a single '!' on a line.

*/

class ascistream
{
public:
			ascistream(std::istream& strm,bool rdhead=true,
				   char kvsep=mAscStrmDefKeyValSep)
				: mystrm(false)
				, keyvalsep(kvsep)
				{ init(&strm,rdhead); }
			ascistream(std::istream* strm,bool rdhead=true,
				   char kvsep=mAscStrmDefKeyValSep)
				: mystrm(true)
				, keyvalsep(kvsep)
				{ init(strm,rdhead); }
			~ascistream();

    ascistream&		next();

    const char*		projName() const;	//!< Usually 'dTect'
    bool		isOfFileType(const char*) const;
    const char*		fileType() const	{ return filetype.buf(); }
    const char*		version() const;
    const char*		timeStamp() const	{ return timestamp.buf(); }
    int			majorVersion() const;
    int			minorVersion() const;

    enum EntryType	{ Empty, Keyword, KeyVal, ParagraphMark, EndOfFile };
    EntryType		type() const;
    bool		atEOS() const		{ return type() > KeyVal; }
			//!< returns true if at end of segment (='paragraph')
    bool		hasKeyword(const char*) const;
    bool		hasValue(const char*) const;
    int			getVal() const;
    double		getValue() const;
    bool		getYN() const;

    inline std::istream& stream() const		{ return *streamptr; }

    const char*		keyWord() const		{ return keybuf.buf(); }
    const char*		value() const		{ return valbuf.buf(); }

			// This is for overriding what's in the file
    void		setKeyWord( const char* s ) { keybuf = s; }
    void		setValue( const char* s ) { valbuf = s; }

protected:

    std::istream*	streamptr;
    bool		mystrm;
    BufferString	keybuf;
    BufferString	valbuf;
    const char		keyvalsep;

    BufferString	filetype;
    BufferString	header;
    BufferString	timestamp;

private:

    void		init(std::istream*,bool);

};


inline bool atEndOfSection( const ascistream& strm )
{ return strm.atEOS(); }


#endif
