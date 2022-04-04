#ifndef qstreambuf_h
#define qstreambuf_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
________________________________________________________________________

*/

#include "basicmod.h"
#include "commondefs.h"
#include <streambuf>
#include <vector>
#include <istream>

#include <QSharedPointer>

mFDQtclass( QIODevice );
mFDQtclass( QProcess );


/*!
\brief Adapter to use a qprocess as a stream.

  Usage like:

  qstreambuf fsb( QIODevice& );
  std::istream strm( &fsb );

  Tested for istream with char* only.
*/

mClass(Basic) qstreambuf : public std::streambuf
{
public:
				qstreambuf(QIODevice&,bool isstderr,
					   bool takeoverdevice);
				~qstreambuf();

    void			detachDevice(bool readall);
				/*!<Don't use device after this call.
				    \param readall true if device should be
					   emptied for all pending messages
					   before.
				*/

    std::streambuf::int_type	underflow() override;
    std::streamsize		xsputn(const char_type*,
				       std::streamsize) override;

    int				sync() override;

private:
    void			readAll();

    std::vector<char>		buffer_;
    bool			ownsdevice_;

    QByteArray			readbuffer_;
    QProcess*			process_;
    QIODevice*			iodevice_;
    bool			isstderr_;
};

//!Does everything a std::istream does, but also deletes the streambuf
mClass(Basic) iqstream : public std::istream
{
public:
			iqstream( std::streambuf* buf )
			    : std::istream( buf )
			{}
			~iqstream()
			{ delete rdbuf(); }
};


//!Does everything a std::ostream does, but also deletes the streambuf
mClass(Basic) oqstream : public std::ostream
{
public:
			oqstream( std::streambuf* buf )
			    : std::ostream( buf )
			{}
			~oqstream()
			{ delete rdbuf(); }
};

#endif
