#ifndef qstreambuf_h
#define qstreambuf_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

*/

#include "basicmod.h"
#include "commondefs.h"
#include <streambuf>
#include <vector>

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

mExpClass(Basic) qstreambuf : public std::streambuf
{
public:
				qstreambuf(QIODevice&,bool isstderr);
				~qstreambuf();

    void			detachDevice(bool readall);
				/*!<Don't use device after this call.
				    \param readall true if device should be
					   emptied for all pending messages
					   before.
				*/

    virtual std::streambuf::int_type	underflow();
    virtual std::streamsize		xsputn(const char_type*,
					       std::streamsize);

private:
    void			readAll();

    std::vector<char>		buffer_;

    QByteArray			readbuffer_;
    QProcess*			process_;
    QIODevice*			iodevice_;
    bool			isstderr_;
};

#endif
