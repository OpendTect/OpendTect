#pragma once
/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz; http://www.joanneum.at/resources/gph/
 mitarbeiterinnen/mitarbeiter-detailansicht/person/0/3144/eichkitz.html
 * DATE     : November 2013
-*/

#include "attribprovider.h"
#include "arraynd.h"
#include "glcmmod.h"

#include <utility>

namespace Attrib
{

mExpClass(GLCM) GLCM_attrib : public Provider
{ mODTextTranslationClass(GLCM_attrib);
public:
    static void			initClass();
    explicit			GLCM_attrib(Desc&);
    static const char*		attribName()		{ return "GLCM"; }
    static const char*		gateStr()		{ return "gate"; }
    static const char*		numbergreyStr()		{ return "greylevels"; }
    static const char*		attributeStr()		{ return "attribute"; }
    static const char*		stepoutStr()		{ return "stepout"; }
    static const char*		directionStr()		{ return "direction"; }
    static const char*		minlimitStr()		{ return "minlimit"; }
    static const char*		maxlimitStr()		{ return "maxlimit"; }
    static const char*		sampStr()		{ return "samples"; }
    static const char*		steeringStr()		{ return "steering"; }
    static const char*		attribTypeStr(int);
    static const char*		directTypeStr(int);
    void			initSteering()	{ stdPrepSteering(stepout_); }

protected:
    ~GLCM_attrib()		{}
    static Provider*		createInstance(Desc&);
    static void			updateDesc(Desc&);
    static void			updateDefaults(Desc&);

    bool			allowParallelComputation() const
				{ return true; }

    bool			getInputOutput(int input,TypeSet<int>&) const;
    bool			getInputData(const BinID&,int zintv);
    bool			computeData(const DataHolder&,
					    const BinID& relpos,int z0,
					    int nrsamples, int threadid) const;

    const BinID*		desStepout(int,int) const;
    const BinID*		reqStepout(int input, int output) const;

    const Interval<float>*	reqZMargin( int input, int output ) const;
    const Interval<float>*	desZMargin( int input, int output ) const;
    const Interval<int>*	desZSampMargin( int input, int output ) const;

private:
    int				attribute_;
    int				usegreylevels_;
    int				direction_;
    int				samples_;
    float			minlimit_;
    float			maxlimit_;
    int				samprange_;
    bool			dosteer_;
    BinID			stepout_;

    Interval<int>		sampgate_;
    Interval<float>		gate_;
    Interval<float>		desgate_;

    ObjectSet<const DataHolder> inpdata_;
    const DataHolder*		steerdata_;
    int				dataidx_;

    struct Node
    {
	int			nodeI;
	int			nodeJ;
	int			numbercoocurrence;
	Node*			next;
    };

    Node			Node_;


    struct PosAndSteeridx
    {
	TypeSet<int>		steerindexes_;
	TypeSet<BinID>		positions_;
	TypeSet<int>		posidx_;
    };

    PosAndSteeridx		posandsteeridx_;

    int				computeGreyLevel(float) const;
    std::pair<double, double>	computeMu(Node* LinkedList,
					  int elements) const;
    std::pair<double, double>	computeSigma(Node* LinkedList,
					     int elements,double MuX,
					     double MuY) const;
    double			computeMean(Node* LinkedList,
					    int elements) const;
    double			computeMuXminusY(Node* LinkedList,
						 int elements) const;
    double			computeMXplusY(Node* LinkedList,
					       int elements,int N) const;
    double			computeMXminusY(Node* LinkedList,
						int elements, int N) const;
    double			computeMxI(Node* LinkedList,
					   int elements,int i) const;
    double			computeMyJ(Node* LinkedList,
					   int elements,int j) const;
    double			computeHXY1(Node* LinkedList,
					    int elements) const;
    double			computeHXY2(Node* LinkedList,
					    int elements) const;
    double			computeHX(Node* LinkedList,int elements) const;
    double			computeHY(Node* LinkedList,int elements) const;
    double			computeEnergy(Node* head,int elements) const;
    double			computeContrast(Node* LinkedList,
						int elements) const;
    double			computeCorrelation(Node* LinkedList,
						   int elements) const;
    double			computeVariance(Node* LinkedList,
						int elements) const;
    double			computeInverseDifferenceMoment(
							Node* LinkedList,
							int elements) const;
    double			computeSumAverage(Node* LinkedList,
						  int elements) const;
    double			computeSumVariance(Node* LinkedList,
						   int elements) const;
    double			computeSumEntropy(Node* LinkedList,
						  int elements) const;
    double			computeEntropy(Node* LinkedList,
					       int elements) const;
    double			computeDifferenceVariance(Node* LinkedList,
							  int elements) const;
    double			computeDifferenceEntropy(
						Node* LinkedList,
						int elements) const;
    double			computeF12(Node* LinkedList,int elements) const;
    double			computeF13(Node* LinkedList,int elements) const;
    double			computeHomogeneity(Node* LinkedList,
						   int elements) const;
    double			computeSumMean(Node* LinkedList,
					       int elements) const;
    double			computeMaximumProbability(Node* LinkedList,
							  int elements) const;
    double			computeClusterTendency(Node* LinkedList,
						       int elements) const;
    double			computeClusterShade(Node* LinkedList,
						    int elements) const;
    double			computeClusterProminence(Node* LinkedList,
							 int elements) const;
    double			computeDissimilarity(Node* LinkedList,
						     int elements) const;
    double			computeDifferenceMean(Node* LinkedList,
						      int elements) const;
    double			computeAutocorrelation(Node* LinkedList,
						       int elements) const;
    double			computeInertia(Node* LinkedList,
					       int elements) const;
    bool			getValIJ(int,int,int,int,int,
					 float&,float&) const;

};

};

