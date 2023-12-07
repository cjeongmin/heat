FROM ubuntu:20.04

LABEL description="HEAT 프로그램을 실행시키기 위한 컨테이너를 만듭니다."

RUN apt update && \
    apt install -y gcc && \
    apt install -y git && \
    apt install -y make && \
    apt install -y curl && \
    git clone https://github.com/cjeongmin/heat.git

