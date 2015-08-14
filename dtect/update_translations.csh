#!/bin/csh -f
#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id$
#
#_______________________________________________________________________________

# The purpose of this script is to make a .pro file to tell lupdate which
# files to scan and for which languages a ts file has to be created.
# It will:
# - make  a copy of an od directory
# - replace mExpClass and mClass by class
# - replace *_static_tr( "myfunc", ... ) with static_func_myfunc( ... )
# - create lupdate.pro
# - run lupdate for plural (*en-us.ts) and non-plural translations (all others).
# - copy the new translations into the work-directory.
# - remove the temporary copy

if (  $#argv < 3 ) then
    echo "Usage : $0 <sourcedir> <binarydir> <lupdate>"
    exit 1
endif


if ( $?DTECT_SCRIPT_VERBOSE ) then
    echo "$0  ++++++"
    echo "args: $*"
    set verbose=yes
    set echo=on
endif

set scriptdir=`dirname $0`
set nrcpu = `${scriptdir}/GetNrProc`

set sourcedir=$1
set binarydir=$2
set lupdate=$3
set tmpoddir=${binarydir}/lupdate_tmp_$$

set kernel=`uname -a | awk '{print $1}'`
if ( "${kernel}" == "Darwin" ) then
    set lupdate_dir=`dirname ${lupdate}`
    setenv DYLD_LIBRARY_PATH ${lupdate_dir}/../lib
endif

if ( -e $tmpoddir ) then
    \rm -rf $tmpoddir
endif

mkdir $tmpoddir
cp -a ${sourcedir}/{CMakeModules,data,src,include,plugins} $tmpoddir/.

set srcdir=$tmpoddir/data/localizations/source
set profnm=$srcdir/normaltrans.pro


set olddir=`pwd`
cd srcdir

set headers = `find $tmpoddir -path "*.h"`

#Create file with all files
set filelist="filelist.txt"

echo "" >> ${filelist}

echo -n "HEADERS = " >> ${filelist}
foreach fnm ( $headers )
    echo " \" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
end

echo "" >> ${filelist}

set sources = `find ${tmpoddir} -path "*.cc"`

echo -n "SOURCES = " >> ${filelist}
foreach fnm ( $sources )
    echo " \" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
end

# Create a list of target .ts files for normal ts files
echo -n "TRANSLATIONS = " > ${profnm}

echo " \" >> ${profnm}
echo -n "    od_template.ts" >> ${profnm}

foreach fnm ( *.ts )
    if ( "${fnm}" =~ "*en-us.ts" ) then
	continue
    endif

    echo " \" >> ${profnm}
    echo -n "    $fnm" >> ${profnm}
end

cat ${filelist} >> ${profnm}

#Create a list of .ts files for plural operations
set pluralpro=$srcdir/plural.pro
echo -n "TRANSLATIONS = " > ${pluralpro}

foreach fnm ( *.ts )
    if ( ! ("${fnm}" =~ "*en-us.ts") ) then
	continue
    endif

    echo " \" >> ${pluralpro}
    echo -n "    $fnm" >> ${pluralpro}
end

#Merge the filelist with the list of plural ts-files
cat ${filelist} >> ${pluralpro}

#Filter the sources for patterns
echo ${sources} | xargs -P ${nrcpu} sed -i \
	-e 's/[^ \t]*_static_tr([ \t]*/static_func___begquote/g' \
	-e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/::tr(,/::tr(/g' \
	-e 's/__begquote"//g' \
	-e 's/"__endquote//g'

#Filter the headers for patterns
echo ${headers} | xargs -P ${nrcpu} sed -i \
	-e 's/mExpClass(.*)/class/g' \
	-e 's/mClass(.*)/class/g' \
	-e 's/[^ \t]*_static_tr([ \t]*/static_func___begquote/g' \
	-e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/::tr(,/::tr(/g' \
	-e 's/__begquote"//g' \
	-e 's/"__endquote//g'


#Run lupdate
${lupdate} -silent -locations relative ${profnm}
${lupdate} -silent -locations relative -pluralonly ${pluralpro}

#Copy the resulting ts-files back
foreach fnm ( *.ts )
    cp -f ${fnm} ${binarydir}/data/localizations/source/
end

#Go back to starting dir
cd ${olddir}

#Remvoe temporary dir
\rm -rf  ${tmpoddir}

exit 0
