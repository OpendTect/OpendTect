#ifndef prestackeventascio_h
#define prestackeventascio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "binidvalset.h"
#include "task.h"
#include "horsampling.h"
#include "prestackevents.h"
#include "tableascio.h"

class StreamData;
namespace Table { class FormatDesc; }

namespace PreStack
{
class EventManager;

/*!Outputs an ascii string with all prestack event, each pick on one row. The
columns are as follows:

     Inline
     Crossline
     Event index (0-N).
     Dip, going to increasing inlines, If not dip is available, 0 is written
     Dip, going to increasing crosslines, If not dip is available, 0 is written
     Event quality, 0-255
     Azimuth (0-2PI)
     Offset
     Depth
     Pick quality, 0-255
*/


mClass EventExporter : public SequentialTask
{
public:
    			EventExporter(std::ostream& strm,EventManager&);
    			~EventExporter();
    void		setHRange(const HorSampling& hrg);

    od_int64		nrDone() const	{ return nrdone_; }
    od_int64		totalNr() const	{ return locations_.totalSize(); }

    const char*		message() const	{ return message_; }
    int			nextStep();
    const char*		nrDoneText() const;

protected:

    std::ostream&		strm_;
    EventManager&		events_;
    HorSampling			hrg_;

    BinIDValueSet		locations_;
    BinIDValueSet::Pos		pos_;

    int				nrdone_;
    int				fileidx_;
    const char*			message_;
};


mClass EventAscIO : public Table::AscIO
{
public:
    				EventAscIO( const Table::FormatDesc& fd,
						std::istream& strm )
				    : Table::AscIO(fd)
				    , udfval_(mUdf(float))
				    , finishedreadingheader_(false)
				    , strm_(strm)      		    {}

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&);
    static void                 createDescBody(Table::FormatDesc*);

    bool			isXY() const;
    int				getNextLine(BinID& bid,int& horid,
	    				    float& off,float& val);

protected:

    std::istream&		strm_;
    float			udfval_;
    bool			isxy_;
    bool			finishedreadingheader_;
};


mClass EventImporter : public SequentialTask
{
public:
    			EventImporter(const char*,const Table::FormatDesc&,
				      EventManager&);
    			~EventImporter();

    od_int64		nrDone() const;
    od_int64		totalNr() const		{ return totalnr_; }

    const char*		message() const	{ return message_; }
    int			nextStep();
    const char*		nrDoneText() const;

protected:

    StreamData&			sd_;
    EventAscIO*			ascio_;
    Event*			event_;
    EventManager&		evmgr_;

    const char*			message_;
    od_int64			totalnr_;
    BinID			lastbid_;
    int				lasthorid_;
};


} // namespace PreStack

#endif
