FROM ubuntu:20.04

LABEL "com.github.actions.name"="Fetch Rack SDK"
LABEL "com.github.actions.description"="Download and unzip the Rack SDK"
LABEL "com.github.actions.icon"="briefcase"
LABEL "com.github.actions.color"="gray-dark"

RUN apt-get update
RUN apt-get install -y curl unzip

ADD entrypoint2.sh /entrypoint2.sh
RUN chmod a+x /entrypoint2.sh

ENTRYPOINT ["/entrypoint2.sh"]
