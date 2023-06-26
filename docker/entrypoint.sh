#!/bin/bash

# Set the path to the SSH private key
SSH_PRIVATE_KEY=/root/.ssh/id_rsa

# Check if the SSH private key file exists
if [ -f "${SSH_PRIVATE_KEY}" ]; then
    # Set permissions
    chmod 600 "${SSH_PRIVATE_KEY}"
else
    echo "No SSH private key found at: ${SSH_PRIVATE_KEY}"
fi

# Execute the main process
exec "$@"
