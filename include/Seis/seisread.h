#ifndef seisread_h
#define seisread_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		27-1-98
 RCS:		$Id: seisread.h,v 1.38 2012-08-03 13:00:37 cvskris Exp $
________________________________________________________________________

-*/

#include "seismod.h"
#include "seisstor.h"
#include "seistype.h"
#include "linekey.h"
class SeisTrc;
class Executor;
class HorSampling;
class SeisTrcBuf;
class SeisPS3DReader;
namespace PosInfo { class Line2DData; class CubeDataIterator; }
namespace Seis { class Bounds; class Bounds2D; }


/*!\brief reads from a seismic data store.

If you don't want all of the stored data, you must set use the
SeisTrcTranslator facilities (SelData and ComponentData) after calling
prepareWork(). If you don't call prepareWork(), the reader will do that but
you cannot use SeisTrcTranslator facilities then.

Then, the routine is: get(trc.info()) possibly followed by get(trc).
Not keeping this sequence is at your own risk.

Note: 2D Pre-Stack data cannot (yet) be read via this class.

*/

mClass(Seis) SeisTrcReader : public SeisStoreAccess
{
public:

			SeisTrcReader(const IOObj* =0);
				//!< Open 'real user entries from '.omf' file
				//!< Can be anything: SEGY - CBVS - database
			SeisTrcReader(const char* fnm);
				//!< Open 'loose' CBVS files only.
			~SeisTrcReader();

    void		forceFloatData( bool yn=true )	{ forcefloats = yn; }
    			//!< Only effective if called before prepareWork()
    bool		prepareWork(Seis::ReadMode rm=Seis::Prod);
    			//!< After this, you can set stuff on the translator
    			//!< If not called, will be done automatically

    int			get(SeisTrcInfo&);
			/*!< -1 = Error. errMsg() will return a message.
			      0 = End
			      1 = Usable info
			      2 = Not usable (trace needs to be skipped)
			      If 1 is returned, then you should also call
			      get(SeisTrc&). */
			
    bool		get(SeisTrc&);
			/*!< It is possible to directly call this without
			     checking the get(SeisTrcInfo&) result. Beware that
			     the trace selections in the SelData may be
			     ignored then - depending on the Translator's
			     capabilities. */

    void		fillPar(IOPar&) const;

    bool		isPrepared() const		{ return prepared; }
    Seis::Bounds*	getBounds() const;
    			//!< use after prepareWork(). If not avail: survinfo
    void		setComponent( int ic )		{ selcomp_ = ic; }
    			//!< use before startWork()
    			//!< -1 (default) is all components

    			// 2D only
    int			curLineIdx() const		{ return curlineidx; }
    StepInterval<int>	curTrcNrRange() const		{ return curtrcnrrg; }
    LineKey		lineKey() const;
    LineKeyProvider*	lineKeyProvider() const;

protected:

    bool		foundvalidinl, foundvalidcrl;
    bool		new_packet, needskip;
    bool		forcefloats;
    bool		prepared;
    bool		inforead;
    int			prev_inl;
    int			curlineidx;
    int			nrfetchers;
    HorSampling*	outer;
    SeisTrcBuf*		tbuf_;
    Executor*		fetcher;
    Seis::ReadMode	readmode;
    bool		entryis2d;
    StepInterval<int>	curtrcnrrg;
    SeisPS3DReader*	psrdr_;
    PosInfo::CubeDataIterator* pscditer_;
    BinID		curpsbid_;
    int			selcomp_;

    void		init();
    Conn*		openFirst();
    bool		initRead(Conn*);
    int			nextConn(SeisTrcInfo&);
    bool		doStart();
    bool		ensureCurLineAttribOK(const BufferString&);

    bool		isMultiConn() const;
    void		startWork();

    int			getPS(SeisTrcInfo&);
    bool		getPS(SeisTrc&);

    int			get2D(SeisTrcInfo&);
    bool		get2D(SeisTrc&);
    bool		mkNextFetcher();
    bool		readNext2D();

    Seis::Bounds*	get3DBounds(const StepInterval<int>&,
	    			    const StepInterval<int>&,
				    const StepInterval<float>&) const;
    bool		initBounds2D(const PosInfo::Line2DData&,
	    			     Seis::Bounds2D&) const;

};


#endif

