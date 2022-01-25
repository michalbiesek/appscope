FROM node

RUN npm install -g json-dereference-cli json-schema-gendoc

COPY docker/docs/docker-entrypoint.sh .
ENTRYPOINT ["docker-entrypoint.sh"]
CMD ["doc"]