#ifndef odgooglewritexml_h
#define odgooglewritexml_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id: googlexmlwriter.h,v 1.3 2009-11-11 14:08:16 cvsbert Exp $
-*/

#include "bufstring.h"
class StreamData;
class Coord;

namespace ODGoogle
{
class XMLItem;

class XMLWriter
{
public:

			XMLWriter(const char* elemname,const char* fnm=0,
				  const char* survnm=0);
			~XMLWriter()		{ close(); }

    bool		isOK() const;
    const char*		errMsg() const		{ return errmsg_.buf(); }

    void		setElemName( const char* nm ) //!< before open()
						{ elemnm_ = nm; }
    void		setSurveyName( const char* nm ) //!< before open()
						{ survnm_ = nm; }

    bool		open(const char* fnm);
    void		close();

    void		start(const XMLItem&);
    void		finish(const XMLItem&);

    std::ostream&	strm();
    const std::ostream&	strm() const
    			{ return const_cast<XMLWriter*>(this)->strm(); }

    void		writeIconStyles(const char* iconnm,
					const char* ins=0);
    void		writePlaceMark(const char* iconnm,const Coord&,
				       const char* nm);

protected:

    BufferString	elemnm_;
    BufferString	survnm_;
    StreamData&		sd_;
    BufferString	errmsg_;

};


} // namespace ODGoogle

#endif
