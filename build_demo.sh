ORG=`pwd`
DEMODIR=$1

mkdir "$DEMODIR"

cd "$ORG"/complements/mea-homebridge
./build.sh "$DEMODIR"

cd "$ORG"/complements/xplhub
./build.sh "$DEMODIR"

cd "$ORG"/complements/mea-xplToHomebridge
./build.sh "$DEMODIR"

cd "$ORG"
./build.sh "$DEMODIR"

cp scripts/*.sh "$DEMODIR"
