#ifndef ascstream_h
#define ascstream_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		2-5-1995
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "plftypes.h"
#include "od_iostream.h"
#include <iosfwd>

#define mAscStrmParagraphMarker		"!"
#define mAscStrmKeyValSep		':'


/*!
\brief OpendTect standard ascii format file writing.
  
  An ascostream puts data in an OpendTect standard ascii format file. That means
  it has a OpendTect standard header and often keyword/value pairs separated by
  a colon.
*/

mExpClass(Basic) ascostream
{

public:

		ascostream(od_ostream&);
		ascostream(od_ostream*); // becomes mine
		ascostream(std::ostream&);
		ascostream(std::ostream*); // becomes mine
    virtual	~ascostream();

    bool	isOK() const;

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

    inline od_ostream&	stream()		{ return strm_; }

protected:

    od_ostream& strm_;
    bool	strmmine_;

    void	putKeyword(const char*,bool wsep=true);

};


/*!
\brief OpendTect standard ascii format file reading.
  
  An ascistream gets data from a OpendTect standard ascii format file. This
  format consists of the OpendTect header (version, file type, date), and then
  a number of 'paragraphs', each separated by a single '!' on a line.
*/

mExpClass(Basic) ascistream
{
public:
			ascistream(od_istream&,bool rdhead=true);
			ascistream(od_istream*,bool rdhead=true);
			ascistream(std::istream&,bool rdhead=true);
			ascistream(std::istream*,bool rdhead=true);
    virtual		~ascistream();

    ascistream&		next();
    bool		isOK() const;

    const char*		headerStartLine() const	{ return header_.buf(); }
    bool		hasStandardHeader() const;
    const char*		fileType() const	{ return filetype_.buf(); }
    bool		isOfFileType(const char*) const;
    const char*		version() const;
    const char*		timeStamp() const	{ return timestamp_.buf(); }
    int			majorVersion() const;
    int			minorVersion() const;

    enum EntryType	{ Empty, Keyword, KeyVal, ParagraphMark, EndOfFile };
    EntryType		type() const;
    bool		atEOS() const		{ return type() > KeyVal; }
			//!< returns true if at end of segment (='paragraph')

    const char*		keyWord() const		{ return keybuf_.buf(); }
    const char*		value() const		{ return valbuf_.buf(); }
    bool		hasKeyword(const char*) const;
    bool		hasValue(const char*) const;
    int			getIValue(int i=0) const;
    od_uint32		getUIValue(int i=0) const;
    od_int64		getI64Value(int i=0) const;
    od_uint64		getUI64Value(int i=0) const;
    float		getFValue(int i=0) const;
    double		getDValue(int i=0) const;
    bool		getYN(int i=0) const;

    inline od_istream&	stream()		{ return strm_; }

			// This is for overriding what's in the file
    void		setKeyWord( const char* s ) { keybuf_ = s; }
    void		setValue( const char* s ) { valbuf_ = s; }

protected:

    od_istream&		strm_;
    bool		strmmine_;
    BufferString	keybuf_;
    BufferString	valbuf_;

    BufferString	header_;
    BufferString	filetype_;
    BufferString	timestamp_;

private:

    void		init(bool);

};


inline bool atEndOfSection( const ascistream& strm )
{ return strm.atEOS(); }


#endif

