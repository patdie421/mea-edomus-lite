gen_dockerfile()
{
DEST=$1
echo "FROM debian"
echo "RUN apt-get update"
echo "RUN apt-get upgrade"
echo "RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections"
echo "RUN apt-get -y install apt-utils"
echo "RUN apt-get -y install netbase"
echo "RUN apt-get -y install net-tools"
echo "RUN apt-get -y install python"
echo "RUN apt-get -y install libpython2.7"
echo "RUN apt-get -y install curl"
echo "WORKDIR $DEST"
echo "COPY . ."
echo "EXPOSE 3865/udp"
echo "CMD [\"bash\", \"start.sh\" ]"
}

gen_start_sh()
{
DIR=$1
echo "\"$DIR\"/bin/xplhub &"
echo "sleep 5"
echo "\"$DIR\"/bin/mea-edomus --basepath=\"$DIR\" &"
echo "sleep 5"
echo "\"$DIR\"/bin/demo_device"
}

ORG=`pwd`
#SOURCE="/data/dev/app2"
SOURCE="$1"
DEST="/app"

docker rmi mea_edomus

gen_start_sh "$DEST" > start.sh.tmp
gen_dockerfile "$DEST" > Dockerfile.tmp
cp .dockerignore "$SOURCE"
cp Dockerfile.tmp "SOURCE"/Dockerfile
cp start.sh.tmp "$SOURCE"/start.sh
chmod +x "$SOURCE"/start.sh

echo "BUILD_DEMO"
cd ..
./build_demo.sh "$SOURCE" "$DEST"

echo "BUILD_DOCKER"
cd "$SOURCE"
docker build -t mea_edomus .
