#ifndef ascstream_H
#define ascstream_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id: ascstream.h,v 1.14 2007-09-26 11:16:04 cvsbert Exp $
________________________________________________________________________

-*/

#include <bufstring.h>
#include <limits.h>
#include <iosfwd>

#define mAscStrmParagraphMarker		"!"
#define mAscStrmDefKeyValSep		':'
#define mAscStrmMaxWordLength		1023
#define	mAscStrmMaxFileHeadLength	80
#define	mAscStrmMaxLineLength		(USHRT_MAX-1)


typedef char AscStreamHeader[mAscStrmMaxFileHeadLength+1];


/*!\brief OpendTect standard ascii format file writing.

An ascostream puts data in an OpendTect standard ascii format file. That means
it has a OpendTect standard header and often keyword/value pairs separated by
a colon.

*/

class ascostream
{

public:
		ascostream(std::ostream& strm,char kvsep=mAscStrmDefKeyValSep)
			: mystrm(false), keyvalsep(kvsep) { init(&strm); }
		ascostream(std::ostream* strm,char kvsep=mAscStrmDefKeyValSep)
			: mystrm(true), keyvalsep(kvsep)  { init(strm); }
		//!<\note stream becomes mine
		~ascostream();

    bool	put(const char*,const char* val=0);
    bool	put(const char*,int);
    bool	put(const char*,float);
    bool	put(const char*,double);
    bool	putYN(const char*,bool);
    bool	putHeader(const char*,const char* pspec=0);

    void	tabsOn()		{ tabs = true; }
    void	tabsOff()		{ tabs = false; }

    void	newParagraph();
    void	putKeyword(const char*);
    std::ostream& stream() const		{ return *streamptr; }
		operator std::ostream&()	{ return *streamptr; }

protected:

    std::ostream* streamptr;
    bool	mystrm;
    bool	tabs;
    char	keyvalsep;

private:

    void	init( std::ostream* strmptr )
		{ streamptr = strmptr; tabs = false; }

};


/*!\brief reading OpendTect standard ascii format file.

An ascistream gets data from a OpendTect standard ascii format file. This format
consists of the OpendTect header (version, file type, date), and then a number
of 'paragraphs', each separated by a single '!' on a line.

The max size of the value is limited by the line length maximum of
USHRT_MAX-1, i.e. 65534. If word parsing is used, the limit is 1023 per word.

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

    const char*		projName() const; //!< Usually 'dTect'
    bool		isOfFileType(const char*) const;
    const char*		fileType() const
			{ return filetype; }
    const char*		version() const;
    int			majorVersion() const;
    int			minorVersion() const;
    const char*		nextWord();
			//!< 'parsing' of the Value string.
    void		toFirstWord()
			{ curword[0] = '\0'; nextwordptr = valbuf; }

    enum EntryType	{ Empty, Keyword, KeyVal, ParagraphMark, EndOfFile };
    EntryType		type() const;
    bool		atEOS() const
			{ return type() > KeyVal; }
			//!< returns true if at end of segment (='paragraph')
    bool		isTabbed() const	{ return tabbed; }
			//!< returns whether the line started with a tab
    bool		hasKeyword(const char*) const;
    bool		hasValue(const char*) const;
    int			getVal() const;
    double		getValue() const;
    bool		getYN() const;

    inline std::istream& stream() const		{ return *streamptr; }

    const char*		keyWord() const		{ return keybuf; }
    const char*		value() const		{ return valbuf; }

			// This is for overriding what's in the file
    void		setKeyWord( const char* s ) { keybuf = s; }
    void		setValue( const char* s ) { valbuf = s; }
    void		setTabbed( bool yn ) 	{ tabbed = yn; }

protected:

    std::istream*	streamptr;
    bool		mystrm;
    bool		tabbed;
    BufferString	keybuf;
    BufferString	valbuf;
    char		keyvalsep;

    void		resetPtrs(bool);

    const char*		nextwordptr;
    char		curword[mAscStrmMaxWordLength+1];

    AscStreamHeader	filetype, header, timestamp;

private:

    void		init(std::istream*,bool);

};


inline bool atEndOfSection( const ascistream& strm )
{ return strm.atEOS(); }


#endif
