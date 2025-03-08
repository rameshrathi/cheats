#!/bin/bash

# Build the Docker image
docker build -t arm-dev-container -f docker-dev-container .

# Run the container
docker run -d \
    -p 3333:3333 \
    -p 4444:4444 \
    -p 22:22 \
    --name arm-dev \
    -v $(pwd)/work:/home/devuser/work \
    arm-dev-container

# Print instructions
echo "Dev Container is running!"
echo "To attach to the container, run:"
echo "docker exec -it arm-dev /bin/bash"
echo "Working directory is mounted at /home/devuser/work"