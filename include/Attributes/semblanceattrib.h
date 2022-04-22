#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		07-01-2008
________________________________________________________________________

-*/

#include "attributesmod.h"
#include "attribprovider.h"

namespace Attrib
{

/*!\brief Semblance Attribute

Semblance gate= pos0= pos1= stepout=1,1
extension=[0|90|180|Cube|Cross|AllDir|Diagonal] steering=[Yes|No]


If steering is enabled, it is up to the user to make sure that the steering
goes to the same position as pos0 and pos1 respectively.

Input:
0	Data
1	Steering

Extension:	0	90/180	 Cube	Cross	   AllDir	Diagonal
1		pos0	pos0		0,0	   0,0		0,0
2		pos1	pos1		0,step	   -step,step	-step,step
3			pos0rot		step,0	   0,step	step,step
4			pos1rot		0,-step    step,step	step,-step
5					-step,0    step,0	-step,-step
6						   step,-step
7						   0,-step
8						   -step,-step
9						   -step,0

Output:
0	Semblance

*/

mExpClass(Attributes) Semblance : public Provider
{
public:
    static void			initClass();
				Semblance(Desc&);

    static const char*		attribName()	{ return "Semblance"; }
    static const char*		gateStr()	{ return "gate"; }
    static const char*		pos0Str()	{ return "pos0"; }
    static const char*		pos1Str()	{ return "pos1"; }
    static const char*		stepoutStr()	{ return "stepout"; }
    static const char*		steeringStr()	{ return "steering"; }
    static const char*		extensionStr()	{ return "extension"; }
    static const char*		extensionTypeStr(int);
    void			initSteering() override
				{ stdPrepSteering(stepout_); }

    void			prepPriorToBoundsCalc() override;

protected:
				~Semblance() {}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);

    bool			allowParallelComputation() const override
				{ return true; }

    bool			getInputOutput(int inp,
					    TypeSet<int>& res) const override;
    bool			getInputData(const BinID&,int zintv) override;
    bool			computeData(const DataHolder&,
					    const BinID& relpos,
					    int z0,int nrsamples,
					    int threadid) const override;

    const BinID*		reqStepout(int input,int output) const override;
    const BinID*		desStepout(int input,int output) const override;
    const Interval<float>*	reqZMargin(int input,int output) const override;
    const Interval<float>*	desZMargin(int input,int output) const override;
    bool			getTrcPos();

    BinID			pos0_;
    BinID			pos1_;
    BinID			stepout_;
    Interval<float>		gate_;
    int				extension_;
    TypeSet<BinID>		trcpos_;

    Interval<float>		desgate_;

    bool			dosteer_;
    TypeSet<int>		steerindexes_;
    int				dataidx_;

    ObjectSet<const DataHolder> inputdata_;
    const DataHolder*		steeringdata_;
};

} // namespace Attrib

