#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nageswara
 Date:		July 2011
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "executor.h"
#include "emposid.h"

class od_ostream;
class DataPointSet;
namespace EM { class Horizon3D; class EMObjectIterator; }

mExpClass(EarthModel) IsochronMaker : public Executor
{ mODTextTranslationClass(IsochronMaker)
public:
			IsochronMaker(const EM::Horizon3D&,const EM::Horizon3D&,
				      const char* attrnm,int dataidx,
				      DataPointSet* dps=0);
			~IsochronMaker();

    int			nextStep() override;
    int			finishWork();
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override
			{ return tr("Positions handled"); }
    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totnr_; }

    void		setUnits( const bool isinmsc) { inmsec_ = isinmsc; }
    bool		saveAttribute(const EM::Horizon3D*,int attribidx,
	    			      bool overwrite,od_ostream* strm=0);
    static const char*	sKeyHorizonID();
    static const char*	sKeyCalculateToHorID();
    static const char*	sKeyAttribName();
    static const char*	sKeyOutputInMilliSecYN();
    static const char*	sKeyIsOverWriteYN();

protected:
    int				totnr_;
    od_int64			nrdone_;
    uiString			msg_;

    int				sidcolidx_;
    int				dataidx_;
    const EM::Horizon3D&	hor1_;
    const EM::Horizon3D&	hor2_;
    DataPointSet*		dps_;
    EM::EMObjectIterator*	iter_;
    bool			inmsec_;
};
