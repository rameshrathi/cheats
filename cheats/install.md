#!/bin/bash

# Set variables
DOCKER_IMAGE="ubuntu-arm-dev"
CONTAINER_NAME="my-arm-dev"
SHARED_FOLDER="$HOME/Documents/linux"

# Check if the user provided an argument
if [ -z "$1" ]; then
    echo "Usage: ./configure.sh [build|run|cleanup]"
    exit 1
fi

# Handle "build" option
if [ "$1" == "build" ]; then
    echo "🔨 Building Docker image..."

    # Create shared folder if it doesn't exist
    mkdir -p "$SHARED_FOLDER"

    # Build the Docker image
    docker build -t $DOCKER_IMAGE .

    echo "✅ Build complete! Run the container using:"
    echo "   ./configure.sh run"
    exit 0
fi

# Handle "run" option
if [ "$1" == "run" ]; then
    echo "🚀 Running the Docker container..."

    # Check if container exists
    if docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER_NAME$"; then
        echo "⏳ Starting existing container..."
        docker start -it $CONTAINER_NAME
    else
        echo "🆕 Creating a new container..."
        docker run -it --name $CONTAINER_NAME \
          -v "$SHARED_FOLDER":/home/root \
          --workdir /home/root \
          --privileged \
          $DOCKER_IMAGE
    fi
    exit 0
fi

# Handle "cleanup" option
if [ "$1" == "cleanup" ]; then
    echo "🧹 Cleaning up Docker and Linux shared folder..."

    # Stop and remove the container
    if docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER_NAME$"; then
        echo "🛑 Stopping and removing container: $CONTAINER_NAME"
        docker stop $CONTAINER_NAME
        docker rm $CONTAINER_NAME
    fi

    # Remove the Docker image
    if docker images -q $DOCKER_IMAGE > /dev/null; then
        echo "🗑 Removing Docker image: $DOCKER_IMAGE"
        docker rmi $DOCKER_IMAGE
    fi

    # Delete the shared folder
    if [ -d "$SHARED_FOLDER" ]; then
        echo "🗑 Deleting shared folder: $SHARED_FOLDER"
        rm -rf "$SHARED_FOLDER"
    fi

    echo "✅ Cleanup complete!"
    exit 0
fi

# If an invalid option is given
echo "❌ Invalid option. Use: ./configure.sh [build|run|cleanup]"
exit 1
