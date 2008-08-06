#ifndef segydirect_h
#define segydirect_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segydirectdef.h,v 1.1 2008-08-06 12:08:56 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "iopar.h"
#include "bufstringset.h"
#include "position.h"
#include "samplingdata.h"
class SeisTrc;
class Executor;
class TaskRunner;
class DataPointSet;
class PosGeomDetector;
class SEGYSeisTrcTranslator;
namespace Seis { class Bounds; }



class SEGYDirectDef
{
public:

    			SEGYDirectDef(Seis::GeomType,const IOPar&);
    			SEGYDirectDef(const char*); // read from file
			~SEGYDirectDef();

    bool		readFromFile(const char*);
    bool		writeToFile(const char*) const;

    Seis::GeomType	geomType() const	{ return geom_; }   
    const IOPar&	pars() const		{ return pars_; }
    const char*		path() const		{ return path(); }
    const ObjectSet<DataPointSet> data() const	{ return data_; }
    SamplingData<float>	sampling() const	{ return sampling_; }
    int			nrSamples() const	{ return trcsz_; }

    int			nrDataPointSets() const	{ return data_.size(); }
    const DataPointSet&	dataPointSet( int i ) const { return *data_[i]; }
    const PosGeomDetector& geomDetector() const	{ return geomdtector_; }

    bool		doScan(const char* dirnm,TaskRunner* tr=0,
	    			const char* globexpr=0);

    Seis::Bounds*	getBounds(const char* lnm=0) const;

protected:

    Seis::GeomType	geom_;
    const IOPar		pars_;
    BufferString	path_;
    ObjectSet<DataPointSet> data_;
    PosGeomDetector&	geomdtector_;

    int			bpsample_;
    SamplingData<float>	sampling_;
    int			trcsz_;

    friend struct	SEGYDirectDefScanner;

public:

    int			segyformat_;
    bool		rev1_;
    BufferStringSet	fnms_;
    BufferStringSet	failedfnms_;
    BufferStringSet	errmsgs_;
};


#endif
