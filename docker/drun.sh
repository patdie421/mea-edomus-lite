docker --version > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "docker not installed"
   exit 1
fi

docker images > /dev/null 2>&1
if [ $? -ne 0 ]
then
   echo "can't get images, trying with sudo ..."
   sudo docker images > /dev/null 2>&1
   if [ $? -ne 0 ]
   then
      echo "can't get images, check docker installation"
      exit 1
   else
      DOCKER="sudo docker"
   fi 
else
   DOCKER="docker"
fi

$DOCKER run -it --network=host --rm --name mea_edomus --hostname mea_edomus mea_edomus
