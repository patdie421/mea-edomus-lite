ORG=`pwd`
cd ~/environment/mea-edomus-lite

echo "*** parameters ***"
for i in $*
do
   echo $*
done
echo "******************"

make -f Makefile.ubuntu
cd "$ORG"
