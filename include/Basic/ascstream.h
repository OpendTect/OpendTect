#ifndef ascstream_H
#define ascstream_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id: ascstream.h,v 1.3 2000-04-17 14:53:42 bert Exp $
________________________________________________________________________

 An asc stream gets/puts data in/from a dGB standard ascii format file.

 For input, the maximum size of the keyword is 1023, the max size of the
 value is limited by the line length maximum of USHRT_MAX-1, i.e. 65534.
 If word parsing is used, the limit is 1023 per word.

-*/

#include <bufstring.h>
#include <limits.h>
class MeasureUnit;
class istream;
class ostream;

#define mParagraphMarker	"!"
#define mKeyValSepar		':'
#define mMaxWordLength		1023
#define	mMaxFileHeadLength	80
#define	mMaxLineLength		(USHRT_MAX-1)


typedef char AscStreamHeader[mMaxFileHeadLength+1];


class ascostream
{

public:
		ascostream(ostream& strm)
			: mystrm(NO)	{ init(&strm); }
		ascostream(ostream* strm)
			: mystrm(YES)	{ init(strm); }
		~ascostream();

    void	setColonPos( int pos )	{ colonpos = pos; }
    bool	put(const char*,const char* val=0);
    bool	put(const char*,int);
    bool	put(const char*,float,const MeasureUnit* mu=0);
    bool	put(const char*,double,const MeasureUnit* mu=0);
    bool	putYN(const char*,bool);
    bool	putHeader(const char*,const char* pspec=0);

    void	tabsOn()		{ tabs = YES; }
    void	tabsOff()		{ tabs = NO; }
    void	paddingOn()		{ pad = YES; }
    void	paddingOff()		{ pad = NO; }

    void	newParagraph();
    void	putKeyword(const char*);
    ostream&	stream() const		{ return *streamptr; }
		operator ostream&()	{ return *streamptr; }

protected:

    ostream*	streamptr;
    int		colonpos;
    bool	mystrm;
    bool	tabs;
    bool	pad;

private:

    void	init( ostream* strmptr )
		{
		    streamptr = strmptr; tabs = NO; pad = YES;
		    colonpos = 25;
		}

};


class ascistream
{
public:
			ascistream(istream& strm,int rdhead=YES)
				: mystrm(NO)
				, keybuf("",mMaxWordLength)
				, valbuf("",mMaxWordLength)
				{ init(&strm,rdhead); }
			ascistream(istream* strm,int rdhead=YES)
				: mystrm(YES)
				, keybuf("",mMaxWordLength)
				, valbuf("",mMaxWordLength)
				{ init(strm,rdhead); }
			~ascistream();

    ascistream&		next();

    bool		isOfFileType(const char*) const;
    const char*		fileType() const
			{ return filetype; }
    const char*		version() const;
    const char*		nextWord();
    void		toFirstWord()
			{ curword[0] = '\0'; nextwordptr = valbuf; }

    enum EntryType	{ Empty, Keyword, KeyVal, ParagraphMark, EndOfFile };
    EntryType		type() const;
    bool		atEOS() const
			{ return type() > KeyVal; }
    bool		isTabbed() const	{ return tabbed; }
    bool		hasKeyword(const char*) const;
    bool		hasValue(const char*) const;
    int			getVal() const;
    double		getValue(const MeasureUnit* mu=0) const;
    bool		getYN() const;

    inline istream&	stream() const		{ return *streamptr; }

    const char*		keyWord() const		{ return keybuf; }
    const char*		value() const		{ return valbuf; }

    void		setKeyWord( const char* s ) { keybuf = s; }
    void		setValue( const char* s ) { valbuf = s; }
    void		setTabbed( bool yn ) 	{ tabbed = yn; }

protected:

    istream*		streamptr;
    bool		mystrm;
    bool		tabbed;
    BufferString	keybuf;
    BufferString	valbuf;

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
