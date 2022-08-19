#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "task.h"

#include "emposid.h"
#include "trckey.h"
#include "trckeyzsampling.h"

namespace Attrib { class SelSpec; }


namespace MPE
{

class SectionExtender;

/*!
\brief SequentialTask to adjust the section of an EM object with ID
EM::SectionID.
*/

mExpClass(MPEEngine) SectionAdjuster : public SequentialTask
{
public:
    virtual			~SectionAdjuster();

    mDeprecatedObs
    EM::SectionID		sectionID() const;

    virtual void		reset() {}

    void			setPositions(const TypeSet<TrcKey>& targets,
					     const TypeSet<TrcKey>* src=0 );
    void			setSeedPosition(const TrcKey&);

    int				nextStep() override;
    const char*			errMsg() const;

    virtual TrcKeyZSampling	getAttribCube(const Attrib::SelSpec&) const;
    				/*!<\returns the cube in which I need the
				     given attrib to track in activevolum. */
    virtual void		getNeededAttribs(
					TypeSet<Attrib::SelSpec>&) const;
    virtual bool		is2D() const			{ return false;}
    virtual bool		hasInitializedSetup() const	{ return true; }

    virtual int			getNrAttributes() const		{ return 0; }
    virtual const Attrib::SelSpec* getAttributeSel( int idx ) const { return 0;}
    virtual void		setAttributeSel( int idx,
	    					 const Attrib::SelSpec& ) {}

    void			setThresholdValue(float val);
    float			getThresholdValue() const;
    bool			removeOnFailure(bool yn);
    				/*!<If true, tracked nodes that does not
				    meet certain constraits, e.g. thresholds,
				    are removed. If not, the initial value
				    is kept.
				    \returns the previous status. */
    bool			removesOnFailure() const;

    virtual void		fillPar(IOPar&) const;
    virtual bool		usePar(const IOPar&);

    void			setUndo(bool yn) { setundo_ = yn; }
    void			setSeedId(int);

protected:
				SectionAdjuster();

    TypeSet<TrcKey>		tks_;
    TypeSet<TrcKey>		tksrc_;
    BufferString		errmsg_;
    float			thresholdval_		= 0.5f;
    bool			removeonfailure_	= true;

    TrcKey			seedtk_			= TrcKey::udf();
    bool			setundo_		= true;
    int				seedid_			= 0;

    static const char*		sKeyAdjuster();
    static const char*		sKeyThreshold();
    static const char*		sKeyRemoveOnFailure();
};

} // namespace MPE
