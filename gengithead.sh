#!/bin/bash

#$1 = directory of git repo 
#$2 = optional ouput path, default ./git_version.h
#$3 = optional prefix, default is GIT 

if [ "$1" != "" ]; then
    GITPATH=$1
else
    GITPATH='./'
fi

if [ "$2" != "" ]; then
    HEADPATH=$2
else
    HEADPATH='./git_version'
fi

if [ "$3" != "" ]; then
    PREFIX=$3
else
    PREFIX='GIT'
fi


#echo "Generating header for git hash"

GIT_HEADER="$HEADPATH"

GIT_VERSION=$(git --git-dir=$GITPATH/.git --work-tree=$GITPATH log -1 --format=%H)

GIT_MODIFIED=0
echo $(git --git-dir=$GITPATH/.git --work-tree=$GITPATH status) > /tmp/git_status
if grep --quiet "modified" /tmp/git_status; then
   GIT_MODIFIED=1
fi

REPO_NAME=$(basename `git --exec-path=$GITPATH rev-parse --show-toplevel`)

echo "#ifndef $PREFIX""_VERSION_H" > $GIT_HEADER
echo "#define $PREFIX""_VERSION_H" >> $GIT_HEADER
echo "" >> $GIT_HEADER
echo "#define $PREFIX""_CURRENT_SHA1 \"$GIT_VERSION\"" >> $GIT_HEADER
echo "#define $PREFIX""_REPO_MODIFIED  $GIT_MODIFIED"  >> $GIT_HEADER
echo "" >> $GIT_HEADER
echo "" >> $GIT_HEADER
if [ $GIT_MODIFIED = 1 ]; then
echo "#if $PREFIX""_REPO_MODIFIED == 1" >> $GIT_HEADER
echo "  #ifndef GITHEAD_NOWARNING" >> $GIT_HEADER
echo "    #pragma message (\"********************************\")" >> $GIT_HEADER
echo "    #pragma message (\"*                              *\")" >> $GIT_HEADER
echo "    #pragma message (\"* WARNING: repository modified *\")" >> $GIT_HEADER
echo "    #pragma message (\"*  changes not committed for   *\")" >> $GIT_HEADER
echo "    #pragma message (\"*    $REPO_NAME    *\")" >> $GIT_HEADER
echo "    #pragma message (\"*                              *\")" >> $GIT_HEADER
echo "    #pragma message (\"********************************\")" >> $GIT_HEADER
echo "  #endif" >> $GIT_HEADER
echo "#endif" >> $GIT_HEADER
fi
echo "" >> $GIT_HEADER
echo "" >> $GIT_HEADER
echo "#endif" >> $GIT_HEADER