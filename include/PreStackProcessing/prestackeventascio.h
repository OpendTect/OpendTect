#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		November 2008
________________________________________________________________________

-*/

#include "prestackprocessingmod.h"

#include "binidvalset.h"
#include "prestackevents.h"
#include "tableascio.h"
#include "task.h"
#include "trckeysampling.h"

namespace Table { class FormatDesc; }

namespace PreStack
{

class EventManager;

/*!
\brief Outputs an ascii string with all PreStack event, each pick on one row.
The columns are as follows:

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

mExpClass(PreStackProcessing) EventExporter : public SequentialTask
{ mODTextTranslationClass(EventExporter);
public:
    			EventExporter(od_ostream& strm,EventManager&);
    			~EventExporter();

    void		setHRange(const TrcKeySampling&);

    od_int64		nrDone() const	{ return nrdone_; }
    od_int64		totalNr() const	{ return locations_.totalSize(); }

    uiString		uiMessage() const	{ return message_; }
    int			nextStep();
    uiString		uiNrDoneText() const;

protected:

    od_ostream&			strm_;
    EventManager&		events_;
    TrcKeySampling		tks_;

    BinIDValueSet		locations_;
    BinIDValueSet::SPos		pos_;

    int				nrdone_;
    int				fileidx_;
    uiString			message_;
};


/*!
\brief Ascii I/O for PreStack event. 
*/

mExpClass(PreStackProcessing) EventAscIO : public Table::AscIO
{ mODTextTranslationClass(EventAscIO);
public:
				EventAscIO(const Table::FormatDesc&,
					   od_istream&);

    static Table::FormatDesc*   getDesc();
    static void			updateDesc(Table::FormatDesc&);
    static void                 createDescBody(Table::FormatDesc*);

    bool			isXY() const;
    int				getNextLine(BinID& bid,int& horid,
	    				    float& off,float& val);

protected:

    od_istream&			strm_;
    float			udfval_;
    bool			isxy_;
    bool			finishedreadingheader_;
};


/*!
\brief PreStack event importer.
*/

mExpClass(PreStackProcessing) EventImporter : public SequentialTask
{ mODTextTranslationClass(EventImporter);
public:
    			EventImporter(const char*,const Table::FormatDesc&,
				      EventManager&);
    			~EventImporter();

    od_int64		nrDone() const;
    od_int64		totalNr() const		{ return totalnr_; }

    uiString		uiMessage() const	{ return message_; }
    int			nextStep();
    uiString		uiNrDoneText() const;

protected:

    od_istream&			strm_;
    EventAscIO*			ascio_;
    Event*			event_;
    EventManager&		evmgr_;

    uiString			message_;
    od_int64			totalnr_;
    BinID			lastbid_;
    int				lasthorid_;
};

} // namespace PreStack

