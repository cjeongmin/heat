FROM ubuntu:20.04

LABEL description="HEAT 프로그램을 실행시키기 위한 컨테이너를 만듭니다."

RUN DEBIAN_FRONTEND=noninteractive && TZ=Asia/Seoul && apt-get update && apt-get -y install tzdata

RUN apt update && \
    apt install -y gcc && \
    apt install -y git && \
    apt install -y make && \
    apt install -y curl && \
    git clone https://github.com/cjeongmin/heat.git && \
    apt install -y build-essential && \
    apt install -y nodejs && \
    apt install -y npm && \
    npm install -g yarn && \
    cd heat/test-server && \
    yarn install --check-files