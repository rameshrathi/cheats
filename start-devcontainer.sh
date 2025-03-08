#!/bin/bash

# Check if the image already exists
if docker images -q arm-dev-container 2>/dev/null | grep -q .; then
  echo "Image 'arm-dev-container' already exists. Skipping build."
else
  # Build the Docker image if it doesn't exist
  echo "Building Docker image 'arm-dev-container'..."
  docker build -t arm-dev-container -f docker-dev-container .
fi

# Check if the container already exists and remove it if it does.
if docker ps -aq -f name=arm-dev 2>/dev/null | grep -q .; then
    echo "Container 'arm-dev' already exists. Removing..."
    docker stop arm-dev 2>/dev/null
    docker rm arm-dev 2>/dev/null
fi

# Run the container
echo "Running Docker container 'arm-dev'..."
docker run -d \
    -p 3333:3333 \
    -p 4444:4444 \
    -p 22:22 \
    --name arm-dev \
    -v "$(pwd)/work:/home/devuser/work" \
    arm-dev-container

# Print instructions
echo "Dev Container 'arm-dev' is running!"
echo "To attach to the container, run:"
echo "docker exec -it arm-dev /bin/bash"
echo "Working directory is mounted at /home/devuser/work"