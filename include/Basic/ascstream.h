#ifndef ascstream_H
#define ascstream_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id: ascstream.h,v 1.6 2002-12-27 16:15:17 bert Exp $
________________________________________________________________________

-*/

#include <bufstring.h>
#include <limits.h>
#include <iosfwd>

class MeasureUnit;

#define mParagraphMarker	"!"
#define mDefKeyValSep		':'
#define mMaxWordLength		1023
#define	mMaxFileHeadLength	80
#define	mMaxLineLength		(USHRT_MAX-1)


typedef char AscStreamHeader[mMaxFileHeadLength+1];


/*!\brief dGB standard ascii format file writing.

An ascostream puts data in a dGB standard ascii format file. That means
it has a dGB standard header and often keyword/value pairs separated by
a colon.

*/

class ascostream
{

public:
		ascostream(ostream& strm,char kvsep=mDefKeyValSep)
			: mystrm(NO), keyvalsep(kvsep)	{ init(&strm); }
		ascostream(ostream* strm,char kvsep=mDefKeyValSep)
			: mystrm(YES), keyvalsep(kvsep)	{ init(strm); }
		~ascostream();

    bool	put(const char*,const char* val=0);
    bool	put(const char*,int);
    bool	put(const char*,float,const MeasureUnit* mu=0);
    bool	put(const char*,double,const MeasureUnit* mu=0);
    bool	putYN(const char*,bool);
    bool	putHeader(const char*,const char* pspec=0);

    void	tabsOn()		{ tabs = YES; }
    void	tabsOff()		{ tabs = NO; }

    void	newParagraph();
    void	putKeyword(const char*);
    ostream&	stream() const		{ return *streamptr; }
		operator ostream&()	{ return *streamptr; }

protected:

    ostream*	streamptr;
    bool	mystrm;
    bool	tabs;
    char	keyvalsep;

private:

    void	init( ostream* strmptr )
		{ streamptr = strmptr; tabs = NO; }

};


/*!\brief reading dGB standard ascii format file.

An ascistream gets data from a dGB standard ascii format file. This format
consists of the dGB header (version, file type, date), and then a number of
'paragraphs', each separated by a single '!' on a line.

The max size of the value is limited by the line length maximum of
USHRT_MAX-1, i.e. 65534. If word parsing is used, the limit is 1023 per word.

*/

class ascistream
{
public:
			ascistream(istream& strm,int rdhead=YES,
				   char kvsep=mDefKeyValSep)
				: mystrm(NO)
				, keybuf("",mMaxWordLength)
				, valbuf("",mMaxWordLength)
				, keyvalsep(kvsep)
				{ init(&strm,rdhead); }
			ascistream(istream* strm,int rdhead=YES,
				   char kvsep=mDefKeyValSep)
				: mystrm(YES)
				, keybuf("",mMaxWordLength)
				, valbuf("",mMaxWordLength)
				, keyvalsep(kvsep)
				{ init(strm,rdhead); }
			~ascistream();

    ascistream&		next();

    bool		isOfFileType(const char*) const;
    const char*		fileType() const
			{ return filetype; }
    const char*		version() const;
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
    double		getValue(const MeasureUnit* mu=0) const;
    bool		getYN() const;

    inline istream&	stream() const		{ return *streamptr; }

    const char*		keyWord() const		{ return keybuf; }
    const char*		value() const		{ return valbuf; }

			// This is for overriding what's in the file
    void		setKeyWord( const char* s ) { keybuf = s; }
    void		setValue( const char* s ) { valbuf = s; }
    void		setTabbed( bool yn ) 	{ tabbed = yn; }

protected:

    istream*		streamptr;
    bool		mystrm;
    bool		tabbed;
    BufferString	keybuf;
    BufferString	valbuf;
    char		keyvalsep;

    void		resetPtrs(bool);

    const char*		nextwordptr;
    char		curword[mMaxWordLength+1];

    AscStreamHeader	filetype, header, timestamp;

private:

    void		init(istream*,int);

};


inline bool atEndOfSection( const ascistream& strm )
{ return strm.atEOS(); }


#endif
