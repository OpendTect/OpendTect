#ifndef ascstream_H
#define ascstream_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id: ascstream.h,v 1.2 2000-03-02 15:24:25 bert Exp $
________________________________________________________________________

 An asc stream gets/puts data in/from a standard ascii format file.

-*/

#include <gendefs.h>
class MeasureUnit;
class istream;
class ostream;

#define mParagraphMarker	"!"
#define mKeyValSepar		':'
#define	mMaxFileHeadLength	80
#define	mMaxFileKeyLength	80
#define	mMaxFileEntryLength	1023


typedef char AscStreamHeader[mMaxFileHeadLength+1];
typedef char AscStreamKeyword[256]; // allow long keywords
typedef char AscStreamValue[mMaxFileEntryLength+1];


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

private:
    ostream*	streamptr;
    int		mystrm;
    bool	tabs;
    int		pad;
    void	init( ostream* strmptr )
		{
		    streamptr = strmptr; tabs = NO; pad = YES;
		    colonpos = 25;
		}
    int		colonpos;
};


class ascistream
{
public:
			ascistream(istream& strm,int rdhead=YES)
				: mystrm(NO)
				{ init(&strm,rdhead); }
			ascistream(istream* strm,int rdhead=YES)
				: mystrm(YES)
				{ init(strm,rdhead); }
			~ascistream();

    ascistream&		next();
    ascistream&		nextLine(char*,int);

    bool			isOfFileType(const char*) const;
    const char*		fileType() const
			{ return filetype; }
    const char*		version() const;
    const char*		nextWord();
    void		toFirstWord()
			{ curword[0] = '\0'; nextwordptr = valstr; }

    enum EntryType	{ Empty, Keyword, KeyVal, ParagraphMark, EndOfFile };
    EntryType		type() const;
    bool		atEOS() const
			{ return type() > KeyVal; }
    bool		hasKeyword(const char*) const;
    bool		hasValue(const char*) const;
    int			getVal() const;
    double		getValue(const MeasureUnit* mu=0) const;
    bool		getYN() const;

    istream&		stream() const		{ return *streamptr; }

    AscStreamKeyword	keyword;
    AscStreamValue	valstr;
    bool		tabbed;

private:
    istream*		streamptr;
    bool		mystrm;

    void		init(istream*,int);
    void		resetCurrent();

    const char*		nextwordptr;
    AscStreamValue	curword;

    AscStreamHeader	filetype, header, timestamp;
};

inline bool atEndOfSection( const ascistream& strm )
{ return strm.atEOS(); }


#endif
