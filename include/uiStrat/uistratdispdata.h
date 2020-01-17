#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Mar 2010
________________________________________________________________________

-*/

#include "uistratmod.h"
#include "notify.h"
#include "color.h"
#include "ranges.h"


namespace Strat
{
    class UnitRef;
    class LeavedUnitRef;
    class NodeOnlyUnitRef;
    class NodeUnitRef;
    class RefTree;
}

class uiStratRefTree;
class uiTreeViewItem;

mExpClass(uiStrat) StratDispData
{
public:
			StratDispData();
			~StratDispData();

    mStruct(uiStrat) Unit
    {
			Unit(const char* nm, const char* fullcode=0,
				    const Color& col = Color::White() )
				: name_( nm )
				, fullcode_(fullcode)
				, color_(col)
				, isdisplayed_(true)
				{}

	const char*	name() const	{ return name_.buf(); }
	const char*	fullCode() const { return fullcode_.buf(); }

	Color		color_;
	Interval<float>	zrg_;
	bool		isdisplayed_;

	int		colidx_; //tree depth

    protected :

	BufferString	name_;
	BufferString    fullcode_;
    };


    mStruct(uiStrat) Level
    {
			Level( const char* nm, const char* unitcode )
				: name_(nm)
				, unitcode_(unitcode)
				{}

	const BufferString name_;
	const BufferString unitcode_;
	Color		color_;
	float		zpos_;
    };


    mStruct(uiStrat) Column
    {
			Column( const char* nm )
			    : name_(nm)
			    , isdisplayed_(true)
			    {}
			~Column()
			{
			    deepErase( units_ );
			    deepErase( levels_ );
			}

	const BufferString name_;
	bool		isdisplayed_;

	ObjectSet<Unit>	units_;
	ObjectSet<Level> levels_;
    };

    void		eraseData();

    void		addCol(Column*);
    int			nrCols() const;
    Column*		getCol(int colidx);
    const Column*	getCol(int colidx) const;

    void		addUnit(int colidx,Unit*);
    int			nrUnits(int colidx) const;
    Unit*		getUnit(int colidx,int uidx);
    const Unit*		getUnit(int colidx,int uidx) const;

    int			nrLevels(int colidx) const;
    const Level*	getLevel(int colidx,int lidx) const;

    int			nrDisplayedCols() const;

protected:
    ObjectSet<Column> cols_;
};



/*!\brief used to gather all units and tied levels from a tree for display*/

mExpClass(uiStrat) uiStratTreeToDisp : public CallBacker
{
public:
			uiStratTreeToDisp(StratDispData&,
					bool withaux=true,
					bool withlvls=true);
			~uiStratTreeToDisp();

    void		setTree();
    Notifier<uiStratTreeToDisp> newtreeRead;

    int			levelColIdx() const	{ return levelcolidx_; }

protected:

    StratDispData&	data_;
    Strat::RefTree*	tree_;

    bool		withauxs_; //lithologies & descriptions
    bool		withlevels_;
    int			lithocolidx_;
    int			desccolidx_;
    int			levelcolidx_;

    void		readUnits();
    void		addUnit(const Strat::NodeUnitRef&);
    void		addDescs(const Strat::LeavedUnitRef&);
    void		addLithologies(const Strat::LeavedUnitRef&);
    void		addLevel(const Strat::LeavedUnitRef&);
    void		addAnnot(const char*,Interval<float>& posrg,bool);
    void		readFromTree();

    void		triggerDataChange(CallBacker*);
    void		treeDel(CallBacker*);
};


/*!brief used to write directly in the treeView of the uiStratRefTree
   as if we were handling the uiTree directly*/

mExpClass(uiStrat) uiStratDispToTree : public CallBacker
{
public:
			uiStratDispToTree(uiStratRefTree&);
			~uiStratDispToTree();

    uiTreeViewItem*	getItemFromTree(const char*);

    void		handleUnitMenu(const char*);
    void		handleUnitProperties(const char*);
    void		setUnitLvl(const char*);
    void		addUnit(const char*);
    uiTreeViewItem*	setCurrentTreeItem(const char*);

protected:

    uiStratRefTree&	uitree_;
};
