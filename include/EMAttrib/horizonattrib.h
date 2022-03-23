#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          September 2006
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "attribprovider.h"
#include "bufstring.h"
#include "multiid.h"

namespace EM { class Horizon; }

namespace Attrib
{

class DataHolder;

mClass(EMAttrib) Horizon : public Provider
{ mODTextTranslationClass(Horizon);
public:
    static void		initClass();
			Horizon(Desc&);
			~Horizon();

    virtual void	prepareForComputeData();

    static const char*	attribName()	{ return "Horizon"; }
    static const char*	sKeyHorID()	{ return "horid"; }
    static const char*	sKeySurfDataName(){ return "surfdatanm"; }
    static const char*	sKeyType()	{ return "type"; }
    static const char*	sKeyRelZ()	{ return "relz"; }
    static const char*	outTypeNamesStr(int);

    bool                isOK() const;

protected:
    static Provider*	createInstance( Desc& );
    static void         updateDesc( Desc& );

    virtual bool	getInputData(const BinID&,int intv);
    virtual bool	computeData(const DataHolder&,const BinID& relpos,
	    			    int z0,int nrsamples,int threadid) const;

    virtual bool	allowParallelComputation() const { return true; }

    void		fillLineID();

    MultiID		horid_;
    BufferString	surfdatanm_;
    int			outtype_;
    bool		relz_;

    EM::Horizon*	horizon_;
    const DataHolder*	inputdata_;
    int			dataidx_;
    int			horizon2dlineid_;
};

} // namespace Attrib
