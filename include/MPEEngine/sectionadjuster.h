#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          January 2005
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
				SectionAdjuster(EM::SectionID sid=-1);
    TypeSet<TrcKey>		tks_;
    TypeSet<TrcKey>		tksrc_;
    BufferString		errmsg_;
    EM::SectionID		sectionid_;
    float			thresholdval_;
    bool			removeonfailure_;

    TrcKey			seedtk_;
    bool			setundo_;
    int				seedid_;

    static const char*		sKeyAdjuster();
    static const char*		sKeyThreshold();
    static const char*		sKeyRemoveOnFailure();
};

} // namespace MPE

