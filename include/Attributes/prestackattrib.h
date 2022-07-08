#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	B.Bril & H.Huck
 Date:		14-01-2008
________________________________________________________________________

-*/


#include "attributesmod.h"
#include "attribprovider.h"
#include "prestackanglecomputer.h"
#include "prestackprop.h"
#include "multiid.h"
#include "datapack.h"

class SeisPSReader;
class IOObj;

namespace PreStack { class Gather;
		     class GatherSetDataPack; class ProcessManager; }


namespace Attrib
{

/*!
\brief "Prestack Attribute"

  Outputs a standard attribute from prestack data.
  Classname should really be PreStack, but the compiler complains and mixes up
  with PreStack namespace.

<pre>
  PreStack calctype= axistype= lsqtype= offsaxis= valaxis= useazim= comp=
  aperture= preprocessor=


  Input:
  0		Prestack Data

  Output:
  0		Attribute
</pre>
*/

mExpClass(Attributes) PSAttrib : public Provider
{ mODTextTranslationClass(PSAttrib)
public:

    static void		initClass();

			PSAttrib(Desc&);

    static const char*	attribName()		{ return "PreStack"; }
    static const char*	offStartStr()		{ return "offstart"; }
    static const char*	offStopStr()		{ return "offstop"; }
    static const char*	preProcessStr()		{ return "preprocessor"; }
    static const char*	calctypeStr()		{ return "calctype"; }
    static const char*	stattypeStr()		{ return "stattype"; }
    static const char*	lsqtypeStr()		{ return "lsqtype"; }
    static const char*	offsaxisStr()		{ return "offsaxis"; }
    static const char*	valaxisStr()		{ return "valaxis"; }
    static const char*	componentStr()		{ return "comp"; }
    static const char*	apertureStr()		{ return "aperture"; }
    static const char*	velocityIDStr()		{ return "velocityid"; }
    static const char*	useangleStr()		{ return "useangle"; }
    static const char*	angleStartStr()		{ return "anglestart"; }
    static const char*	angleStopStr()		{ return "anglestop"; }
    static const char*	rayTracerParamStr()	{ return "raytracerparam"; }
    static const char*	gathertypeStr()		{ return "gathertype"; }
    static const char*	xaxisunitStr()		{ return "xaxisunit"; }
    static const char*	angleDPIDStr()		{ return "angleid"; }
    static const char*	angleSmoothType()	{ return "anglesmoothtype"; }
    static const char*	angleFiltFunction()	{ return "anglefiltfunc"; }
    static const char*	angleFiltValue()	{ return "anglefiltval"; }
    static const char*	angleFiltLength()	{ return "anglefiltlen"; }
    static const char*	angleFFTF3Freq()	{ return "anglefftf3freq"; }
    static const char*	angleFFTF4Freq()	{ return "anglefftf4freq"; }

    enum GatherType	{ Off, Ang };
			mDeclareEnumUtils(GatherType)
    enum XaxisUnit	{ Deg, Rad };
			mDeclareEnumUtils(XaxisUnit)

    const PreStack::PropCalc::Setup&	setup() const	{ return setup_; }
    const MultiID&			psID() const	{ return psid_; }
    const MultiID&			preProcID() const { return preprocid_; }
    const MultiID&			velocityID() const
					{ return velocityid_; }

    void		updateCSIfNeeded(TrcKeyZSampling&) const override;
    void		setAngleComp(PreStack::AngleComputer*);
    void		setAngleData(DataPack::ID);

protected:

			~PSAttrib();
    static Provider*	createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const override
			{ return true;}

    bool		getInputOutput(int input,
				       TypeSet<int>& res) const override;
    bool		getInputData(const BinID&, int idx) override;
    bool		getAngleInputData();
    bool		computeData(const DataHolder&,const BinID& relpos,
			    int t0,int nrsamples,int threadid) const override;
    void		prepPriorToBoundsCalc() override;
    void		setSmootheningPar();
    float		getXscaler(bool isoffset, bool isindegrees) const;
    DataPack::ID	getPreProcessedID(const BinID& relbid);
    bool		getGatherData(const BinID& bid,DataPack::ID& gatherid,
				      DataPack::ID& angglegid);

    MultiID			psid_;
    IOObj*			psioobj_ = nullptr;
    SeisPSReader*		psrdr_ = nullptr;
    int				component_ = 0;
    PreStack::ProcessManager*	preprocessor_ = nullptr;
    PreStack::PropCalc*		propcalc_ = nullptr;
    PreStack::PropCalc::Setup	setup_;
    RefMan<PreStack::AngleComputer>	anglecomp_ = nullptr;
    DataPack::ID		anglegsdpid_ = DataPack::FullID::PackID(-1);

    MultiID			preprocid_;
    int				dataidx_;
    const DataHolder*		inputdata_;
    MultiID			velocityid_;

    ObjectSet<PreStack::Gather>    gatherset_;

private:

    WeakPtr<PreStack::GatherSetDataPack>	getMemoryGatherSetDP() const;
    void			setGatherIsAngle(PreStack::Gather&);
};

}; // namespace Attrib

