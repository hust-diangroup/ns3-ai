docker rmi -f ns3-3.30
docker build -t ns3-3.30 .

docker stop ns3-3.30
docker rm ns3-3.30

docker run -it --name ns3-3.30 --restart=always ns3-3.30 bash
