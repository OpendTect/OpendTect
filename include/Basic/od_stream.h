#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"

#include "bufstring.h"
#include "strmdata.h"
#include "uistring.h"
#include "od_iosfwd.h"
#include <iosfwd>

class FilePath;
namespace OS { class MachineCommand; }



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

Note the usage of the stream checking functions. There are only two left:

- isBad() - indicates a fatal, non-recoverable error.
- isOK() - indicates whether the stream is ready for new read or write.

The difference is with end-of-file when reading. eof will not give you isBad(),
but it will make the stream !isOK(). There is no difference for write.

Therefore, rule of thumb is: use isOK() for loop control, isBad() as a return
value. If you know that the file should contain more data than the last read,
then isOK() will be your choice anyway.

 */


mExpClass(Basic) od_stream
{ mODTextTranslationClass(od_stream);
public:

    typedef od_stream_Count	Count;
    typedef od_stream_Pos	Pos;

    virtual			~od_stream();

    bool			isOK() const;	//!< eof is not OK
    bool			isBad() const;	//!< eof is not Bad

    uiString			errMsg() const;
				//!< see also below.
    bool			forRead() const;
    bool			forWrite() const;
    bool			isLocal() const;

    enum Ref			{ Abs, Rel, End };
    Pos				position() const;

    mDeprecated			("Use setReadPosition/setWritePosition")
    void			setPosition(Pos,Ref r=Abs);

    const char*			fileName() const;
    void			setFileName(const char*);

    inline StreamData&		streamData()			{ return sd_; }
    inline const StreamData&	streamData() const		{ return sd_; }

    void			setNoClose( bool yn=true )	{ noclose_=yn; }
    void			close();

    void			addErrMsgTo(BufferString&) const;
    void			addErrMsgTo(uiString&) const;
    void			addErrMsgTo(uiRetVal&) const;

    mDeprecated			("Use the one with uiString&")
    static od_stream*		create(const char*,bool forread,
					BufferString& errmsg);
				//!< returns null on failure, never a bad stream
    static od_stream*		create(const char*,bool forread,
					   uiString& errmsg);
				//!< returns null on failure, never a bad stream
    static const char*		sStdIO();
				//!< pass this as filename to get cin or cout
    static const char*		sStdErr();
				//!< pass this as filename to get cerr

protected:

			od_stream();
			od_stream(const char*,bool,bool editmode=false);
			od_stream(const FilePath&,bool,bool editmode=false);
			od_stream(const OS::MachineCommand&,const char* workdir,
				  bool editmode=false);
			od_stream(std::ostream*);
			od_stream(std::ostream&);
			od_stream(std::istream*);
			od_stream(std::istream&);
    od_stream&		operator=(const od_stream&)		= delete;

    StreamData		sd_;
    bool		mine_		= true;
    bool		noclose_	= false;
    mutable uiString	errmsg_;

    BufferString	noStdStreamPErrMsg() const;

private:

    bool		setFromCommand(const OS::MachineCommand&,
				       const char* workdir,bool editmode);

};
