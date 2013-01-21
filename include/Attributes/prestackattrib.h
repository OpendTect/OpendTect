#ifndef prestackattrib_h
#define prestackattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        B.Bril & H.Huck
 Date:          14-01-2008
 RCS:           $Id$
________________________________________________________________________

-*/


#include "attributesmod.h"
#include "attribprovider.h"
#include "prestackprop.h"
#include "multiid.h"

class SeisPSReader;
class IOObj;

namespace PreStack { class ProcessManager; class Gather; }


namespace Attrib
{

/*!
\ingroup Attributes
\brief "Pre-Stack Attribute"

  Outputs a standard attribute from pre-stack data.
  Classname should really be PreStack, but the compiler complains and mixes up
  with PreStack namespace.

<pre>
  PreStack calctype= axistype= lsqtype= offsaxis= valaxis= useazim= comp=
  aperture= preprocessor=
  
  
  Input:
  0		Pre-Stack Data
  
  Output:
  0		Attribute
</pre>
*/
    
mExpClass(Attributes) PSAttrib : public Provider
{
public:

    static void		initClass();

			PSAttrib(Desc&);

    static const char*  attribName()		{ return "PreStack"; }
    static const char*  offStartStr()		{ return "offstart"; }
    static const char*  offStopStr()		{ return "offstop"; }
    static const char*  preProcessStr()		{ return "preprocessor"; }
    static const char*  calctypeStr()		{ return "calctype"; }
    static const char*  stattypeStr()		{ return "stattype"; }
    static const char*  lsqtypeStr()		{ return "lsqtype"; }
    static const char*  offsaxisStr()		{ return "offsaxis"; }
    static const char*  valaxisStr()		{ return "valaxis"; }
    static const char*  useazimStr()		{ return "useazim"; }
    static const char*  componentStr()		{ return "comp"; }
    static const char*  apertureStr()		{ return "aperture"; }

    const ::PreStack::PropCalc::Setup&	setup() const	{ return setup_; }
    const MultiID&			psID() const	{ return psid_; }
    const MultiID&			preProcID() const { return preprocid_; }

    void                updateCSIfNeeded(CubeSampling&) const;

protected:

			~PSAttrib();
    static Provider*    createInstance(Desc&);

    bool		allowParallelComputation() const	{ return true;}
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;
    void		prepPriorToBoundsCalc();

    MultiID			psid_;
    IOObj*			psioobj_;
    ::PreStack::PropCalc::Setup setup_;
    int				component_;
    SeisPSReader*		psrdr_;
    ::PreStack::PropCalc*	propcalc_;

    ::PreStack::ProcessManager*	preprocessor_;
    MultiID			preprocid_;
    int				dataidx_;
    const DataHolder*		inputdata_;

    ObjectSet<PreStack::Gather>    gatherset_; 
};

}; // namespace Attrib

#endif

