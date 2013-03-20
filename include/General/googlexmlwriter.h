#ifndef googlexmlwriter_h
#define googlexmlwriter_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Nov 2007
 * ID       : $Id$
-*/

#include "generalmod.h"
#include "bufstring.h"

class StreamData;
class Coord;
class LatLong;
class Color;
class SurveyInfo;
template <class T> class TypeSet;


namespace ODGoogle
{
class XMLItem;

/*!
\brief XML Writer.
*/

mExpClass(General) XMLWriter
{
public:

			XMLWriter(const char* elemname,const char* fnm=0,
				  const char* survnm=0);
			~XMLWriter()		{ close(); }

    bool		isOK() const;
    const char*		errMsg() const		{ return errmsg_.str(); }

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

    void		writeIconStyles(const char* iconnm,int xpixoffs,
					const char* ins=0);
    void		writePlaceMark(const char* iconnm,const Coord&,
				       const char* nm);
    void		writePlaceMark(const char* iconnm,const LatLong&,
				       const char* nm,const char* desc=0);
    void		writeLine(const char* iconnm,const TypeSet<Coord>&,
	    			  const char* nm);

    void		writePolyStyle(const char* stlnm,const Color&,int wdth);
    void		writePoly(const char* stlnm,const char* polynm,
				  const TypeSet<Coord>&,float hght,
				  const SurveyInfo* si=0);

protected:

    BufferString	elemnm_;
    BufferString	survnm_;
    StreamData&		sd_;
    BufferString	errmsg_;

};


} // namespace ODGoogle

#endif

