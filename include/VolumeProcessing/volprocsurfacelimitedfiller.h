#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Y.C. Liu
 Date:		April 2010
________________________________________________________________________

-*/

#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "dbkey.h"

namespace Geometry { class FaultStickSurface; }
namespace EM { class Fault; class Horizon; class Horizon3D; }

namespace VolProc
{

/*!Fills a volume with values.

  The borders are given by a set of horizons
  or faults with pre set side to calculate. The values are calculated based on
  v0+g*d, where for each variable we use either fixed values or from some
  fixed horizon reference data.
*/


mExpClass(VolumeProcessing) SurfaceLimitedFiller : public Step
{ mODTextTranslationClass(SurfaceLimitedFiller);
public:
		mDefaultFactoryCreatorImpl0Param( Step, SurfaceLimitedFiller );
		mDefaultFactoryInstantiationBase( "SurfaceLimitedFiller",
				tr("Horizon-based painter - Advanced") );

			SurfaceLimitedFiller();
			~SurfaceLimitedFiller();
    virtual void	releaseData();

    bool		isOK() const;

    bool		setSurfaces(const DBKeySet&,
				    const TypeSet<char>& fillside);
			/*Assume going down increases z.
			  For horizons, side = 1 if go below, -1 if go above.*/

    int			nrOfSurfaces() const		 {return side_.size();}
    char		getSurfaceFillSide(int idx) const{ return side_[idx]; }
    const DBKey*	getSurfaceID(int idx) const;

			//Start value/grid stuff
    bool		usesStartValue() const	    { return usestartval_; }
    void		useStartValue(bool yn)	    { usestartval_ = yn; }

    float		getStartValue() const	    { return fixedstartval_; }
    void		setStartValue(float vel)    { fixedstartval_ = vel; }

    bool		setStartValueHorizon(const DBKey*);
    const DBKey*	getStartValueHorizonID() const;
    int			getStartAuxdataIdx() const {return startauxdataselidx_;}
    void		setStartAuxdataIdx(int i)  { startauxdataselidx_ = i; }

			//Gradient value/grid stuff
    bool		usesGradientValue() const   { return usegradient_; }
    void		useGradientValue(bool yn)   { usegradient_ = yn; }

    bool		isGradientVertical() const  { return gradvertical_; }
    void		setGradientVertical(bool yn){ gradvertical_ = yn; }

    float		getGradient() const	{ return fixedgradient_; }
    void		setGradient(float grd)	{ fixedgradient_ = grd; }

    bool		setGradientHorizon(const DBKey*);
    const DBKey*	getGradientHorizonID() const;
    int			getGradAuxdataIdx()	{ return gradauxdataselidx_; }
    void		setGradAuxdataIdx(int i){ gradauxdataselidx_ = i; }

			//Reference horizon/z stuff
    bool		usesRefZValue() const		{ return userefz_; }
    void		useRefZValue(bool yn)		{ userefz_ = yn; }

    void		setRefZValue(float zv)		{ refz_ = zv; }
    float		getRefZValue() const		{ return refz_; }

    bool		setRefHorizon(const DBKey*);
    const DBKey*	getRefHorizonID() const;

    bool		useHorInterFillerPar(const IOPar&);

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);

private:

    virtual bool	needsFullVolume() const		{ return false; }
    virtual bool	canInputAndOutputBeSame() const { return true; }
    virtual bool	areSamplesIndependent() const	{ return true; }
    virtual bool	needsInput() const		{ return false; }
    virtual bool	isInputPrevStep() const		{ return true; }
    virtual bool	canHandle2D() const		{ return false; }
    virtual bool	prefersBinIDWise() const	{ return true; }

    virtual bool	prepareComp(int);
    virtual bool	computeBinID(const BinID&, int);
    virtual od_int64	extraMemoryUsage(OutputSlotID,
					 const TrcKeyZSampling&) const;

    EM::Horizon*	loadHorizon(const DBKey&) const;
			//!<\note horizon is reffed on return.
    int			setDataHorizon(const DBKey&,EM::Horizon3D*&,
				       int auxdataidx) const;

    DBKey		gradhorid_;
    EM::Horizon3D*	gradhorizon_;
    int			gradauxdataselidx_;
    int			gradauxidx_;
    float		fixedgradient_;
    bool		usegradient_;
    bool		gradvertical_;

    DBKey		starthorid_;
    EM::Horizon3D*	starthorizon_;
    int			startauxdataselidx_;
    int			startauxidx_;
    float		fixedstartval_;
    bool		usestartval_;

    DBKey		refhorid_;
    EM::Horizon*	refhorizon_;
    float		refz_;
    bool		userefz_;

			/* The following four have the same size, for any idx,
			      faults_[idx] or hors_[idx] is 0. */
    TypeSet<char>	side_;
    DBKeySet	surfacelist_;
    ObjectSet<EM::Horizon> hors_;
    ObjectSet<Geometry::FaultStickSurface> faults_;

    bool		usebottomval_;
    double		valrange_;

    static const char*	sKeySurfaceID()	    { return "Surface MID"; }
    static const char*	sKeySurfaceFillSide() { return "Surface fill side"; }
    static const char*	sKeyNrSurfaces()    { return "Nr of surfaces"; }

    static const char*	sKeyUseStartValue() { return "Use start value"; }
    static const char*	sKeyStartValue()    { return "Start value"; }
    static const char*	sKeyStartValHorID() { return "Start value horizon"; }
    static const char*	sKeyStartAuxDataID(){ return "Start auxdata id"; }

    static const char*	sKeyUseGradValue()  { return "Use gradient value"; }
    static const char*	sKeyGradValue()	    { return "Gradient value"; }
    static const char*	sKeyGradHorID()	    { return "Gradient horizon"; }
    static const char*	sKeyGradAuxDataID() { return "Gradient auxdata id"; }
    static const char*	sKeyGradType()	    { return "Gradient type"; }

    static const char*	sKeyRefHorID()	    { return "Reference horizon"; }
    static const char*	sKeyRefZ()	    { return "Reference z"; }
    static const char*	sKeyUseRefZ()	    { return "Use reference z"; }

};

} // namespace VolProc
