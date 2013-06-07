#ifndef gapdeconattrib_h
#define gapdeconattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          14-07-2006
 RCS:           $Id: gapdeconattrib.h,v 1.15 2009/07/22 16:01:27 cvsbert Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

/*!\brief Gap Decon Attribute
*/

namespace Attrib
{

class GapDecon : public Provider
{
public:

    static void			initClass();
				GapDecon(Desc&);

    static const char*		attribName()	{ return "GapDecon"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		lagsizeStr()	{ return "lagsize"; }
    static const char*		gapsizeStr()	{ return "gapsize"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		noiselevelStr()	{ return "noiselevel"; }
    static const char*		isinp0phaseStr(){ return "isinpzerophase"; }
    static const char*		isout0phaseStr(){ return "isoutzerophase"; }
    static const char*		onlyacorrStr()	{ return "onlyautocorr"; }

    void                        prepareForComputeData();

protected:

    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const
				{ return false; }

    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
	    				    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const;
    const Interval<int>*	desZSampMargin(int,int) const
    				{ return &dessampgate_; }

    Interval<float>		gate_;
    int				lagsize_;
    int				gapsize_;
    int				noiselevel_;

    bool			isinpzerophase_;
    bool			isoutzerophase_;
    BinID			stepout_;
    bool			useonlyacorr_;

    int				nlag_;
    int				ncorr_;
    int				lcorr_;
    int				ngap_;
    
    const DataHolder*		inputdata_;
    const DataHolder*		inputdatamixed_;
    int				dataidx_;
    int				dataidxmixed_;

    const float*		hilbfilter_;
    Interval<int>		dessampgate_;
};

} // namespace Attrib


#endif
