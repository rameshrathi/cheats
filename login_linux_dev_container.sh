#!/bin/bash

IMAGE_NAME="linux-dev-ssh"
CONTAINER_NAME="dev-container"
SSH_USER="root"
SSH_PASS="root"
SSH_PORT=22

# Build the image if not exist
if ! docker images --format "{{.Repository}}" | grep -q "^$IMAGE_NAME$"; then
    echo "Building Docker image $IMAGE_NAME..."
    docker build -t $IMAGE_NAME -f Dockerfile.ubuntu .
fi

# Create the container if not exist
if ! docker ps -a --format "{{.Names}}" | grep -q "^$CONTAINER_NAME$"; then
    echo "Creating container $CONTAINER_NAME..."
    docker run -d --name $CONTAINER_NAME -p 2222:$SSH_PORT $IMAGE_NAME
else
    # Start the container if it's stopped
    if [ "$(docker inspect -f '{{.State.Running}}' $CONTAINER_NAME)" != "true" ]; then
        echo "Starting container $CONTAINER_NAME..."
        docker start $CONTAINER_NAME
    fi
fi

# Allow some time for SSH to start
sleep 2

# Get the container's IP address
CONTAINER_IP=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $CONTAINER_NAME)

echo "Container IP: $CONTAINER_IP"

# SSH login (use password auth)
echo "Logging into the container via SSH..."

# You can use sshpass to automate login (install it via: sudo apt-get install sshpass)
if command -v sshpass &>/dev/null; then
    sshpass -p $SSH_PASS ssh -o StrictHostKeyChecking=no -p 2222 $SSH_USER@127.0.0.1
else
    echo "sshpass not found. Please install sshpass or login manually:"
    echo "ssh $SSH_USER@127.0.0.1 -p 2222"
fi
