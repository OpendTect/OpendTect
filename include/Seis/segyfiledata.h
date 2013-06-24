#ifndef segyfiledata_h
#define segyfiledata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "seismod.h"
#include "bufstring.h"
#include "bufstringset.h"
#include "iopar.h"
#include "manobjectset.h"
#include "position.h"
#include "samplingdata.h"
#include "seisposkey.h"
#include "sortedtable.h"
#include "threadlock.h"

class ascostream;
class DataCharacteristics;
template <class T> class DataInterpreter;
class SEGYSeisTrcTranslator;
namespace Seis { class PosIndexer; }
 

namespace SEGY
{

/*!<Stores scanned data from SEGY-files. */

mExpClass(Seis) FileDataSet
{
public:

    struct TrcIdx
    {
			TrcIdx( int fnr=-1, od_int64 tnr=0 )
			    : filenr_(fnr), trcidx_(tnr)	{}
	bool		isValid() const		{ return filenr_>=0; }
	void		toNextFile()		{ filenr_++; trcidx_ = 0; }

	int		filenr_;
	od_int64	trcidx_;
    };

    			FileDataSet(const IOPar& iop, ascistream& );
			//!<Reads old version of file into memory
    			FileDataSet(const IOPar& iop);
			/*!<Creates empty set, can later be filled from
			    scanning. */
    			~FileDataSet();

			FileDataSet(const IOPar&,const char* filename,
				od_int64 start,
				const DataCharacteristics& int32 );
			/*!<Reads new version of file, only auxdata is read
			    in, the bulk of the data remains on disk. */
    			FileDataSet(const FileDataSet& fd);
			//!<Not implemented, just to make linker complain.

    void		save2DCoords(bool yn);

    bool		setOutputStream(std::ostream&);
    			/*!<Will store all information to the stream, rather 
			    than in memory. */
    void		setAuxData(const Seis::GeomType&,
	    			   const SEGYSeisTrcTranslator&);

    void		addFile(const char* fnm);
    bool		addTrace(int fileidx,const Seis::PosKey&,const Coord&,
	    			 bool usable);

    const SamplingData<float>&	getSampling() const { return sampling_; }
    int				getTrcSz() const { return trcsz_; }

    				//Auxdata
    int				nrFiles() const;
    FixedString			fileName(int) const;
    bool			isEmpty() const		{ return totalsz_==1; }
    od_int64			size() const		{ return totalsz_; }
    bool			isRev1() const		{ return isrev1_; }
    Seis::GeomType		geomType() const	{ return geom_; }
    const IOPar&		segyPars() const	{ return segypars_;}

    				//TraceData
    bool			getDetails(od_int64,Seis::PosKey&,
	    				   bool& usable) const;
    Coord			get2DCoord(int trcnr) const;
    TrcIdx			getFileIndex(od_int64) const;


    //bool		toNext(TrcIdx&,bool allownull=true,
	    			//bool allownotusable=false) const;

    void			setIndexer(Seis::PosIndexer* n);
				/*!<addTrace will send the trace info to the
				    indexer. Indexer must be kept alive outside
				    object. */

    void			getReport(IOPar&) const;
    void			dump(std::ostream&) const;

    bool			usePar(const IOPar& iop);
    				//!<Read auxdata from storage
    void			fillPar(IOPar& iop) const;
    				//!<Write auxdata

protected:

    struct		StoredData
    {
			StoredData(const char* filename,od_int64 start,
				const DataCharacteristics& int32);
			StoredData(std::ostream&);
			~StoredData();

	bool		getKey(od_int64, Seis::PosKey&, bool& ) const;
	bool		add(const Seis::PosKey&,bool);

    protected:
	DataInterpreter<int>*		int32di_;

	mutable Threads::Lock		lock_;
	std::istream*			istrm_;
	od_int64			start_;

	std::ostream*			ostrm_;
    };

    bool			readVersion1( ascistream& );
    bool			readVersion1File( ascistream& );

    				//Auxdata
    IOPar			segypars_;
    Seis::GeomType		geom_;
    bool			isrev1_;
    SamplingData<float>		sampling_;
    int				trcsz_;
    int				nrstanzas_;
    BufferStringSet		filenames_;
    TypeSet<od_int64>		cumsizes_;
    od_int64			totalsz_;
    int				nrusable_;

    				//TraceData
    TypeSet<Seis::PosKey>	keys_;
    BoolTypeSet			usable_;
    StoredData*			storeddata_;
    SortedTable<int,Coord>*	coords_; //trcnr vs coord

    Seis::PosIndexer*		indexer_;
};

} // namespace


#endif

