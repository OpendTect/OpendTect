#ifndef od_stream_h
#define od_stream_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "od_iosfwd.h"
#include <iosfwd>
class FilePath;
class StreamData;


/*!\brief OD base class for stream read/write

If the stream is owner of the std stream, then it will close it automatically
when it goes out of scope (or is deleted). That means: except when you
construct it with a std::[io]stream& . With that construction the od_stream is
merely an adaptor. In all cases, the close() function is called automatically
on destruction, but you can do it 'earlier' if you want.

As a result, copying a stream will transfer the stream to the copy, if the
od_stream is owner. So unlike StreamData, you will then not have 2 od_streams
pointing to the same std stream. In this way you can for example return an
od_stream from a function.
 
 */


mExpClass(Basic) od_stream
{
public:

    typedef od_stream_Count	Count;
    typedef od_stream_Pos	Pos;

    virtual			~od_stream();

    bool			isOK() const;
    const char*			errMsg() const; //!< see also below
    bool			forRead() const;
    bool			forWrite() const;

    enum Ref			{ Abs, Rel, End };
    Pos				position() const;
    void			setPosition(Pos,Ref r=Abs);
    Pos				endPosition() const;

    const char*			fileName() const;
    void			setFileName(const char*);

    inline StreamData&		streamData()			{ return sd_; }
    inline const StreamData&	streamData() const		{ return sd_; }

    void			setNoClose( bool yn=true )	{ noclose_=yn; }
    void			releaseStream(StreamData&);
    void			close();
    bool			isBad() const;	//!< eof is not Bad

    void			addErrMsgTo(BufferString&) const;

protected:

    			od_stream();
    			od_stream(const char*,bool);
    			od_stream(const FilePath&,bool);
    			od_stream(std::ostream*);
    			od_stream(std::ostream&);
    			od_stream(std::istream*);
    			od_stream(std::istream&);
    			od_stream(const od_stream&);
    od_stream&		operator =(const od_stream&);

    StreamData&		sd_;
    bool		mine_;
    bool		noclose_;

};



#endif
