#ifndef seiscbvsps_h
#define seiscbvsps_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seiscbvsps.h,v 1.2 2004-12-30 15:04:40 bert Exp $
________________________________________________________________________

-*/

#include "seispsread.h"
#include "seispswrite.h"
class IOPar;
template <class T> class SortedList;
class SeisTrcTranslator;


/*!\brief Implementation class: I/O from a CBVS pre-stack seismic data store.

  Every inline is a CBVS cube. A gather corresponds to one crossline. 
  Because CBSV seismics is inline-sorted, the crossline number is stored
  as inline in the cube. Upon retrieval actual BinID and Coord are restored.

 */

class SeisCBVSPSIO
{
public:

    			SeisCBVSPSIO(const char* dirnm);
			// Check errMsg() to see failure
    virtual		~SeisCBVSPSIO();

protected:


    mutable BufferString errmsg_;
    const BufferString	dirnm_;
    const BufferString	selmask_;
    const char*		ext() const	{ return selmask_.buf()+1; }

};


/*!\brief reads from a CBVS pre-stack seismic data store. */

class SeisCBVSPSReader : public SeisPSReader
		       , private SeisCBVSPSIO
{
public:

    			SeisCBVSPSReader(const char* dirnm);
			// Check errMsg() to see failure
			~SeisCBVSPSReader();

    bool		getGather(const BinID&,SeisTrcBuf&) const;
    const char*		errMsg() const		{ return errmsg_.buf(); } 

    const SortedList<int>& inlines() const		{ return inls_; }

protected:

    SortedList<int>&	inls_;

};


/*!\brief writes to a CBVS pre-stack seismic data store.

 Note: Can make new data stores and append new inlines to existing.
 Will replace any existing inlines.

 */

class SeisCBVSPSWriter : public SeisPSWriter
		       , private SeisCBVSPSIO
{
public:

    			SeisCBVSPSWriter(const char* dirnm);
			// Check errMsg() to see failure
			~SeisCBVSPSWriter();

    void		setPrefStorType( DataCharacteristics::UserType ut )
						{ reqdtype_ = ut; }
    void		close();

    bool		put(const SeisTrc&);
    const char*		errMsg() const		{ return errmsg_.buf(); } 

protected:

    int					curinl_;
    int					nringather_;
    DataCharacteristics::UserType	reqdtype_;
    DataCharacteristics			dc_;
    SeisTrcTranslator*			tr_;

    bool				newInl(const SeisTrc&);

};


#endif
