#ifndef strmprov_H
#define strmprov_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		17-5-1995
 Contents:	Generalized stream opener.
 RCS:		$Id: strmprov.h,v 1.1 2000-03-02 15:24:35 bert Exp $
________________________________________________________________________

 Class will provide input or output stream for disk or tape file or system
 command.

@$*/
 
 
#include <idobj.h>
#include <fixstring.h>
#include <conn.h>


/*$@ StreamProvider
 provides a stream with requested source attached:
 - starting with '@' --> OS command that produces the data on stdin/stdout
 - having a ':' --> remote machinename preceding (e.g. dgb1:/dev/exabyte)
 - tape devices are assumed to be on /dev
 Thus:
 - dgb1:/dev/exabyte
	tape device on remote host dgb1
 - dgb1:@handle_data
	Executable handle_data on remote host dgb1 will get/put on stdin/stdout
 - dgb1:/foo/bar
	File /foo/bar on remote host dgb1
 - foo.bar
	file foo.bar on current directory

 A null string or the strings "stdin" and "stdout" will get std input/output.
@$*/


class StreamProvider : public IDObject
{			isConcreteClass
public:
		StreamProvider(const char* nm=0);
		StreamProvider(const char*,const char*,StreamConn::Type);
    void	set(const char*);

    int		skipFiles(int) const;
    int		rewind() const;
    int		offline() const;
    int		bad() const				{ return isbad; }

    int		exists(int forread) const;
    int		remove() const;

    StreamData	makeOStream() const;
    StreamData	makeIStream() const;

    const char*	fullName() const;
    const char*	hostName() const			{ return hostname; }
    const char*	fileName() const			{ return fname; }
    const char*	command() const				{ return fname; }
    int		blockSize() const			{ return blocksize; }

    void	setHostName( const char* hname )	{ hostname = hname; }
    void	setFileName( const char* fn )		{ fname = fn; }
    void	setCommand( const char* fn )		{ fname = fn; }
    void	setBlockSize( long bs )			{ blocksize = bs; }
    void	addPathIfNecessary(const char*);

    StreamConn::Type	type()				{ return type_; }
    int			isNormalFile() const;

protected:
    FixedString<256>	fname;
    FixedString<32>	hostname;

    long		blocksize;
    bool		isbad;
    StreamConn::Type	type_;

};


/*$-*/
#endif
