#ifndef wellinfocollecter_h
#define wellinfocollecter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: wellextractdata.h,v 1.1 2004-05-04 09:14:47 bert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"

class MultiID;
class IODirEntryList;


namespace Well
{
class Info;
class Marker;

/*!\brief Collects info about all wells in store */

class InfoCollector : public ::Executor
{
public:

			InfoCollector(bool wellloginfo=true,bool markers=true);
			~InfoCollector();

    int			nextStep();
    const char*		message() const		{ return curmsg_.buf(); }
    const char*		nrDoneText() const	{ return "Wells inspected"; }
    int			nrDone() const		{ return curidx_; }
    int			totalNr() const		{ return totalnr_; }

    typedef ObjectSet<Marker>	MarkerSet;

    const ObjectSet<MultiID>&	ids() const	{ return ids_; }
    const ObjectSet<Info>&	infos() const	{ return infos_; }
    				//!< Same size as ids()
    const ObjectSet<MarkerSet>&	markers() const	{ return markers_; }
    				//!< If selected, same size as ids()
    const ObjectSet<BufferStringSet>& logs() const { return logs_; }
    				//!< If selected, same size as ids()


protected:

    ObjectSet<MultiID>		ids_;
    ObjectSet<Info>		infos_;
    ObjectSet<MarkerSet>	markers_;
    ObjectSet<BufferStringSet>	logs_;
    IODirEntryList*		direntries_;
    int				totalnr_;
    int				curidx_;
    BufferString		curmsg_;
    bool			domrkrs_;
    bool			dologs_;

};


}; // namespace Well


#endif
