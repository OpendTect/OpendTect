#ifndef wellextractdata_h
#define wellextractdata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		May 2004
 RCS:		$Id: wellextractdata.h,v 1.5 2004-05-06 21:03:52 bert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "position.h"
#include "ranges.h"
#include "enums.h"

class MultiID;
class IODirEntryList;


namespace Well
{
class Info;
class Data;
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


/*!\brief Collects positions along selected well tracks */

class TrackSampler : public ::Executor
{
public:

    typedef TypeSet<BinIDValue>	BinIDValueSet;
    enum SelPol		{ NearPos, Corners };
    			DeclareEnumUtils(SelPol)

			TrackSampler(const BufferStringSet& ioobjids,
				     ObjectSet<BinIDValueSet>&);

    BufferString	topmrkr;
    BufferString	botmrkr;
    BufferString	lognm;
    float		above;
    float		below;
    SelPol		selpol;

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return "Scanning well tracks"; }
    const char*		nrDoneText() const { return "Wells inspected"; }
    int			nrDone() const	   { return curidx; }
    int			totalNr() const	   { return ids.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids; }
    ObjectSet<BinIDValueSet>&	bivSets()		{ return bivsets; }

    static const char*	sKeyTopMrk;
    static const char*	sKeyBotMrk;
    static const char*	sKeyLimits;
    static const char*	sKeySelPol;
    static const char*	sKeyDataStart;
    static const char*	sKeyDataEnd;
    static const char*	sKeyLogNm;

protected:

    const BufferStringSet&	ids;
    ObjectSet<BinIDValueSet>&	bivsets;
    int				curidx;
    const bool			timesurv;
    Interval<float>		fulldahrg;

    void		getData(const Data&,BinIDValueSet&);
    void		getLimitPos(const ObjectSet<Marker>&,bool,float&) const;
    bool		getSnapPos(const Data&,float,BinIDValue&,int&,
	    			   Coord3&) const;
    void		addBivs(BinIDValueSet&,const BinIDValue&,
				const Coord3&) const;

};


/*!\brief Collects positions along selected well tracks */

class LogDataExtracter : public ::Executor
{
public:

    enum SamplePol	{ Med, Avg, MostFreq, Nearest };
    			DeclareEnumUtils(SamplePol)

			LogDataExtracter(const BufferStringSet& ioobjids);

    SamplePol		samppol;
    static const char*	sKeySamplePol;

    void		usePar(const IOPar&);

    int			nextStep();
    const char*		message() const	   { return "Scanning well tracks"; }
    const char*		nrDoneText() const { return "Wells inspected"; }
    int			nrDone() const	   { return curidx; }
    int			totalNr() const	   { return ids.size(); }

    const BufferStringSet&	ioObjIds() const	{ return ids; }

protected:

    const BufferStringSet&	ids;
    int				curidx;
    const bool			timesurv;

};


}; // namespace Well


#endif
