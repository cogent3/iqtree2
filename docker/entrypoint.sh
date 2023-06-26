#!/bin/bash

# Check if the SSH_PRIVATE_KEY_FILE_NAME environment variable is set
if [ -n "$SSH_PRIVATE_KEY_FILE_NAME" ]; then
    # Copy the SSH private key file specified in the environment variable
    cp "${SSH_PRIVATE_KEY_FILE_NAME}" /root/.ssh/id_rsa
    
    # Set permissions
    chmod 600 /root/.ssh/id_rsa
else
    echo "SSH_PRIVATE_KEY_FILE_NAME environment variable is not set."
fi

# Execute the main process
exec "$@"
