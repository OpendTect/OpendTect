#ifndef sopachmaker_h
#define isopachmaker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2011
 RCS:		$Id: isopachmaker.h,v 1.4 2011/07/11 11:43:21 cvsranojay Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "emposid.h"

class DataPointSet;
namespace EM{ class Horizon3D; class EMObjectIterator; }

mClass IsopachMaker : public Executor
{
public:
			IsopachMaker(const EM::Horizon3D&,const EM::Horizon3D&,
			     	     const char* attrnm,int dataidx,
				     DataPointSet* dps=0);
			~IsopachMaker();

    int			nextStep();
    int			finishWork();
    const char*		message() const		{ return msg_.buf(); }
    const char*		nrDoneText() const	{ return "Positions handled"; }
    od_int64 		nrDone() const		{ return nrdone_; }
    od_int64		totalNr() const		{ return totnr_; }

    void		setUnits( const bool isinmsc) { inmsec_ = isinmsc; }
    bool		saveAttribute(const EM::Horizon3D*,int attribidx,
	    			      bool overwrite, std::ostream* strm=0);
    static const char*	sKeyHorizonID();
    static const char*	sKeyCalculateToHorID();
    static const char*	sKeyAttribName();
    static const char*	sKeyOutputInMilliSecYN();
    static const char*	sKeyIsOverWriteYN();

protected:
    int				totnr_;
    od_int64			nrdone_;
    BufferString		msg_;

    int				sidcolidx_;
    int				dataidx_;
    const EM::Horizon3D&	hor1_;
    const EM::Horizon3D&	hor2_;
    DataPointSet*		dps_;
    EM::EMObjectIterator*	iter_;
    const EM::SectionID		sectid1_;
    const EM::SectionID		sectid2_;
    bool			inmsec_;
};

#endif
