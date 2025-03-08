#!/bin/bash

CONTAINER_NAME="multi-dev-container"
IMAGE_NAME="multi-dev-image"
HOST_PORT_CODE_SERVER=8080
CONTAINER_PORT_CODE_SERVER=8080
SHARED_DIR="$HOME/Documents/linux"

case "$1" in
    build)
        echo "📦 Building the Docker image..."
        docker build -t $IMAGE_NAME -f DockerfileMultiDev .
        ;;

    run)
        echo "🚀 Running the Docker container..."

        # Check if the container exists
        if docker ps -a --format '{{.Names}}' | grep -q "^$CONTAINER_NAME$"; then
            echo "🔄 Container '$CONTAINER_NAME' already exists."

            # Check if the container is running
            if docker ps --format '{{.Names}}' | grep -q "^$CONTAINER_NAME$"; then
                echo "✅ Container is already running."
            else
                echo "⏳ Restarting the container..."
                docker start $CONTAINER_NAME
            fi

            # Attach to the running container
            docker attach $CONTAINER_NAME
        else
            echo "🆕 Creating and running a new container..."

            # Build the image if it doesn't exist
            if ! docker images --format '{{.Repository}}' | grep -q "^$IMAGE_NAME$"; then
                echo "📦 Building the Docker image..."
                docker build -t $IMAGE_NAME -f DockerfileMultiDev .
            fi

            # Run the new container
            docker run -d --name $CONTAINER_NAME \
                -p $HOST_PORT_CODE_SERVER:$CONTAINER_PORT_CODE_SERVER \
                -p 3333:3333 -p 4444:4444 -p 22:22 \
                -v "$SHARED_DIR":/home/root \
                --workdir /home/root \
                --privileged \
                $IMAGE_NAME
        fi

        echo "🚀 Code Server is running at: http://localhost:$HOST_PORT_CODE_SERVER"
        echo "🔧 Use this container for ARM development as well!"
        ;;

    cleanup)
        echo "🗑 Removing all containers, images, and shared directory..."
        docker rm -f $CONTAINER_NAME
        docker rmi -f $IMAGE_NAME
        rm -rf "$SHARED_DIR"
        echo "✅ Cleanup complete."
        ;;

    *)
        echo "Usage: $0 {build|run|cleanup}"
        exit 1
        ;;
esac
