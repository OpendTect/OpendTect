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

mFDQtclass( QIODevice );


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
				qstreambuf(QIODevice&);
				~qstreambuf();

    virtual std::streamsize	showmanyc();
    virtual std::streamsize	xsgetn(char_type*,std::streamsize);
    virtual std::streamsize	xsputn(const char_type*,std::streamsize);

private:
    QIODevice*	iodevice_;
};

#endif
