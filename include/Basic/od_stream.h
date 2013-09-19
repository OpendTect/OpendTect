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
#include <iosfwd>
class FilePath;
class StreamData;


/*!\brief OD base class for stream read/write */

mExpClass(Basic) od_stream
{
public:

    typedef od_uint64	Count;
    typedef od_int64	Pos;

    virtual		~od_stream();

    bool		isOK() const;
    bool		isBad() const;
    const char*		errMsg() const;

    enum Ref		{ Abs, Rel, Beg, End };
    Pos			position() const;
    void		setPosition(Pos,Ref r=Abs);

    const char*		fileName() const;
    void		setFileName(const char*);

    od_int64		endPosition() const;
    bool		forWrite() const;

    inline StreamData&	streamData()		{ return sd_; }
    inline const StreamData& streamData() const	{ return sd_; }
    void		releaseStream(StreamData&);

protected:

    			od_stream(const char*,bool);
    			od_stream(const FilePath&,bool);
    			od_stream(std::ostream*);
    			od_stream(std::ostream&);
    			od_stream(std::istream*);
    			od_stream(std::istream&);


    StreamData&		sd_;
    bool		mine_;

    void		close();

private:

    			od_stream(const od_stream&);
    od_stream&		operator =(const od_stream&);

};



#endif
