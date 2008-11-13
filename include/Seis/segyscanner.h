#ifndef segyscanner_h
#define segyscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Jul 2008
 RCS:		$Id: segyscanner.h,v 1.11 2008-11-13 11:33:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "seistype.h"
#include "bufstringset.h"
#include "position.h"
#include "segyfiledef.h"
class IOPar;
class SeisTrc;
class DataPointSet;
class SEGYSeisTrcTranslator;
namespace PosInfo { class Detector; }


namespace SEGY
{
class FileDataSet;


class Scanner : public Executor
{
public:

    			Scanner(const IOPar&,Seis::GeomType);
    			Scanner(const FileSpec&,Seis::GeomType,const IOPar&);
			~Scanner();

    Seis::GeomType	geomType() const	{ return geom_; }
    const IOPar&	pars() const		{ return pars_; }
    void		setMaxNrtraces( int n )	{ nrtrcs_ = n; }
    void		setForceRev0( bool yn )	{ forcerev0_ = yn; }

    int			nextStep();
    const char*		message() const		{ return msg_.buf(); }
    od_int64		nrDone() const		{ return nrdone_; }
    const char*		nrDoneText() const	{ return "Traces scanned"; }

    const FileDataSet&	fileDataSet() const	{ return fds_; }

    BufferStringSet	fnms_;		//!< Actually used, possibly with errs
    BufferStringSet	failedfnms_;	//!< Failed to open or read
    BufferStringSet	failerrmsgs_;	//!< Err Msgs for failed
    BufferStringSet	scanerrfnms_;	//!< Error during scan (but in fnms_)
    BufferStringSet	scanerrmsgs_;	//!< Err Msgs for 'Error during scan'

    void			getReport(IOPar&) const;
    StepInterval<float>		zRange() const;
    const PosInfo::Detector&	posInfoDetector() const { return dtctor_; }

    const BufferStringSet&	warnings() const	{ return trwarns_; }

protected:

    Seis::GeomType	geom_;
    const IOPar&	pars_;
    FileDataSet&	fds_;
    SEGYSeisTrcTranslator* tr_;
    int			nrtrcs_;
    bool		forcerev0_;

    SeisTrc&		trc_;
    int			curfidx_;
    BufferString	msg_;
    int			nrdone_;
    PosInfo::Detector&	dtctor_;
    BufferStringSet	trwarns_;

    int			openNext();
    int			readNext();
    void		closeTr();

    void		init(const FileSpec&);
    int			finish(bool);
    void		addFailed(const char*);
    void		initFileData();
    void		addErrReport(IOPar&) const;

};

} // namespace

#endif
